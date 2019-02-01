#ifndef AUDIODECODER_H
#define AUDIODECODER_H

#include "avbasedecoder.h"

extern "C" {
	#include <libswresample/swresample.h>
	#include <libavutil/time.h>
}

#include <algorithm>

class AudioDecoder: public AVBaseDecoder {
	public:
		virtual ~AudioDecoder() override;
		bool start();
		void stop();
		uint32_t availableData();
		uint32_t getData(uint8_t* data, uint32_t requairedDataSize);
		double getLastPts();
		double getRtspDifferencePts();
		int64_t getLastPtsCheckTime();
		bool setConvertingParameters(AVSampleFormat destSampleFormat, int64_t destChLayuot = -1, int destSampleRate = -1);
		int getSrcSampleRate();
		int getSrcChannels();
		AVSampleFormat getDestSampleFormat();
		int getDestChannels();
		int getDestSampleRate();

	protected:
		SwrContext* convertContext = nullptr;
		int srcSample_rate = 0;
		int64_t srcCh_layuot = 0;
		AVSampleFormat srcSample_format;
		int destSample_rate = 0;
		int64_t destCh_layuot = 0;
		AVSampleFormat destSample_format;

		uint32_t dataSize = 0;

		double lastPts = 0.0;
		double rtspDifferencePts = 0.0;
		int64_t lastPtsCheckTime = 0;

		uint32_t frameDataGivenAway = 0;
		uint32_t ftameDataPtrIndex = 0;

		uint32_t getDataFromFrame(AVFrame* decodedFrame, bool& isEmpty, uint8_t* data, uint32_t requairedDataSize);
		virtual bool convertFrame(AVFrame* dest, AVFrame* source) override;
		void reconvertAll(AVSampleFormat oldSample_format, int oldSample_rate, int64_t oldCh_layuot);
		virtual void handleEndOfFile(std::unique_lock<std::mutex>& frameLocker) override;
		void initFrameBuffer() override;
};

#endif // AUDIODECODER_H
