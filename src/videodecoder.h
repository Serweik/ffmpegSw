#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include "avabstactdecoder.h"

extern "C" {
	#include <libswscale/swscale.h>
	#include <libavutil/imgutils.h>
	#include <libavutil/time.h>
}

#include <algorithm>

class VideoDecoder: public AVBaseDecoder {
	public:
		virtual ~VideoDecoder() override;
		bool start();
		void stop();
		bool hasData();
		bool getData(uint8_t** data, int* linesize);
		bool getData(uint8_t* data, int dataSize);
		bool setConvertingParameters(AVPixelFormat dstFormat, int flags, int dstW = -1, int dstH = -1);
		void setAudioPts(double newAudioLastPts, int64_t checkTime, double newRtspDifferencePts);
		int getSourceWidth();
		int getSourceHeigth();
		int getDestinationWidth();
		int getDestinationHeigth();

	protected:
		SwsContext* convertContext = nullptr;
		AVPixelFormat srcPixFormat;
		int srcWidth = 0;
		int srcHeight = 0;
		AVPixelFormat destPixFormat;
		int destWidth = 0;
		int destHeight = 0;
		int convertFlags = 0;

		bool timeInitialized = false;
		int64_t startTime = 0;
		int64_t lastTime = 0;
		int64_t frameShowDelay = 0;
		double videoLastPts = 0.0;
		double videoRtspDiferencePts = 0.0;
		int lastFrameReadIndex = -1;

		double audioLastPts = 0.0;
		double audioRtspDiferencePts = 0.0;
		int64_t audioLastPtsCheckTime = 0;
		bool needSinchronyzeToAudio = false;
		bool stabilized = false;

		virtual bool convertFrame(AVFrame* dest, AVFrame* source) override;
		void reconvertAll(AVPixelFormat oldPixFormat, int oldWidth, int oldHeight);
		virtual void handleEndOfFile(std::unique_lock<std::mutex>& frameLocker) override;
		void initFrameBuffer() override;
		double getPts(AVFrame* decodedFrame);
};

#endif // VIDEODECODER_H
