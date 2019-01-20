#include "camera.h"

Camera::Camera(QSharedPointer<AVffmpegWrapper> _aVffmpegWrapper, QWidget* parent):
	QWidget(parent), aVffmpegWrapper(_aVffmpegWrapper) {

}

Camera::~Camera() {
	if(fileDescriptor != -1) {
		aVffmpegWrapper->closeFile(fileDescriptor);
	}
}

void Camera::setPath(QString path) {
	this->path = path;
}

bool Camera::start() {
	fileDescriptor = aVffmpegWrapper->openFile(path.toStdString(),
											   AVfileContext::REPEATE_AND_RECONNECT,
											   AVfileContext::VIDEO);
	if(fileDescriptor == -1) {
		return false;
	}
	imageWidth = 800;
	aVffmpegWrapper->setVideoConvertingParameters(fileDescriptor, AV_PIX_FMT_RGB24, SWS_FAST_BILINEAR, imageWidth, 480);
	bufsize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, imageWidth, 480, 1);
	buffer = new uint8_t[bufsize];
	renderingTimer = startTimer(5);
	return true;
}

void Camera::timerEvent(QTimerEvent* event) {
	if(event->timerId() == renderingTimer) {
		killTimer(renderingTimer);
		if(!aVffmpegWrapper->endOfFile(fileDescriptor)) {
			if(aVffmpegWrapper->getVideoData(fileDescriptor, &buffer[0], bufsize)) {
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
	bufsize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, imageWidth, event->size().height(), 1);
	delete [] buffer;
	buffer = new uint8_t[bufsize * 2];
	QWidget::resizeEvent(event);
}

void Camera::paintEvent(QPaintEvent* event) {
	if(buffer) {
		QImage image(&buffer[0], imageWidth, this->height(), QImage::Format_RGB888);
		QPainter painter(this);
		painter.drawImage(this->rect(), image, image.rect());
	}
	event->accept();
}
