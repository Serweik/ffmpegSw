#ifndef AVABSTACTDECODER_H
#define AVABSTACTDECODER_H

extern "C" {
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
}

#include <array>
#include <functional>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <memory>

#include "avitemcontainer.h"

class AVBaseDecoder {
	public:
		using AVPacketType = AVItemContainer<AVPacket,
											std::function<void(AVPacket*)>,
											std::function<void(AVPacket*)>,
											std::function<void(AVPacket*)>>;

		using AVFrameType = AVItemContainer<AVFrame,
											std::function<void(AVFrame*)>,
											std::function<void(AVFrame*)>,
											std::function<void(AVFrame*)>>;

		virtual ~AVBaseDecoder();
		void init();
		bool start();
		void stop();
		void fileFinished();
		bool isRunning();
		bool setStreamAndCodecContext(AVStream* newStream, AVCodecContext* newCodecContext);
		bool pushPacket(AVPacket* newPacket);
		bool isReady();

	protected:
		bool buffersInitialized = false;
		bool running = false;
		bool stopping = false;
		bool endOfFile = false;

		static const int packetsBufferSize = 40;
		std::array<AVPacketType, packetsBufferSize> packet;
		int packetWriteIndex = 0;
		int packetReadIndex = 0;
		std::mutex packetMutex;
		std::condition_variable packetCond;

		std::thread decodingThread;
		static const int framesBufferSize = 20;
		std::array<AVFrameType, framesBufferSize> frame;
		int frameWriteIndex = 0;
		int frameReadIndex = 0;
		std::mutex frameMutex;
		std::condition_variable frameCond;

		AVCodecContext* codecContext = nullptr;
		AVStream* stream = nullptr;

		void decoding();
		virtual bool convertFrame(AVFrame* dest, AVFrame* source) = 0;
		virtual void handleEndOfFile(std::unique_lock<std::mutex>& frameLocker) = 0;
		virtual void initPackepBuffer();
		virtual void initFrameBuffer();
};

#endif // AVABSTACTDECODER_H
