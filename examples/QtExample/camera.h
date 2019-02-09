#ifndef CAMERA_H
#define CAMERA_H

#include "../../src/avffmpegwrapper.h"

#include <QWidget>
#include <QTimerEvent>
#include <QResizeEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QImage>
#include <QPixmap>
#include <QSize>
#include <QSharedPointer>
#include <QAudioOutput>
#include <QByteArray>

class Camera: public QWidget {
	Q_OBJECT
	public:
		explicit Camera(QSharedPointer<AVffmpegWrapper> _aVffmpegWrapper, QWidget* parent = nullptr);
		~Camera() override;

		void setPath(QString path);
		bool start();

	private slots:
		void audioNotify();

	private:
		int fileDescriptor = -1;
		QSharedPointer<AVffmpegWrapper> aVffmpegWrapper;
		QString path;
		int renderingTimer = -1;
		int imageWidth = 0;

		QByteArray videoFrame;
		QPixmap videoPixmap;

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
		void audioPlaying();
		void feedAudioOutput();

	// QObject interface
	protected:
		virtual void timerEvent(QTimerEvent* event) override;

	// QWidget interface
	protected:
		virtual void resizeEvent(QResizeEvent* event) override;
		virtual void paintEvent(QPaintEvent* event) override;
};

#endif // CAMERA_H
