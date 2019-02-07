#include "avfilecontext.h"

AVfileContext::~AVfileContext() {
	closeFile();
}

bool AVfileContext::openFile(const std::string& path, PlayingMode playingMode, int streamType) {
	std::unique_lock<std::mutex> lock(safeReplayMutex);
	bool allRight = false;
	if(avFormatContext || readingThreadIsRunning) {
		return false;
	}
	filePath = path;
	this->playingMode = playingMode;
	this->streamType = streamType;
	if(playingMode == REPEATE_AND_RECONNECT) {
		readingThreadIsRunning = true;
		readingThreadIsStopping = false;
		lock.unlock();
		readingThread = std::thread(&AVfileContext::reading, this);
		return true;
	}

	AVDictionary *stream_opts = nullptr;
	av_dict_set(&stream_opts, "rtsp_flags", "prefer_tcp", 0);
	av_dict_set(&stream_opts, "stimeout", "10000000", 0); // in microseconds
	if(stream_opts == nullptr) {
		return false;
	}
	avFormatContext = nullptr;
	int temp = 0;
	auto deleter = [&](int*) {
		if(!allRight) {
			videoDecoder.stop();
			audioDecoder.stop();
			videoStreamId = -1;
			audioStreamId = -1;
			if(avFormatContext) {avformat_close_input(&avFormatContext); avFormatContext = nullptr;}
		}
		if(stream_opts) {av_dict_free(&stream_opts);}
	};
	std::unique_ptr<int, decltype(deleter)> allCloser(&temp, deleter);


	if(avformat_open_input(&avFormatContext, filePath.c_str(), nullptr, &stream_opts) != 0) {
		return false;
	}

	if(avformat_find_stream_info(avFormatContext, nullptr) < 0) {
		return false;
	}

	AVCodec* avcodec = nullptr;
	videoStreamId = -1;
	if(streamType & VIDEO) {
		int ret = av_find_best_stream(avFormatContext, AVMEDIA_TYPE_VIDEO, -1, -1, &avcodec, 0);
		if(ret >= 0) {
			videoStreamId = ret;
			AVStream* vstrm = avFormatContext->streams[videoStreamId];

			AVCodecContext* videoCodecContext = avcodec_alloc_context3(nullptr);
			if(videoCodecContext == nullptr) {
				return false;
			}
			if(avcodec_parameters_to_context(videoCodecContext, vstrm->codecpar) < 0) {
				avcodec_free_context(&videoCodecContext);
				return false;
			}
			if(avcodec_open2(videoCodecContext, avcodec, nullptr) < 0) {
				avcodec_free_context(&videoCodecContext);
				return false;
			}
			videoDecoder.init();
			videoDecoder.setStreamAndCodecContext(vstrm, videoCodecContext);
		}
	}

	avcodec = nullptr;
	audioStreamId = -1;
	if(streamType & AUDIO) {
		int ret = av_find_best_stream(avFormatContext, AVMEDIA_TYPE_AUDIO, -1, -1, &avcodec, 0);
		if(ret >= 0) {
			audioStreamId = ret;
			AVStream* vstrm = avFormatContext->streams[audioStreamId];
			AVCodecContext* audioCodecContext = avcodec_alloc_context3(nullptr);
			if(audioCodecContext == nullptr) {
				return false;
			}
			if(avcodec_parameters_to_context(audioCodecContext, vstrm->codecpar) < 0) {
				avcodec_free_context(&audioCodecContext);
				return false;
			}
			if(avcodec_open2(audioCodecContext, avcodec, nullptr) < 0) {
				avcodec_free_context(&audioCodecContext);
				return false;
			}
			audioDecoder.init();
			audioDecoder.setStreamAndCodecContext(vstrm, audioCodecContext);
			if(audioCallback != nullptr) {
				audioPlayingThreadIsRunning = true;
				audioPlayingThreadIsStopping = false;
				audioPlayingThread = std::thread(&AVfileContext::audioPlaying, this);
			}
		}
	}
	allRight = true;
	return true;
}

void AVfileContext::closeFile() {
	std::lock_guard<std::mutex> lock(safeReplayMutex);
	if(audioPlayingThreadIsRunning) {
		audioPlayingThreadIsStopping = true;
		if(audioPlayingThread.joinable()) {
			audioPlayingThread.join();
		}
	}
	videoDecoder.stop();
	audioDecoder.stop();
	stopReading();
	if(avFormatContext) {
		avformat_close_input(&avFormatContext);
		avFormatContext = nullptr;
	}
}

int AVfileContext::getSourceVideoWidth() {
	return videoDecoder.getSourceWidth();
}

int AVfileContext::getSourceVideoHeigth() {
	return videoDecoder.getSourceHeigth();
}

int AVfileContext::getDestinationWidth() {
	return videoDecoder.getDestinationWidth();
}

int AVfileContext::getDestinationHeigth() {
	return videoDecoder.getSourceHeigth();
}

bool AVfileContext::setVideoConvertingParameters(AVPixelFormat dstFormat, int flags, int dstW, int dstH) {
	std::lock_guard<std::mutex> lock(safeReplayMutex);
	return videoDecoder.setConvertingParameters(dstFormat, flags, dstW, dstH);
}

bool AVfileContext::setAudioConvertingParameters(AVSampleFormat destSampleFormat, int64_t destChLayuot, int destSampleRate) {
	std::lock_guard<std::mutex> lock(safeReplayMutex);
	bool result = audioDecoder.setConvertingParameters(destSampleFormat, destChLayuot, destSampleRate);
	if(result) {
		if(audioPlayingThreadIsRunning) {
			audioPlayingThreadIsStopping = true;
			if(audioPlayingThread.joinable()) {
				audioPlayingThread.join();
			}
			std::lock_guard<std::mutex> lock(safeAudioCallbackMutex);
			if(audioCallback != nullptr) {
				audioPlayingThreadIsRunning = true;
				audioPlayingThreadIsStopping = false;
				audioPlayingThread = std::thread(&AVfileContext::audioPlaying, this);
			}
		}
	}
	return result;
}

void AVfileContext::setPlayingMode(AVfileContext::PlayingMode newPlayingMode) {
	playingMode = newPlayingMode;
}

void AVfileContext::setAudioCallback(std::function<void(uint8_t*, uint32_t, int&)> audioCallback, int32_t audioSamplesNum) {
	std::lock_guard<std::mutex> lock(safeReplayMutex);
	if(!audioPlayingThreadIsRunning) {
		if(audioCallback != nullptr) {
			this->audioCallback = audioCallback;
			this->audioSamplesNum = audioSamplesNum;
			if(audioDecoder.isReady()) {
				std::cout << "start play" << std::endl;
				audioPlayingThreadIsRunning = true;
				audioPlayingThreadIsStopping = false;
				audioPlayingThread = std::thread(&AVfileContext::audioPlaying, this);
			}
		}
	}else {
		if(audioCallback == nullptr) {
			audioPlayingThreadIsStopping = true;
			audioPlayingThread.join();
		}else {
			std::lock_guard<std::mutex> lock(safeAudioCallbackMutex);
			this->audioCallback = audioCallback;
			this->audioSamplesNum = audioSamplesNum;
		}
	}
}

bool AVfileContext::startReading() {
	std::lock_guard<std::mutex> lock(safeReplayMutex);
	if(readingThreadIsRunning || videoDecoder.isRunning() || audioDecoder.isRunning()) {
		return false;
	}
	if(!avFormatContext) {
		return false;
	}
	if(videoStreamId >= 0) {
		if(!videoDecoder.start()) {
			return false;
		}
	}
	if(audioStreamId >= 0) {
		if(!audioDecoder.start()) {
			videoDecoder.stop();
			return false;
		}
	}
	if(videoStreamId < 0 && audioStreamId < 0) {
		return false;
	}
	readingThreadIsRunning = true;
	readingThreadIsStopping = false;
	readingThread = std::thread(&AVfileContext::reading, this);
	return true;
}

void AVfileContext::stopReading() {
	if(readingThreadIsRunning) {
		readingThreadIsStopping = true;
		if(readingThread.joinable()) {
			readingThread.join();
		}
	}
}

bool AVfileContext::isReading() {
	return readingThreadIsRunning;
}

bool AVfileContext::isDecodingVideo() {
	return videoDecoder.isRunning();
}

bool AVfileContext::isDecodingAudio() {
	return audioDecoder.isRunning();
}

bool AVfileContext::hasVideoFrame() {
	return videoDecoder.hasData();
}

uint64_t AVfileContext::availableAudioData() {
	return audioDecoder.availableData();
}

bool AVfileContext::endOfFile() {
	if(isDecodingVideo() || isDecodingAudio() || hasVideoFrame() || availableAudioData() || isReading()) {
		return false;
	}
	return true;
}

bool AVfileContext::getVideoData(uint8_t** data, int* dataSize) {
	return videoDecoder.getData(&data[0], &dataSize[0]);
}

bool AVfileContext::getVideoData(uint8_t* data, int dataSize) {
	return videoDecoder.getData(&data[0], dataSize);
}

uint32_t AVfileContext::getAudioData(uint8_t* data, uint32_t dataSize) {
	uint32_t result = audioDecoder.getData(data, dataSize);
	if(result > 0) {
		videoDecoder.setAudioPts(audioDecoder.getLastPts(), audioDecoder.getLastPtsCheckTime(), audioDecoder.getRtspDifferencePts());
	}
	return result;
}

int AVfileContext::audioSampleRate() {
	return audioDecoder.getDestSampleRate();
}

int AVfileContext::audioChannels() {
	return audioDecoder.getDestChannels();
}

bool AVfileContext::hasVideoStream() {
	return videoDecoder.isReady();
}

bool AVfileContext::hasAudioStream() {
	return audioDecoder.isReady() && audioDecoder.availableData();
}

void AVfileContext::audioPlaying() {
	uint8_t* buffer = nullptr;
	int temp = 0;
	auto deleter = [&](int*){
		if(buffer) delete [] buffer;
		audioPlayingThreadIsRunning = false;
	};
	std::unique_ptr<int, decltype(deleter)> threadFinishIndicator(&temp, deleter);

	while(true) {
		if(audioDecoder.availableData()) {
			break;
		}else if(audioPlayingThreadIsStopping || !audioDecoder.isRunning()) {
			return;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
	uint32_t bytesPerSample = static_cast<uint32_t>(av_get_bytes_per_sample(audioDecoder.getDestSampleFormat()));
	uint32_t channels = static_cast<uint32_t>(audioDecoder.getDestChannels());
	uint32_t bufsize = static_cast<uint32_t>(audioSamplesNum) * bytesPerSample * channels;
	buffer = new uint8_t[bufsize];

	double sampleRate = static_cast<double>(audioDecoder.getDestSampleRate());
	int64_t callPeriod = static_cast<int64_t>(1000000.0 / (sampleRate / audioSamplesNum)) - 600;

	int64_t lastCallTime = av_gettime();
	int64_t cutPeriod = 0;
	while(!audioPlayingThreadIsStopping && audioDecoder.isRunning()) {
		int64_t now = av_gettime();
		while(now - lastCallTime < callPeriod) {
			now = av_gettime();
		}
		uint32_t result = getAudioData(buffer, static_cast<unsigned>(bufsize));

		if(result > 0) {
			std::unique_lock<std::mutex> locker(safeAudioCallbackMutex);
			int32_t oneStepWrited = 0;
			int32_t alreadyWrited = 0;
			audioCallback(buffer, result, oneStepWrited);
			while(oneStepWrited < static_cast<int32_t>(result)) {
				locker.unlock();
				if(oneStepWrited > -1) {
					result -= static_cast<uint32_t>(oneStepWrited);
					alreadyWrited += oneStepWrited;
				}

				oneStepWrited = 0;
				now = av_gettime();
				lastCallTime = now;
				int64_t newCallPeriod = static_cast<int64_t>(1000000.0 / (sampleRate / (static_cast<double>(result) / (bytesPerSample * channels)))) - 600;
				cutPeriod = newCallPeriod - 3000;
				if(cutPeriod >= 2000) {
					std::this_thread::sleep_for(std::chrono::microseconds(cutPeriod));
				}

				while(now - lastCallTime < newCallPeriod) {
					now = av_gettime();
				}
				locker.lock();
				if(audioPlayingThreadIsStopping || !audioDecoder.isRunning()) {
					return;
				}
				audioCallback(&buffer[alreadyWrited], result, oneStepWrited);
			}
			//now = av_gettime();
		}
		lastCallTime = now;
		cutPeriod = callPeriod - 3000;
		if(cutPeriod >= 2000) {
			std::this_thread::sleep_for(std::chrono::microseconds(cutPeriod));
		}
	}
}

void AVfileContext::reading() {
	AVPacket* packet = av_packet_alloc();
	int temp = 0;
	auto deleter = [&](int*){
		readingThreadIsRunning = false;
		av_packet_free(&packet);
		audioDecoder.fileFinished();
		videoDecoder.fileFinished();
	};
	std::unique_ptr<int, decltype(deleter)> threadFinishIndicator(&temp, deleter);

	if(avFormatContext == nullptr) {
		if(!fallHandle()) {
			return;
		}
	}

	while(!readingThreadIsStopping) {
		int result = av_read_frame(avFormatContext, packet);
		if(result == 0) {
			if(packet->stream_index == videoStreamId) {
				if(!videoDecoder.pushPacket(packet)) {
					av_packet_unref(packet);
					if(fallHandle()) {
						continue;
					}
					return;
				}
			}else if(packet->stream_index == audioStreamId) {
				if(!audioDecoder.pushPacket(packet)) {
					av_packet_unref(packet);
					if(fallHandle()) {
						continue;
					}
					return;
				}
			}
			av_packet_unref(packet);
		}else {
			if(!fallHandle()) {
				return;
			}
		}
	}
}

bool AVfileContext::fallHandle() {
	if(playingMode == NORMAL) {
		return false;
	}else {
		if(videoStreamId != -1) {
			while(videoDecoder.hasData()) {
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
			while(!readingThreadIsStopping) {
				std::unique_lock<std::mutex> lock(safeReplayMutex, std::try_to_lock);
				if(lock.owns_lock()) {
					videoDecoder.stop();
					break;
				}
			}
			if(readingThreadIsStopping) {
				return false;
			}
		}
		if(audioStreamId != -1) {
			while(audioDecoder.availableData() > 0) {
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
			while(!readingThreadIsStopping) {
				std::unique_lock<std::mutex> lock(safeReplayMutex, std::try_to_lock);
				if(lock.owns_lock()) {
					if(audioPlayingThreadIsRunning) {
						audioPlayingThreadIsStopping = true;
						if(audioPlayingThread.joinable()) {
							audioPlayingThread.join();
						}
					}
					audioDecoder.stop();
					break;
				}
			}
			if(readingThreadIsStopping) {
				return false;
			}
		}
		while(!repeat()) {
			if(readingThreadIsStopping) {
				return false;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		return true;
	}
}

bool AVfileContext::repeat() {
	std::unique_lock<std::mutex> lock(safeReplayMutex, std::try_to_lock);
	if(!lock.owns_lock()) {
		return false;
	}
	if(readingThreadIsStopping) {
		return false;
	}
	if(avFormatContext != nullptr) {
		avformat_close_input(&avFormatContext);
	}

	bool allRight = false;

	AVDictionary *stream_opts = nullptr;
	av_dict_set(&stream_opts, "rtsp_flags", "prefer_tcp", 0);
	av_dict_set(&stream_opts, "stimeout", "10000000", 0); // in microseconds
	if(stream_opts == nullptr) {
		return false;
	}
	int temp = 0;
	auto deleter = [&](int*){
		if(!allRight) {
			if(avFormatContext) {avformat_close_input(&avFormatContext); avFormatContext = nullptr;}
		}
		if(stream_opts) {av_dict_free(&stream_opts);}
	};
	std::unique_ptr<int, decltype(deleter)> allCloser(&temp, deleter);

	int resOpen = avformat_open_input(&avFormatContext, filePath.c_str(), nullptr, &stream_opts);
	if(resOpen != 0) {
		return false;
	}

	if(avformat_find_stream_info(avFormatContext, nullptr) < 0) {
		return false;
	}

	AVCodec* avcodec = nullptr;
	int videoStreamId = -1;
	if(streamType & VIDEO) {
		int ret = av_find_best_stream(avFormatContext, AVMEDIA_TYPE_VIDEO, -1, -1, &avcodec, 0);
		if(ret >= 0) {
			videoStreamId = ret;
			AVStream* vstrm = avFormatContext->streams[videoStreamId];

			AVCodecContext* videoCodecContext = avcodec_alloc_context3(nullptr);
			if(videoCodecContext == nullptr) {
				return false;
			}
			if(avcodec_parameters_to_context(videoCodecContext, vstrm->codecpar) < 0) {
				avcodec_free_context(&videoCodecContext);
				return false;
			}
			if(avcodec_open2(videoCodecContext, avcodec, nullptr) < 0) {
				avcodec_free_context(&videoCodecContext);
				return false;
			}
			videoDecoder.init();
			if(!videoDecoder.setStreamAndCodecContext(vstrm, videoCodecContext)) {
				avcodec_free_context(&videoCodecContext);
				return false;
			}
		}
	}

	avcodec = nullptr;
	int audioStreamId = -1;
	if(streamType & AUDIO) {
		int ret = av_find_best_stream(avFormatContext, AVMEDIA_TYPE_AUDIO, -1, -1, &avcodec, 0);
		if(ret >= 0) {
			audioStreamId = ret;
			AVStream* vstrm = avFormatContext->streams[audioStreamId];
			AVCodecContext* audioCodecContext = avcodec_alloc_context3(nullptr);
			if(audioCodecContext == nullptr) {
				return false;
			}
			if(avcodec_parameters_to_context(audioCodecContext, vstrm->codecpar) < 0) {
				avcodec_free_context(&audioCodecContext);
				return false;
			}
			if(avcodec_open2(audioCodecContext, avcodec, nullptr) < 0) {
				avcodec_free_context(&audioCodecContext);
				return false;
			}
			audioDecoder.init();
			if(!audioDecoder.setStreamAndCodecContext(vstrm, audioCodecContext)) {
				avcodec_free_context(&audioCodecContext);
				return false;
			}
			if(audioCallback != nullptr) {
				audioPlayingThreadIsRunning = true;
				audioPlayingThreadIsStopping = false;
				audioPlayingThread = std::thread(&AVfileContext::audioPlaying, this);
			}
		}
	}

	if(videoStreamId != -1) {
		videoDecoder.start();
	}
	this->videoStreamId = videoStreamId;

	if(audioStreamId != -1) {
		audioDecoder.start();
	}
	this->audioStreamId = audioStreamId;

	allRight = true;
	return true;
}
