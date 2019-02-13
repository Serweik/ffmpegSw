#ifndef CAMERA_H
#define CAMERA_H

#include "../../src/avfilecontext.h"

#include <QWidget>
#include <QTimerEvent>
#include <QResizeEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QImage>
#include <QPixmap>
#include <QSize>
#include <QAudioOutput>
#include <QByteArray>
#include <QThread>

//#include <iostream>

class Camera: public QWidget {
	Q_OBJECT
	public:
		explicit Camera(QWidget* parent = nullptr);
		~Camera() override;

		void setPath(QString path);
		bool start();

	private slots:
		void audioNotify();

	private:
		QString path;
		int renderingTimer = -1;
		int imageWidth = 0;
		int imageHeight = 0;

		QByteArray videoFrame;
		QPixmap videoPixmap;
		bool videoFrameUpdated = false;

		QByteArray audioSamplesBuffer;
		int numSamples = 0;
		int audioBufsize = 0;
		double sampleRate = 0.0;
		QAudioOutput* audioOutput = nullptr;
		QIODevice* audioDevice = nullptr;
		QAudioFormat format;
		std::thread audioPlayingThread;
		std::mutex audioSamplesMutex;
		bool audioPlayingThreadIsRunning = false;
		bool audioPlayingThreadIsStopping = false;
		QThread* audioThread = nullptr;
		AVfileContext avFile;

		void audioPlaying();
		void feedAudioOutput();

		void videoPlaying();

	// QObject interface
	protected:
		virtual void timerEvent(QTimerEvent* event) override;

	// QWidget interface
	protected:
		virtual void resizeEvent(QResizeEvent* event) override;
		virtual void paintEvent(QPaintEvent* event) override;
};

#endif // CAMERA_H
