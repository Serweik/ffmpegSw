#include "avffmpegwrapper.h"

AVffmpegWrapper::AVffmpegWrapper() {
	threadCounterDecrement = [&](int*) {
		-- threadCounter;
	};
}

AVffmpegWrapper::~AVffmpegWrapper() {
	std::lock_guard<std::mutex> locker(avFileMutex);
	while(threadCounter != 0) {
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
	avFiles.clear();
}

int AVffmpegWrapper::openFile(const std::string& path, AVfileContext::PlayingMode playingMode, int streamType) {
	std::lock_guard<std::mutex> locker(avFileMutex);
	while(threadCounter != 0) {
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
	int fileDescriptor = -1;
	std::unique_ptr<AVfileContext, std::function<void(AVfileContext*)>> fileContext(new AVfileContext, [](AVfileContext* fCtx) {
																						fCtx->closeFile();
																						delete fCtx;
																					});
	if(fileContext->openFile(path, playingMode, streamType)) {
		fileDescriptor = findEmptyDescriptor();
		avFiles.insert(std::make_pair(static_cast<unsigned int>(fileDescriptor), std::move(fileContext)));
	}

	return fileDescriptor;
}

void AVffmpegWrapper::closeFile(int fileDescriptor) {
	std::lock_guard<std::mutex> locker(avFileMutex);
	while(threadCounter != 0) {
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
	if(avFiles.find(fileDescriptor) != avFiles.end()) {
		avFiles.erase(fileDescriptor); //Deleter will invoke ptr->closeFile()
	}
}

int AVffmpegWrapper::getSourceVideoWidth(int fileDescriptor) {
	std::unique_lock<std::mutex> locker(avFileMutex);
	int temp = 0;
	++ threadCounter;
	std::unique_ptr<int, decltype(threadCounterDecrement)> decrementer(&temp, threadCounterDecrement);
	locker.unlock();
	int result = -1;
	if(avFiles.find(fileDescriptor) != avFiles.end()) {
		int width = avFiles[fileDescriptor]->getSourceVideoWidth();
		result = width;
	}
	return result;
}

int AVffmpegWrapper::getSourceVideoHeigth(int fileDescriptor) {
	std::unique_lock<std::mutex> locker(avFileMutex);
	int temp = 0;
	++ threadCounter;
	std::unique_ptr<int, decltype(threadCounterDecrement)> decrementer(&temp, threadCounterDecrement);
	locker.unlock();
	int result = -1;
	if(avFiles.find(fileDescriptor) != avFiles.end()) {
		int heigh = avFiles[fileDescriptor]->getSourceVideoHeigth();
		result = heigh;
	}
	return result;
}

int AVffmpegWrapper::getDestinationWidth(int fileDescriptor) {
	std::unique_lock<std::mutex> locker(avFileMutex);
	int temp = 0;
	++ threadCounter;
	std::unique_ptr<int, decltype(threadCounterDecrement)> decrementer(&temp, threadCounterDecrement);
	locker.unlock();
	int result = -1;
	if(avFiles.find(fileDescriptor) != avFiles.end()) {
		return avFiles[fileDescriptor]->getDestinationWidth();
	}
	return result;
}

int AVffmpegWrapper::getDestinationHeigth(int fileDescriptor) {
	std::unique_lock<std::mutex> locker(avFileMutex);
	int temp = 0;
	++ threadCounter;
	std::unique_ptr<int, decltype(threadCounterDecrement)> decrementer(&temp, threadCounterDecrement);
	locker.unlock();
	int result = -1;
	if(avFiles.find(fileDescriptor) != avFiles.end()) {
		return avFiles[fileDescriptor]->getDestinationHeigth();
	}
	return result;
}

bool AVffmpegWrapper::setVideoConvertingParameters(int fileDescriptor, AVPixelFormat dstFormat, int flags, int dstW, int dstH) {
	std::unique_lock<std::mutex> locker(avFileMutex);
	int temp = 0;
	++ threadCounter;
	std::unique_ptr<int, decltype(threadCounterDecrement)> decrementer(&temp, threadCounterDecrement);
	locker.unlock();
	bool result = false;
	if(avFiles.find(fileDescriptor) != avFiles.end()) {
		if(avFiles[fileDescriptor]->setVideoConvertingParameters(dstFormat, flags, dstW, dstH)) {
			result = true;
		}
	}
	return result;
}

bool AVffmpegWrapper::setAudioConvertingParameters(int fileDescriptor, AVSampleFormat destSampleFormat, int64_t destChLayuot, int destSampleRate) {
	std::unique_lock<std::mutex> locker(avFileMutex);
	int temp = 0;
	++ threadCounter;
	std::unique_ptr<int, decltype(threadCounterDecrement)> decrementer(&temp, threadCounterDecrement);
	locker.unlock();
	bool result = false;
	if(avFiles.find(fileDescriptor) != avFiles.end()) {
		if(avFiles[fileDescriptor]->setAudioConvertingParameters(destSampleFormat, destChLayuot, destSampleRate)) {
			result = true;
		}
	}
	return result;
}

void AVffmpegWrapper::setPlayingMode(int fileDescriptor, AVfileContext::PlayingMode newPlayingMode) {
	std::unique_lock<std::mutex> locker(avFileMutex);
	int temp = 0;
	++ threadCounter;
	std::unique_ptr<int, decltype(threadCounterDecrement)> decrementer(&temp, threadCounterDecrement);
	locker.unlock();
	if(avFiles.find(fileDescriptor) != avFiles.end()) {
		avFiles[fileDescriptor]->setPlayingMode(newPlayingMode);
	}
}

void AVffmpegWrapper::setAudioCallback(int fileDescriptor, std::function<void(uint8_t*, uint32_t, int&)> audioCallback, int32_t audioSamplesNum) {
	std::unique_lock<std::mutex> locker(avFileMutex);
	int temp = 0;
	++ threadCounter;
	std::unique_ptr<int, decltype(threadCounterDecrement)> decrementer(&temp, threadCounterDecrement);
	locker.unlock();
	if(avFiles.find(fileDescriptor) != avFiles.end()) {
		avFiles[fileDescriptor]->setAudioCallback(audioCallback, audioSamplesNum);
	}
}

bool AVffmpegWrapper::startReading(int fileDescriptor) {
	std::unique_lock<std::mutex> locker(avFileMutex);
	int temp = 0;
	++ threadCounter;
	std::unique_ptr<int, decltype(threadCounterDecrement)> decrementer(&temp, threadCounterDecrement);
	locker.unlock();
	bool result = false;
	if(avFiles.find(fileDescriptor) != avFiles.end()) {
		if(avFiles[fileDescriptor]->startReading()) {
			result = true;
		}
	}
	return result;
}

bool AVffmpegWrapper::isReading(int fileDescriptor) {
	std::unique_lock<std::mutex> locker(avFileMutex);
	int temp = 0;
	++ threadCounter;
	std::unique_ptr<int, decltype(threadCounterDecrement)> decrementer(&temp, threadCounterDecrement);
	locker.unlock();
	if(avFiles.find(fileDescriptor) != avFiles.end()) {
		return avFiles[fileDescriptor]->isReading();
	}
	return false;
}

bool AVffmpegWrapper::isDecodingVideo(int fileDescriptor) {
	std::unique_lock<std::mutex> locker(avFileMutex);
	int temp = 0;
	++ threadCounter;
	std::unique_ptr<int, decltype(threadCounterDecrement)> decrementer(&temp, threadCounterDecrement);
	locker.unlock();
	if(avFiles.find(fileDescriptor) != avFiles.end()) {
		return avFiles[fileDescriptor]->isDecodingVideo();
	}
	return false;
}

bool AVffmpegWrapper::isDecodingAudio(int fileDescriptor) {
	std::unique_lock<std::mutex> locker(avFileMutex);
	int temp = 0;
	++ threadCounter;
	std::unique_ptr<int, decltype(threadCounterDecrement)> decrementer(&temp, threadCounterDecrement);
	locker.unlock();
	if(avFiles.find(fileDescriptor) != avFiles.end()) {
		return avFiles[fileDescriptor]->isDecodingAudio();
	}
	return false;
}

bool AVffmpegWrapper::hasVideoFrame(int fileDescriptor) {
	std::unique_lock<std::mutex> locker(avFileMutex);
	int temp = 0;
	++ threadCounter;
	std::unique_ptr<int, decltype(threadCounterDecrement)> decrementer(&temp, threadCounterDecrement);
	locker.unlock();
	if(avFiles.find(fileDescriptor) != avFiles.end()) {
		return avFiles[fileDescriptor]->hasVideoFrame();
	}
	return false;
}

uint64_t AVffmpegWrapper::availableAudioData(int fileDescriptor) {
	std::unique_lock<std::mutex> locker(avFileMutex);
	int temp = 0;
	++ threadCounter;
	std::unique_ptr<int, decltype(threadCounterDecrement)> decrementer(&temp, threadCounterDecrement);
	locker.unlock();
	if(avFiles.find(fileDescriptor) != avFiles.end()) {
		return avFiles[fileDescriptor]->availableAudioData();
	}
	return false;
}

bool AVffmpegWrapper::endOfFile(int fileDescriptor) {
	std::unique_lock<std::mutex> locker(avFileMutex);
	int temp = 0;
	++ threadCounter;
	std::unique_ptr<int, decltype(threadCounterDecrement)> decrementer(&temp, threadCounterDecrement);
	locker.unlock();
	if(avFiles.find(fileDescriptor) != avFiles.end()) {
		return avFiles[fileDescriptor]->endOfFile();
	}
	return false;
}

bool AVffmpegWrapper::getVideoData(int fileDescriptor, uint8_t** data, int* dataSize) {
	std::unique_lock<std::mutex> locker(avFileMutex);
	int temp = 0;
	++ threadCounter;
	std::unique_ptr<int, decltype(threadCounterDecrement)> decrementer(&temp, threadCounterDecrement);
	locker.unlock();
	if(avFiles.find(fileDescriptor) != avFiles.end()) {
		return avFiles[fileDescriptor]->getVideoData(&data[0], &dataSize[0]);
	}else {
		return false;
	}
}

bool AVffmpegWrapper::getVideoData(int fileDescriptor, uint8_t* data, int dataSize) {
	std::unique_lock<std::mutex> locker(avFileMutex);
	int temp = 0;
	++ threadCounter;
	std::unique_ptr<int, decltype(threadCounterDecrement)> decrementer(&temp, threadCounterDecrement);
	locker.unlock();
	if(avFiles.find(fileDescriptor) != avFiles.end()) {
		return avFiles[fileDescriptor]->getVideoData(&data[0], dataSize);
	}else {
		return false;
	}
}

uint32_t AVffmpegWrapper::getAudioData(int fileDescriptor, uint8_t* targetBuffet, uint32_t dataSize) {
	std::unique_lock<std::mutex> locker(avFileMutex);
	int temp = 0;
	++ threadCounter;
	std::unique_ptr<int, decltype(threadCounterDecrement)> decrementer(&temp, threadCounterDecrement);
	locker.unlock();
	if(avFiles.find(fileDescriptor) != avFiles.end()) {
		return avFiles[fileDescriptor]->getAudioData(&targetBuffet[0], dataSize);
	}else {
		return 0;
	}
}

int AVffmpegWrapper::audioSampleRate(int fileDescriptor) {
	std::unique_lock<std::mutex> locker(avFileMutex);
	int temp = 0;
	++ threadCounter;
	std::unique_ptr<int, decltype(threadCounterDecrement)> decrementer(&temp, threadCounterDecrement);
	locker.unlock();
	if(avFiles.find(fileDescriptor) != avFiles.end()) {
		int sampleRate = avFiles[fileDescriptor]->audioSampleRate();
		if(sampleRate < 0) {
			return -1;
		}
		return sampleRate;
	}
	return -1;
}

int AVffmpegWrapper::audioChannels(int fileDescriptor) {
	std::unique_lock<std::mutex> locker(avFileMutex);
	int temp = 0;
	++ threadCounter;
	std::unique_ptr<int, decltype(threadCounterDecrement)> decrementer(&temp, threadCounterDecrement);
	locker.unlock();
	if(avFiles.find(fileDescriptor) != avFiles.end()) {
		int channels = avFiles[fileDescriptor]->audioChannels();
		if(channels < 0) {
			return -1;
		}
		return channels;
	}
	return -1;
}

bool AVffmpegWrapper::hasVideoStream(int fileDescriptor) {
	std::unique_lock<std::mutex> locker(avFileMutex);
	int temp = 0;
	++ threadCounter;
	std::unique_ptr<int, decltype(threadCounterDecrement)> decrementer(&temp, threadCounterDecrement);
	locker.unlock();
	if(avFiles.find(fileDescriptor) != avFiles.end()) {
		return avFiles[fileDescriptor]->hasVideoStream();
	}
	return -1;
}

bool AVffmpegWrapper::hasAudioStream(int fileDescriptor) {
	std::unique_lock<std::mutex> locker(avFileMutex);
	int temp = 0;
	++ threadCounter;
	std::unique_ptr<int, decltype(threadCounterDecrement)> decrementer(&temp, threadCounterDecrement);
	locker.unlock();
	if(avFiles.find(fileDescriptor) != avFiles.end()) {
		return avFiles[fileDescriptor]->hasAudioStream();
	}
	return -1;
}

int AVffmpegWrapper::findEmptyDescriptor() {
	for(int fileDescriptor = 0; fileDescriptor < std::numeric_limits<int>::max(); ++ fileDescriptor) {
		if(avFiles.find(fileDescriptor) == avFiles.end()) {
			return fileDescriptor;
		}
	}
	return -1;
}
