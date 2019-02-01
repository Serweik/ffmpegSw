#include "camera.h"

Camera::Camera(QSharedPointer<AVffmpegWrapper> _aVffmpegWrapper, QWidget* parent):
	QWidget(parent), aVffmpegWrapper(_aVffmpegWrapper)
{

}

Camera::~Camera() {
	if(fileDescriptor != -1) {
		aVffmpegWrapper->closeFile(fileDescriptor);
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

	format.setChannelCount(2);
	format.setSampleSize(16);
	format.setCodec("audio/pcm");
	format.setByteOrder(QAudioFormat::LittleEndian);
	format.setSampleType(QAudioFormat::SignedInt);
	numSamples = 1536;
	audioBufsize = numSamples * 2 * 2; /*because 2 channels and 2 bytes per sample*/
	aVffmpegWrapper->setAudioConvertingParameters(fileDescriptor, AV_SAMPLE_FMT_S16, AV_CH_LAYOUT_STEREO);
	aVffmpegWrapper->setAudioCallback(fileDescriptor, [&](uint8_t* data, uint32_t len, int& writed) {
		if(audioDevice) {
			writed = static_cast<int>(audioDevice->write(reinterpret_cast<const char*>(data), len));
		}
	}, numSamples);
	renderingTimer = startTimer(2);
	return true;
}

void Camera::timerEvent(QTimerEvent* event) {
	if(event->timerId() == renderingTimer) {
		killTimer(renderingTimer);
		if(!aVffmpegWrapper->endOfFile(fileDescriptor)) {
			if(audioDevice == nullptr && aVffmpegWrapper->hasAudioStream(fileDescriptor)) {
				format.setSampleRate(aVffmpegWrapper->audioSampleRate(fileDescriptor));
				audioOutput = new QAudioOutput(format, this);
				audioOutput->setBufferSize(audioBufsize * 2);
				audioDevice = audioOutput->start();
				if(audioOutput->error() != QAudio::NoError) {
					audioDevice = nullptr;
					audioOutput->deleteLater();
				}
			}
			if(aVffmpegWrapper->getVideoData(fileDescriptor, &videoBuffer[0], videoBufsize)) {
				this->update();
			}
			renderingTimer = startTimer(5);
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
