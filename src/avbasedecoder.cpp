#include "avbasedecoder.h"

AVBaseDecoder::~AVBaseDecoder() {
	stop();
}

void AVBaseDecoder::init() {
	if(buffersInitialized) return;
	initPackepBuffer();
	initFrameBuffer();
	buffersInitialized = true;
}

bool AVBaseDecoder::start() {
	if(running || !codecContext || !stream){return false;}
	packetWriteIndex = packetReadIndex = 0;
	frameReadIndex = frameWriteIndex = 0;
	running = true;
	stopping = false;
	endOfFile = false;
	decodingThread = std::thread(&AVBaseDecoder::decoding, this);
	return true;
}

void AVBaseDecoder::stop() {
	std::unique_lock<std::mutex> packLocker(packetMutex);
	std::unique_lock<std::mutex> frameLocker(frameMutex);
	stopping = true;
	frameLocker.unlock();
	frameCond.notify_all();
	packLocker.unlock();
	packetCond.notify_all();
	if(decodingThread.joinable()) {
		decodingThread.join();
	}
	for(auto& packetContainer : packet) {
		packetContainer.unrefPtr();
	}
	for(auto& frameContainer : frame) {
		frameContainer.unrefPtr();
	}
	if(codecContext) {
		avcodec_flush_buffers(codecContext);
		avcodec_free_context(&codecContext);
		codecContext = nullptr;
	}
	packetWriteIndex = packetReadIndex = 0;
	frameReadIndex = frameWriteIndex = 0;
	stream = nullptr;
	stopping = false;
}

void AVBaseDecoder::fileFinished() {
	std::unique_lock<std::mutex> packLocker(packetMutex);
	if(stopping) return;
	endOfFile = true;
	packLocker.unlock();
	packetCond.notify_one();
}

bool AVBaseDecoder::isRunning() {
	return running;
}

bool AVBaseDecoder::setStreamAndCodecContext(AVStream* newStream, AVCodecContext* newCodecContext) {
	std::unique_lock<std::mutex> frameLocker(frameMutex);
	if(codecContext) {
		avcodec_flush_buffers(codecContext);
		avcodec_free_context(&codecContext);
		codecContext = nullptr;
	}
	stream = nullptr;
	codecContext = newCodecContext;
	stream = newStream;
	return true;
}

bool AVBaseDecoder::pushPacket(AVPacket* newPacket) {
	if(!running || stopping || endOfFile) return false;

	std::unique_lock<std::mutex> packLocker(packetMutex);
	if(stopping) return false;
	if(((packetWriteIndex < packetReadIndex) && (packetReadIndex - packetWriteIndex < 10)) //free space in the buffer is less then 6 items
	 ||((packetWriteIndex > packetReadIndex) && (packetWriteIndex - packetReadIndex > packetsBufferSize - 10))) { //free space in the buffer is less then 6 items
		packetCond.wait(packLocker, [&](){
			return (packetWriteIndex == packetReadIndex)
					|| (((packetWriteIndex < packetReadIndex) && (packetReadIndex - packetWriteIndex > 10)) //the buffer has free item
					 || ((packetWriteIndex > packetReadIndex) && (packetWriteIndex - packetReadIndex < packetsBufferSize - 10)))//the buffer has free item
					|| stopping;});
		if(stopping) {
			return false;
		}
	}
	av_packet_ref(packet[static_cast<unsigned>(packetWriteIndex)].getPtr(), newPacket);
	packet[static_cast<unsigned>(packetWriteIndex ++)].markPtrHowReferenced();
	if(static_cast<unsigned>(packetWriteIndex) >= packet.size()) {
		packetWriteIndex = 0;
	}
	packLocker.unlock();
	packetCond.notify_one();
	return true;
}

bool AVBaseDecoder::isReady() {
	return (codecContext && stream);
}

void AVBaseDecoder::decoding() {
	AVFrame* frameforDecoding = av_frame_alloc();
	int temp = 0;
	auto deleter = [&](int*) {
		running = false;
		av_frame_free(&frameforDecoding);
		if(!stopping) {
			stopping = true;
			packetCond.notify_one();
		}
	};
	std::unique_ptr<int, decltype(deleter)> threadFinishIndicator(&temp, deleter);

	while(!stopping) {
		std::unique_lock<std::mutex> packLocker(packetMutex);
		if(stopping) return;
		if(packetReadIndex == packetWriteIndex) {
			if(endOfFile) {
				return;
			}
			packetCond.wait(packLocker, [&](){
				return packetReadIndex != packetWriteIndex || stopping || endOfFile;
			});
			if(stopping || (endOfFile && (packetReadIndex == packetWriteIndex))) {
				return;
			}
		}
		AVPacket* srcPacket = packet[static_cast<unsigned>(packetReadIndex)].getPtr();

		std::unique_lock<std::mutex> frameLocker(frameMutex); // for protect codecContext
		int result = avcodec_send_packet(codecContext, srcPacket);
		packet[static_cast<unsigned>(packetReadIndex ++)].unrefPtr();
		if(static_cast<unsigned>(packetReadIndex) >= packet.size()) {
			packetReadIndex = 0;
		}
		packLocker.unlock();
		packetCond.notify_one();
		if(result != 0) {
			continue;
		}
		result = avcodec_receive_frame(codecContext, frameforDecoding);
		if(result == 0) {
			if(stopping) return;

			if(((frameWriteIndex < frameReadIndex) && (frameReadIndex - frameWriteIndex < 6))//free space in the buffer is less then 6 items
			 ||((frameWriteIndex > frameReadIndex) && (frameWriteIndex - frameReadIndex > framesBufferSize - 6))) {//free space in the buffer is less then 6 items
				frameCond.wait(frameLocker, [&](){
					return (frameWriteIndex == frameReadIndex)
							|| (((frameWriteIndex < frameReadIndex) && (frameReadIndex - frameWriteIndex > 6))//the buffer has free item
							 || ((frameWriteIndex > frameReadIndex) && (frameWriteIndex - frameReadIndex < framesBufferSize - 6)))//the buffer has free item
							|| stopping;});
				if(stopping) {
					return;
				}
			}
			if(convertFrame(frame[static_cast<unsigned>(frameWriteIndex)].getPtr(), frameforDecoding)) {
				if(frame[static_cast<unsigned>(frameWriteIndex)].hasDoSomething()) {
					frame[static_cast<unsigned>(frameWriteIndex)].doSomething();
				}
				frame[static_cast<unsigned>(frameWriteIndex ++)].markPtrHowReferenced();
				if(static_cast<unsigned>(frameWriteIndex) >= frame.size()) {
					frameWriteIndex = 0;
				}
				frameLocker.unlock();
			}
			av_frame_unref(frameforDecoding);
		}else if(result != AVERROR(EAGAIN)) {
			handleEndOfFile(frameLocker);
			break;
		}
	}
}

void AVBaseDecoder::initPackepBuffer() {
	for(auto& packetContainer : packet) {
		packetContainer.setDeleter([](AVPacket* packet) {av_packet_free(&packet);});
		packetContainer.setUnreferencer([](AVPacket* packet) {av_packet_unref(packet);});
		packetContainer.setUnreferencedPtr(av_packet_alloc());
	}
}

void AVBaseDecoder::initFrameBuffer() {
	for(auto& frameContainer : frame) {
		frameContainer.setDeleter([](AVFrame* frame) {av_frame_free(&frame);});
		frameContainer.setUnreferencer([](AVFrame* frame) {av_frame_unref(frame);});
		frameContainer.setUnreferencedPtr(av_frame_alloc());
	}
}
