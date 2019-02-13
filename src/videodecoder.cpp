#include "videodecoder.h"

VideoDecoder::~VideoDecoder() {
	if(convertContext != nullptr) {
		sws_freeContext(convertContext);
		convertContext = nullptr;
	}
}

bool VideoDecoder::start() {
	timeInitialized = false;
	needSynchronizeToAudio = false;
	audioLastPts = 0.0;
	audioLastPtsCheckTime = 0;
	lastTime = 0;
	frameShowDelay = 0;
	lastFrameReadIndex = -1;
	videoRtspDiferencePts = 0.0;
	return AVBaseDecoder::start();
}

void VideoDecoder::stop() {
	AVBaseDecoder::stop();
	if(convertContext != nullptr) {
		sws_freeContext(convertContext);
		convertContext = nullptr;
	}
}

bool VideoDecoder::hasData() {
	return frameWriteIndex != frameReadIndex;
}

bool VideoDecoder::getData(uint8_t** data, int* linesize) {
	bool result = false;
	if(frameWriteIndex == frameReadIndex)
		return result;

	if(!timeInitialized) {
		timeInitialized = true;
		startTime = av_gettime();
	}
	int64_t now = av_gettime();
	if(now - lastTime >= frameShowDelay) {
		lastTime = now;
		std::unique_lock<std::mutex> frameLocker(frameMutex, std::try_to_lock);
		if(!frameLocker.owns_lock()) {
			while(!frameLocker.try_lock());
		}
		if(stopping) return false;
		AVFrame* decodedFrame = frame[static_cast<unsigned>(frameReadIndex)].getPtr();
		av_image_copy(&data[0], &linesize[0],
					  const_cast<const uint8_t**>(&decodedFrame->data[0]), &decodedFrame->linesize[0],
					  destPixFormat, destWidth, destHeight);

		++ frameReadIndex;
		if(static_cast<unsigned>(frameReadIndex) >= frame.size()) {
			frameReadIndex = 0;
		}
		double pts = getPts(decodedFrame);

		int64_t diffPts = static_cast<int64_t>(pts - videoLastPts);
		if(videoLastPts == 0.0 && diffPts > 0.5) {
			videoRtspDiferencePts = pts;
		}else if(diffPts < 0.0) {
			videoRtspDiferencePts = 0.0;
			startTime = now;
		}

		std::unique_lock<std::mutex> synchLocker(synchronizeMutex);
		if(needSynchronizeToAudio) {
			audioLastPts += static_cast<double>((now - audioLastPtsCheckTime)) / 1000000;
			audioLastPtsCheckTime = now;
			if(audioPtsIdUpdated) {
				audioLastPts -= audioRtspDiferencePts;
				audioPtsIdUpdated = false;
			}
			double diff = (audioLastPts - pts) * 1000000.0;
			if((diff >= -2000) && (diff <= 2000)) {
				stabilized = true;
			}else if((diff < -100000) || (diff > 100000)) {
				stabilized = false;
			}else if(stabilized) {
				diff = diff > 2000 ? 2000
								   : diff < -2000 ? -2000
												  : diff;
			}
			diffPts = static_cast<int64_t>(diff);
		}else {
			double lastTimePts = videoLastPts;
			double nextTime = lastTimePts + diffPts - videoRtspDiferencePts;
			double diff = (now - startTime) - (nextTime * 1000000);
			diffPts = static_cast<int64_t>(diff);
		}
		synchLocker.unlock();

		double frameDelay = av_q2d(codecContext->time_base);
		frameDelay = static_cast<double>(decodedFrame->repeat_pict) * (frameDelay * 0.5); //@TODO delay = repeat_pict / 2 * fps
		videoLastPts = pts;

		if(frameReadIndex != frameWriteIndex) {
			AVFrame* nextDecodedFrame = frame[static_cast<unsigned>(frameReadIndex)].getPtr();
			double nextPts = getPts(nextDecodedFrame);
			if(nextPts > videoLastPts) {
				int64_t nextDelay = static_cast<int64_t>((nextPts - videoLastPts) * 1000000.0);
				frameShowDelay = nextDelay - diffPts;
			}
		}else {
			frameShowDelay -= diffPts;
		}
		frameShowDelay += frameDelay * 1000000;
		if(frameShowDelay < 0) {
			frameShowDelay = 0;
		}else if(frameShowDelay > 3000000) {
			frameShowDelay = 42000;
		}

		frameLocker.unlock();
		frameCond.notify_one();
		result = true;
	}
	return result;
}

bool VideoDecoder::getData(uint8_t* data, int dataSize) {
	bool result = false;
	if(frameWriteIndex == frameReadIndex)
		return result;

	if(!timeInitialized) {
		timeInitialized = true;
		startTime = av_gettime();
	}
	int64_t now = av_gettime();
	if(now - lastTime >= frameShowDelay) {
		lastTime = now;
		std::unique_lock<std::mutex> frameLocker(frameMutex, std::try_to_lock);
		if(!frameLocker.owns_lock()) {
			while(!frameLocker.try_lock());
		}
		if(stopping) return false;
		AVFrame* decodedFrame = frame[static_cast<unsigned>(frameReadIndex)].getPtr();
		av_image_copy_to_buffer(&data[0], dataSize, const_cast<const uint8_t**>(&decodedFrame->data[0]), &decodedFrame->linesize[0], destPixFormat, destWidth, destHeight, 32);
		++ frameReadIndex;
		if(static_cast<unsigned>(frameReadIndex) >= frame.size()) {
			frameReadIndex = 0;
		}
		double pts = getPts(decodedFrame);

		int64_t diffPts = static_cast<int64_t>(pts - videoLastPts);
		if(videoLastPts == 0.0 && diffPts > 0.5) {
			videoRtspDiferencePts = pts;
		}else if(diffPts < 0.0) {
			videoRtspDiferencePts = 0.0;
			startTime = now;
		}

		std::unique_lock<std::mutex> synchLocker(synchronizeMutex);
		if(needSynchronizeToAudio) {
			audioLastPts += static_cast<double>((now - audioLastPtsCheckTime)) / 1000000;
			audioLastPtsCheckTime = now;
			if(audioPtsIdUpdated) {
				audioLastPts -= audioRtspDiferencePts;
				audioPtsIdUpdated = false;
			}
			double diff = (audioLastPts - pts) * 1000000.0;
			if((diff >= -2000) && (diff <= 2000)) {
				stabilized = true;
			}else if((diff < -100000) || (diff > 100000)) {
				stabilized = false;
			}else if(stabilized) {
				diff = diff > 2000 ? 2000
								   : diff < -2000 ? -2000
												  : diff;
			}
			diffPts = static_cast<int64_t>(diff);
		}else {
			double lastTimePts = videoLastPts;
			double nextTime = lastTimePts + diffPts - videoRtspDiferencePts;
			double diff = (now - startTime) - (nextTime * 1000000);
			diffPts = static_cast<int64_t>(diff);
		}
		synchLocker.unlock();

		double frameDelay = av_q2d(codecContext->time_base);
		frameDelay = static_cast<double>(decodedFrame->repeat_pict) * (frameDelay * 0.5); //@TODO delay = repeat_pict / 2 * fps
		videoLastPts = pts;

		if(frameReadIndex != frameWriteIndex) {
			AVFrame* nextDecodedFrame = frame[static_cast<unsigned>(frameReadIndex)].getPtr();
			double nextPts = getPts(nextDecodedFrame);
			if(nextPts > videoLastPts) {
				int64_t nextDelay = static_cast<int64_t>((nextPts - videoLastPts) * 1000000.0);
				frameShowDelay = nextDelay - diffPts;
			}
		}else {
			frameShowDelay -= diffPts;
		}
		frameShowDelay += frameDelay * 1000000;
		if(frameShowDelay < 0) {
			frameShowDelay = 0;
		}else if(frameShowDelay > 3000000) {
			frameShowDelay = 42000;
		}

		frameLocker.unlock();
		frameCond.notify_one();
		result = true;
	}
	return result;
}

bool VideoDecoder::setConvertingParameters(AVPixelFormat dstFormat, int flags, int dstW, int dstH) {
	std::unique_lock<std::mutex> frameLocker(frameMutex);
	if(stopping) return false;
	SwsContext* newContext = nullptr;
	if(codecContext) {
		dstW = dstW == -1 ? codecContext->width : dstW;
		dstH = dstH == -1 ? codecContext->height : dstH;
		while(dstW % 32 != 0) {dstW += 1;} //for alignment, else the image will have distortions
		if(dstW <= 0 || dstH <= 0 || codecContext->pix_fmt == AVPixelFormat::AV_PIX_FMT_NONE) {
			destPixFormat = dstFormat;
			destWidth = dstW;
			destHeight = dstH;
			convertFlags = flags;
			if(convertContext) {
				sws_freeContext(convertContext);
				convertContext = nullptr;
			}
			return true;
		}else {
			newContext = sws_getContext(
										codecContext->width, codecContext->height,
										codecContext->pix_fmt,
										dstW, dstH,
										dstFormat, flags,
										nullptr, nullptr, nullptr);
		}
	}


	if(convertContext) {
		sws_freeContext(convertContext);
	}
	convertContext = newContext;
	if(convertContext == nullptr) {
		destPixFormat = dstFormat;
		while(dstW % 32 != 0) {dstW += 1;} //for alignment, else the image will have distortions
		destWidth = dstW;
		destHeight = dstH;
		convertFlags = flags;
		return true;
	}

	if((destPixFormat != dstFormat || destWidth != dstW || destHeight != dstH)
	 ||(srcHeight != codecContext->height || srcWidth != codecContext->width || srcPixFormat != codecContext->pix_fmt)) {
		AVPixelFormat oldPixFormat = destPixFormat;
		int oldWidth = destWidth;
		int oldHeight = destHeight;
		destPixFormat = dstFormat;
		destWidth = dstW;
		destHeight = dstH;
		convertFlags = flags;
		reconvertAll(oldPixFormat, oldWidth, oldHeight);
	}
	srcHeight = codecContext->height;
	srcWidth = codecContext->width;
	srcPixFormat = codecContext->pix_fmt;
	return true;
}

void VideoDecoder::setAudioPts(double newAudioLastPts, int64_t checkTime, double newRtspDifferencePts) {
	std::unique_lock<std::mutex> synchLocker(synchronizeMutex);
	if(stopping) return;
	audioLastPts = newAudioLastPts;
	audioRtspDiferencePts = newRtspDifferencePts;
	audioLastPtsCheckTime = checkTime;
	needSynchronizeToAudio = true;
	audioPtsIdUpdated = true;
}

int VideoDecoder::getSourceWidth() {
	std::unique_lock<std::mutex> frameLocker(frameMutex);
	if(!codecContext) {
		return -1;
	}
	return codecContext->width;
}

int VideoDecoder::getSourceHeigth() {
	std::unique_lock<std::mutex> frameLocker(frameMutex);
	if(!codecContext) {
		return -1;
	}
	return codecContext->height;
}

int VideoDecoder::getDestinationWidth() {
	return destWidth;
}

int VideoDecoder::getDestinationHeigth() {
	return destHeight;
}

bool VideoDecoder::convertFrame(AVFrame* dest, AVFrame* source) {
	if(convertContext != nullptr) {
		if(srcHeight != codecContext->height || srcWidth != codecContext->width || srcPixFormat != codecContext->pix_fmt) {
			sws_freeContext(convertContext);
			convertContext = nullptr;
		}
	}
	if(convertContext == nullptr) {
		if(codecContext->width > 0 && codecContext->height > 0 && codecContext->pix_fmt != AVPixelFormat::AV_PIX_FMT_NONE) {
			if(destHeight <= 0) {
				destHeight = codecContext->height;
			}
			if(destWidth <= 0) {
				destWidth = codecContext->width;
			}
			while(destWidth % 32 != 0) {destWidth += 1;} //for alignment, else the image will have distortions
			convertContext = sws_getContext(
										codecContext->width, codecContext->height,
										codecContext->pix_fmt,
										destWidth, destHeight,
										destPixFormat, convertFlags,
										nullptr, nullptr, nullptr);
			if(convertContext == nullptr) {
				return false;
			}
			srcHeight = codecContext->height;
			srcWidth = codecContext->width;
			srcPixFormat = codecContext->pix_fmt;
			reconvertAll(AVPixelFormat::AV_PIX_FMT_NONE, srcWidth, srcHeight); //only for first initialization if we couldn't recieve codec's parameters at start!
		}else {
			return false;
		}
	}

	dest->format = destPixFormat;
	dest->width = destWidth;
	dest->height = destHeight;
	dest->pkt_dts = source->pkt_dts;
	dest->pts = source->pts;
	dest->repeat_pict = source->repeat_pict;
	if(sws_scale(convertContext, source->data, source->linesize, 0, codecContext->height, dest->data, dest->linesize) > 0) {
		return true;
	}else {
		return false;
	}
}

void VideoDecoder::reconvertAll(AVPixelFormat oldPixFormat, int oldWidth, int oldHeight) {
	if(frameReadIndex == frameWriteIndex) {
		for(unsigned int i = 0; i < frame.size(); ++ i) {
			frame[i].unrefPtr();
			av_image_alloc(frame[i].getPtr()->data, frame[i].getPtr()->linesize, destWidth, destHeight, destPixFormat, 32);
			frame[i].markPtrHowReferenced();
		}
	}else {
		bool good = false;
		if(oldPixFormat != AVPixelFormat::AV_PIX_FMT_NONE) {
			SwsContext* tempContext = sws_getContext(
												oldWidth, oldHeight,
												oldPixFormat,
												destWidth, destHeight,
												destPixFormat, convertFlags,
												nullptr, nullptr, nullptr);
			if(tempContext != nullptr) {
				good = true;
				for(int isign = frameReadIndex; std::abs(isign - frameWriteIndex) > 0; ++ isign) { //rescale already decoded frames
					AVFrame* dest = av_frame_alloc();
					av_image_alloc(dest->data, dest->linesize, destWidth, destHeight, destPixFormat, 32);
					int temp = 0;
					auto deleter = [&](int*) {
						if(dest) {
							av_freep(&dest->data[0]);
							av_frame_free(&dest);
						}
					};
					std::unique_ptr<int, decltype(deleter)> frameDeleter(&temp, deleter);

					unsigned int i = static_cast<unsigned>(isign);
					dest->format = destPixFormat;
					dest->width = destWidth;
					dest->height = destHeight;
					dest->pkt_dts = frame[i].getPtr()->pkt_dts;
					dest->pts = frame[i].getPtr()->pts;
					dest->repeat_pict = frame[i].getPtr()->repeat_pict;
					sws_scale(tempContext, frame[i].getPtr()->data, frame[i].getPtr()->linesize, 0, oldHeight, dest->data, dest->linesize);
					frame[i].resetPtr();
					frame[i].setReferencedPtr(dest);
					dest = nullptr;
					if(static_cast<unsigned>(isign) >= frame.size() - 1) {
						isign = -1;
					}
				}
				sws_freeContext(tempContext);
			}
		}
		if(!good) {
			for(int isign = frameReadIndex; std::abs(isign - frameWriteIndex) > 0; ++ isign) { //reset already decoded frames
				unsigned int i = static_cast<unsigned>(isign);
				frame[i].unrefPtr();
				if(static_cast<unsigned>(isign) >= frame.size() - 1) {
					isign = -1;
				}
			}
			frameReadIndex = frameWriteIndex;
		}

		for(int isign = frameWriteIndex; std::abs(isign - frameReadIndex) > 0; ++ isign) { //reallock other frames
			unsigned int i = static_cast<unsigned>(isign);
			frame[i].unrefPtr();
			av_image_alloc(frame[i].getPtr()->data, frame[i].getPtr()->linesize, destWidth, destHeight, destPixFormat, 32);
			frame[i].markPtrHowReferenced();
			if(static_cast<unsigned>(isign) >= frame.size() - 1) {
				isign = -1;
			}
		}
	}
}

void VideoDecoder::handleEndOfFile(std::unique_lock<std::mutex>&) {

}

void VideoDecoder::initFrameBuffer() {
	for(auto& frameContainer : frame) {
		frameContainer.setDeleter([](AVFrame* frame) {av_frame_free(&frame);});
		frameContainer.setUnreferencer([](AVFrame* frame) {av_freep(&frame->data[0]);});
		frameContainer.setUnreferencedPtr(av_frame_alloc());
	}
}

double VideoDecoder::getPts(AVFrame* decodedFrame) {
	double pts = decodedFrame->pts;
	if(pts == AV_NOPTS_VALUE) {
		pts = decodedFrame->pkt_dts;
	}
	if(pts == AV_NOPTS_VALUE) {
		pts = videoLastPts;
	}
	if(pts != 0.0) {
		pts *=  av_q2d(stream->time_base);
	}
	return pts;
}
