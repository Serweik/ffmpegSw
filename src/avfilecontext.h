#ifndef AVFILECONTEXT_H
#define AVFILECONTEXT_H

#include "videodecoder.h"
#include "audiodecoder.h"

class AVfileContext {
	public:
		enum PlayingMode {
			NORMAL,
			REPEATE_AND_RECONNECT
		};

		enum StreamType {
			VIDEO = 1,
			AUDIO = 2,
		};

		AVfileContext() = default;
		AVfileContext(const AVfileContext& other) = delete;
		AVfileContext(AVfileContext&& other) = delete;
		AVfileContext& operator = (const AVfileContext& other) = delete;
		AVfileContext& operator = (AVfileContext&& other) = delete;

		~AVfileContext();
		bool openFile(const std::string& path, PlayingMode playingMode, int streamType);
		void closeFile();
		int getSourceVideoWidth();
		int getSourceVideoHeigth();
		int getDestinationWidth();
		int getDestinationHeigth();
		bool setVideoConvertingParameters(AVPixelFormat dstFormat, int flags, int dstW = -1, int dstH = -1);
		bool setAudioConvertingParameters(AVSampleFormat destSampleFormat, int64_t destChLayuot = -1, int destSampleRate = -1);
		void setPlayingMode(PlayingMode newPlayingMode);
		bool startReading();
		bool isReading();
		bool isDecodingVideo();
		bool isDecodingAudio();
		bool hasVideoFrame();
		uint64_t availableAudioData();
		bool endOfFile();
		bool getVideoData(uint8_t** data, int* dataSize);
		bool getVideoData(uint8_t* data, int dataSize);
		uint32_t getAudioData(uint8_t* data, uint32_t dataSize);
		int audioSampleRate();
		int audioChannels();
		bool hasVideoStream();
		bool hasAudioStream();

	private:
		std::string filePath;

		std::thread readingThread;
		bool readingThreadIsRunning = false;
		bool readingThreadIsStopping = false;
		std::mutex safeReplayMutex;

		AVFormatContext* avFormatContext = nullptr;
		int videoStreamId = -1;
		int audioStreamId = -1;

		VideoDecoder videoDecoder;
		AudioDecoder audioDecoder;

		PlayingMode playingMode = NORMAL;
		int streamType = VIDEO | AUDIO;

		void stopReading();
		void reading();
		bool fallHandle();
		bool repeat();
};

#endif // AVFILECONTEXT_H
