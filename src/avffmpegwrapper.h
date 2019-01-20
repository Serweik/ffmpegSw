#ifndef AVFFMPEGWRAPPER_H
#define AVFFMPEGWRAPPER_H

#include "avfilecontext.h"

#include <unordered_map>
#include <atomic>

class AVffmpegWrapper {
	public:
		AVffmpegWrapper();
		~AVffmpegWrapper();
		int openFile(const std::string& path, AVfileContext::PlayingMode playingMode, int streamType);
		void closeFile(int fileDescriptor);
		int getSourceVideoWidth(int fileDescriptor);
		int getSourceVideoHeigth(int fileDescriptor);
		int getDestinationWidth(int fileDescriptor);
		int getDestinationHeigth(int fileDescriptor);
		bool setVideoConvertingParameters(int fileDescriptor, enum AVPixelFormat dstFormat, int flags, int dstW = -1, int dstH = -1);
		bool setAudioConvertingParameters(int fileDescriptor, AVSampleFormat destSampleFormat, int64_t destChLayuot = -1, int destSampleRate = -1);
		void setPlayingMode(int fileDescriptor, AVfileContext::PlayingMode newPlayingMode);
		bool startReading(int fileDescriptor);
		bool isReading(int fileDescriptor);
		bool isDecodingVideo(int fileDescriptor);
		bool isDecodingAudio(int fileDescriptor);
		bool hasVideoFrame(int fileDescriptor);
		uint64_t availableAudioData(int fileDescriptor);
		bool endOfFile(int fileDescriptor);
		bool getVideoData(int fileDescriptor, uint8_t** data, int* dataSize);
		bool getVideoData(int fileDescriptor, uint8_t* data, int dataSize);
		uint32_t getAudioData(int fileDescriptor, uint8_t* targetBuffet, uint32_t dataSize);
		int audioSampleRate(int fileDescriptor);
		int audioChannels(int fileDescriptor);
		bool hasVideoStream(int fileDescriptor);
		bool hasAudioStream(int fileDescriptor);
	private:
		std::unordered_map<int, std::unique_ptr<AVfileContext, std::function<void(AVfileContext*)>>> avFiles;
		std::mutex avFileMutex;
		std::atomic<unsigned int> threadCounter = {0};
		std::function<void(int*)> threadCounterDecrement = nullptr;

		int findEmptyDescriptor();
};

#endif // AVFFMPEGWRAPPER_H
