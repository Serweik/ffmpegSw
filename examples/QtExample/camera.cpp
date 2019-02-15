#include "camera.h"

Camera::Camera(QWidget* parent):
	QWidget(parent)
{

}

Camera::~Camera() {
	killTimer(renderingTimer);
	avFile.closeFile();
	if(audioPlayingThreadIsRunning) {
		audioPlayingThreadIsStopping = true;
		if(audioPlayingThread.joinable()) {
			audioPlayingThread.join();
		}
	}
	if(audioThread) {
		audioThread->exit(0);
		audioThread->wait(1000);
	}
	if(audioOutput) delete audioOutput;
}

void Camera::setPath(QString path) {
	this->path = path;
}

bool Camera::start() {
	imageWidth = 800;
	imageHeight = 480;
	avFile.setVideoConvertingParameters(AV_PIX_FMT_BGRA, SWS_FAST_BILINEAR, imageWidth, imageHeight);
	imageWidth = avFile.getDestinationWidth();
	imageHeight = avFile.getDestinationHeigth();
	videoFrame.resize(av_image_get_buffer_size(AV_PIX_FMT_BGRA, imageWidth, imageHeight, 32));
	videoPixmap = QPixmap(imageWidth, imageHeight);
	avFile.setAudioConvertingParameters(AV_SAMPLE_FMT_S16, AV_CH_LAYOUT_STEREO);
	if(!avFile.openFile(path.toStdString(),
					   AVfileContext::REPEATE_AND_RECONNECT,
					   AVfileContext::VIDEO | AVfileContext::AUDIO)) {
		return false;
	}
	renderingTimer = startTimer(2, Qt::PreciseTimer);
	return true;
}

void Camera::audioNotify() {
	std::unique_lock<std::mutex> locker(audioSamplesMutex);
	feedAudioOutput();
}

void Camera::feedAudioOutput() {
	int size = std::min(audioOutput->bytesFree(), audioSamplesBuffer.size());
	if(size > 0) {
		audioSamplesBuffer.remove(0, static_cast<int>(audioDevice->write(audioSamplesBuffer, size)));
	}
}

void Camera::audioPlaying() {
	uint8_t* buffer = nullptr;
	int temp = 0;
	auto deleter = [&](int*){
		if(buffer) delete [] buffer;
		audioPlayingThreadIsRunning = false;
	};
	std::unique_ptr<int, decltype(deleter)> threadFinishIndicator(&temp, deleter);

	buffer = new uint8_t[audioBufsize];

	int64_t interval = static_cast<int64_t>(1000000.0 / (sampleRate / numSamples)) - 700;
	int64_t lastTime = av_gettime();
	while(!audioPlayingThreadIsStopping) {
		while(av_gettime() - lastTime < interval);
		lastTime = av_gettime();
		int result = static_cast<int>(avFile.getAudioData(&buffer[0], static_cast<uint32_t>(audioBufsize)));
		if(result > 0) {
			std::unique_lock<std::mutex> locker(audioSamplesMutex);
			audioSamplesBuffer.append(reinterpret_cast<const char*>(&buffer[0]), result);
			feedAudioOutput();
			while(audioSamplesBuffer.size() > audioOutput->bufferSize()) {
				feedAudioOutput();
			}
			locker.unlock();
			int64_t newInterval = (interval - (av_gettime() - lastTime)) - 3000;
			if(newInterval >= 3000) {
				std::this_thread::sleep_for(std::chrono::microseconds(newInterval));
			}
		}else {
			std::this_thread::sleep_for(std::chrono::microseconds(100));
		}
	}
}

void Camera::timerEvent(QTimerEvent* event) {
	if(event->timerId() == renderingTimer) {
		if(!avFile.endOfFile()) {
			if(!audioPlayingThreadIsRunning && avFile.hasAudioStream()) {
				numSamples = avFile.getNbSamples();
				if(numSamples < 100) {
					numSamples = 1536;
				}
				audioBufsize = numSamples * 2 * 2; /*because 2 channels and 2 bytes per sample*/
				format.setChannelCount(2);
				format.setSampleSize(16);
				format.setCodec("audio/pcm");
				format.setByteOrder(QAudioFormat::LittleEndian);
				format.setSampleType(QAudioFormat::SignedInt);
				sampleRate = avFile.audioSampleRate();
				format.setSampleRate(static_cast<int>(sampleRate));
				audioOutput = new QAudioOutput(format);
				audioThread = new QThread(this);
				audioOutput->moveToThread(audioThread);
				audioOutput->setBufferSize(audioBufsize * 20);
				audioDevice = audioOutput->start();
				if(audioOutput->error() != QAudio::NoError) {
					delete audioThread;
					audioThread = nullptr;
					delete audioOutput;
					audioOutput = nullptr;
					audioDevice = nullptr;
				}else {
					audioPlayingThreadIsRunning = true;
					audioPlayingThreadIsStopping = false;
					audioPlayingThread = std::thread(&Camera::audioPlaying, this);
					connect(audioOutput, SIGNAL(notify()), this, SLOT(audioNotify()), Qt::DirectConnection);
					audioOutput->setNotifyInterval(static_cast<int>((1000.0 / (sampleRate / numSamples)) / 2));
					audioThread->start();
				}
			}
			//bool result = avFile.getVideoData(reinterpret_cast<uint8_t*>(videoFrame.data()), videoFrame.size());
			if(avFile.getVideoData(reinterpret_cast<uint8_t*>(videoFrame.data()), videoFrame.size())) {
				QImage image(reinterpret_cast<uint8_t*>(videoFrame.data()), imageWidth, imageHeight, imageWidth * 4, QImage::Format_RGB32);
				if(!image.isNull()) {
					videoPixmap = QPixmap::fromImage(image);
					videoFrameUpdated = true;
					this->repaint();
				}
			}
		}else {
			killTimer(renderingTimer);
			this->deleteLater();
		}
	}
}

void Camera::resizeEvent(QResizeEvent* event){
	videoFrameUpdated = false;
	imageWidth = event->size().width();
	imageHeight = event->size().height();
	if(avFile.setVideoConvertingParameters(AV_PIX_FMT_BGRA, SWS_FAST_BILINEAR, imageWidth, imageHeight)) {
		imageWidth = avFile.getDestinationWidth();
		imageHeight = avFile.getDestinationHeigth();
	}
	videoFrame.resize(av_image_get_buffer_size(AV_PIX_FMT_BGRA, imageWidth, imageHeight, 32));
	QWidget::resizeEvent(event);
}

void Camera::paintEvent(QPaintEvent* event) {
	if(videoFrameUpdated) {
		videoFrameUpdated = false;
		QPainter painter(this);
		painter.drawPixmap(this->rect(), videoPixmap, videoPixmap.rect());
	}
	event->accept();
}
