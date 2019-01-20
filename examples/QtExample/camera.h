#ifndef CAMERA_H
#define CAMERA_H

#include "../../src/avffmpegwrapper.h"

#include <QWidget>
#include <QTimerEvent>
#include <QResizeEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QImage>
#include <QSize>
#include <QSharedPointer>

#include <QDebug>

class Camera: public QWidget {
	Q_OBJECT
	public:
		explicit Camera(QSharedPointer<AVffmpegWrapper> _aVffmpegWrapper, QWidget* parent = nullptr);
		~Camera() override;

		void setPath(QString path);
		bool start();

	private:
		int fileDescriptor = -1;
		QSharedPointer<AVffmpegWrapper> aVffmpegWrapper;
		QString path;
		int renderingTimer = -1;
		uint8_t* buffer = nullptr;
		int bufsize = 0;
		int imageWidth = 0;

	// QObject interface
	protected:
		virtual void timerEvent(QTimerEvent* event) override;

	// QWidget interface
	protected:
		virtual void resizeEvent(QResizeEvent* event) override;
		virtual void paintEvent(QPaintEvent* event) override;
};

#endif // CAMERA_H