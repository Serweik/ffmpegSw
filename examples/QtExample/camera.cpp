#include "camera.h"

Camera::Camera(QSharedPointer<AVffmpegWrapper> _aVffmpegWrapper, QWidget* parent):
	QWidget(parent), aVffmpegWrapper(_aVffmpegWrapper)
{

}

Camera::~Camera() {
	if(fileDescriptor != -1) {
		killTimer(renderingTimer);
		aVffmpegWrapper->closeFile(fileDescriptor);
		if(audioPlayingThreadIsRunning) {
			audioPlayingThreadIsStopping = true;
			if(audioPlayingThread.joinable()) {
				audioPlayingThread.join();
			}
		}
		if(audioOutput) delete audioOutput;
	}
	if(videoBuffer) delete [] videoBuffer;
}

void Camera::setPath(QString path) {
	this->path = path;
}

bool Camera::start() {
	fileDescriptor = aVffmpegWrapper->openFile(path.toStdString(),
											   AVfileContext::REPEATE_AND_RECONNECT,
											   AVfileContext::VIDEO | AVfileContext::AUDIO);

	if(fileDescriptor == -1) {
		return false;
	}
	imageWidth = 800;
	aVffmpegWrapper->setVideoConvertingParameters(fileDescriptor, AV_PIX_FMT_RGB24, SWS_FAST_BILINEAR, imageWidth, 480);
	videoBufsize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, imageWidth, 480, 1);
	videoBuffer = new uint8_t[videoBufsize];
	aVffmpegWrapper->setAudioConvertingParameters(fileDescriptor, AV_SAMPLE_FMT_S16, AV_CH_LAYOUT_STEREO);

	renderingTimer = startTimer(2);
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

	int64_t interval = static_cast<int64_t>(1000000.0 / (sampleRate / numSamples));
	while(!audioPlayingThreadIsStopping) {
		int result = static_cast<int>(aVffmpegWrapper->getAudioData(fileDescriptor, &buffer[0], static_cast<uint32_t>(audioBufsize)));
		if(result > 0) {
			std::unique_lock<std::mutex> locker(audioSamplesMutex);
			if(audioSamplesBuffer.size() < audioBufsize * 10) {
				audioSamplesBuffer.append(reinterpret_cast<const char*>(&buffer[0]), result);
			}
			feedAudioOutput();
			locker.unlock();
			int64_t lastTime = av_gettime();
			int64_t newInterval = interval - (av_gettime() - lastTime);
			if(newInterval >= 100) {
				std::this_thread::sleep_for(std::chrono::microseconds(newInterval));
			}
		}else {
			std::this_thread::sleep_for(std::chrono::microseconds(100));
		}
	}
}

void Camera::timerEvent(QTimerEvent* event) {
	if(event->timerId() == renderingTimer) {
		killTimer(renderingTimer);
		if(!aVffmpegWrapper->endOfFile(fileDescriptor)) {
			if(!audioPlayingThreadIsRunning && aVffmpegWrapper->hasAudioStream(fileDescriptor)) {
				numSamples = 1536;
				audioBufsize = numSamples * 2 * 2; /*because 2 channels and 2 bytes per sample*/
				format.setChannelCount(2);
				format.setSampleSize(16);
				format.setCodec("audio/pcm");
				format.setByteOrder(QAudioFormat::LittleEndian);
				format.setSampleType(QAudioFormat::SignedInt);
				sampleRate = aVffmpegWrapper->audioSampleRate(fileDescriptor);
				format.setSampleRate(static_cast<int>(sampleRate));
				audioOutput = new QAudioOutput(format);
				audioOutput->setBufferSize(audioBufsize * 20);
				audioDevice = audioOutput->start();
				audioDevice->open(QIODevice::WriteOnly);
				if(audioOutput->error() != QAudio::NoError) {
					delete audioOutput;
					audioOutput = nullptr;
					audioDevice = nullptr;
				}else {
					audioPlayingThreadIsRunning = true;
					audioPlayingThreadIsStopping = false;
					audioPlayingThread = std::thread(&Camera::audioPlaying, this);
					connect(audioOutput, SIGNAL(notify()), this, SLOT(audioNotify()));
					audioOutput->setNotifyInterval(20);
				}
			}
			if(aVffmpegWrapper->getVideoData(fileDescriptor, &videoBuffer[0], videoBufsize)) {
				this->update();
			}
			renderingTimer = startTimer(2);
		}else {
			this->deleteLater();
		}
	}
}

void Camera::resizeEvent(QResizeEvent* event){
	imageWidth = event->size().width();
	if(aVffmpegWrapper->setVideoConvertingParameters(fileDescriptor, AV_PIX_FMT_RGB24, SWS_FAST_BILINEAR, imageWidth, event->size().height())) {
		imageWidth = aVffmpegWrapper->getDestinationWidth(fileDescriptor);
	}
	videoBufsize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, imageWidth, event->size().height(), 1);
	delete [] videoBuffer;
	videoBuffer = new uint8_t[videoBufsize];
	QWidget::resizeEvent(event);
}

void Camera::paintEvent(QPaintEvent* event) {
	if(videoBuffer) {
		QImage image(&videoBuffer[0], imageWidth, this->height(), QImage::Format_RGB888);
		QPainter painter(this);
		painter.drawImage(this->rect(), image, image.rect());
	}
	event->accept();
}
