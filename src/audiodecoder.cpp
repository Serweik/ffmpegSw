#include "audiodecoder.h"

AudioDecoder::~AudioDecoder() {
	if(convertContext != nullptr) {
		swr_free(&convertContext);
		convertContext = nullptr;
	}
}

bool AudioDecoder::start() {
	frameDataGivenAway = 0;
	ftameDataPtrIndex = 0;
	return AVBaseDecoder::start();
}

void AudioDecoder::stop() {
	AVBaseDecoder::stop();
	if(convertContext != nullptr) {
		swr_free(&convertContext);
		convertContext = nullptr;
	}
	dataSize = 0;
}

uint32_t AudioDecoder::availableData() {
	return dataSize;
}

uint32_t AudioDecoder::getData(uint8_t* data, uint32_t requairedDataSize) {
	if(dataSize == 0) return 0;

	uint32_t givenSize = 0;
	std::unique_lock<std::mutex> frameLocker(frameMutex);
	if(stopping) return 0;

	AVFrame* decodedFrame = frame[static_cast<unsigned>(frameReadIndex)].getPtr();
	double pts = decodedFrame->pkt_dts;
	if(pts == AV_NOPTS_VALUE) {
		pts = decodedFrame->pts;
	}
	if(pts != AV_NOPTS_VALUE) {
		pts *= av_q2d(stream->time_base);
		if(lastPts == 0.0 && frameReadIndex == 0 && pts > 0.5) {
			rtspDifferencePts = pts;
		}else if(lastPts > pts) {
			rtspDifferencePts = 0;
		}
		lastPts = pts;
		lastPtsCheckTime = av_gettime();
	}

	while(frameReadIndex != frameWriteIndex) {
		decodedFrame = frame[static_cast<unsigned>(frameReadIndex)].getPtr();
		bool isEmpty = false;
		uint32_t receivedData = getDataFromFrame(decodedFrame, isEmpty, &data[givenSize], requairedDataSize);
		if(isEmpty) {
			frame[static_cast<unsigned>(frameReadIndex ++)].unrefPtr();
			if(static_cast<unsigned>(frameReadIndex) >= frame.size())
				frameReadIndex = 0;
		}
		givenSize += receivedData;
		if(receivedData == requairedDataSize) {
			break;
		}
		requairedDataSize -= receivedData;
	}
	dataSize -= static_cast<uint64_t>(givenSize);
	frameLocker.unlock();
	frameCond.notify_one();
	return givenSize;
}

double AudioDecoder::getLastPts() {
	return lastPts;
}

double AudioDecoder::getRtspDifferencePts() {
	return rtspDifferencePts;
}

int64_t AudioDecoder::getLastPtsCheckTime() {
	return lastPtsCheckTime;
}

bool AudioDecoder::setConvertingParameters(AVSampleFormat destSampleFormat, int64_t destChLayuot, int destSampleRate) {
	std::unique_lock<std::mutex> frameLocker(frameMutex);
	if(stopping) return false;
	SwrContext* newContext = nullptr;
	if(codecContext) {
		destChLayuot = destChLayuot == -1 ? static_cast<int64_t>(codecContext->channel_layout) : destChLayuot;
		destSampleRate = destSampleRate == -1 ? codecContext->sample_rate : destSampleRate;
		if(destChLayuot == 0) {
			destChLayuot = AV_CH_LAYOUT_MONO;
		}
		if(destChLayuot < 0 || destSampleRate <= 0 || destSampleFormat == AVSampleFormat::AV_SAMPLE_FMT_NONE
		   || codecContext->sample_rate <= 0 || codecContext->sample_fmt == AVSampleFormat::AV_SAMPLE_FMT_NONE) {
			destSample_rate = destSampleRate;
			destCh_layuot = destChLayuot;
			destSample_format = destSampleFormat;
			if(convertContext) {
				swr_close(convertContext);
				swr_free(&convertContext);
				convertContext = nullptr;
			}
			return true;
		}
		if(codecContext->channel_layout == 0) {
			codecContext->channel_layout = AV_CH_LAYOUT_MONO;
		}
		newContext = swr_alloc_set_opts(nullptr,
								destChLayuot,		 // out_ch_layout
								destSampleFormat,    // out_sample_fmt
								destSampleRate,      // out_sample_rate
								static_cast<int64_t>(codecContext->channel_layout),		// in_ch_layout
								codecContext->sample_fmt,			// in_sample_fmt
								codecContext->sample_rate,			// in_sample_rate
								0,                    // log_offset
								nullptr);             // log_ctx
		if(newContext) {
			swr_init(newContext);
		}
	}
	if(convertContext) {
		swr_close(convertContext);
		swr_free(&convertContext);
	}
	convertContext = newContext;
	if(convertContext == nullptr) {
		destSample_rate = destSampleRate;
		destCh_layuot = destChLayuot;
		destSample_format = destSampleFormat;
		return true;
	}

	if((destSample_rate != destSampleRate || destCh_layuot != destChLayuot || destSample_format != destSampleFormat)
	 ||(srcSample_rate != codecContext->sample_rate
		 || srcCh_layuot != static_cast<int64_t>(codecContext->channel_layout)
		 || srcSample_format != codecContext->sample_fmt)) {
		AVSampleFormat oldSample_format = destSample_format;
		int oldSample_rate = destSample_rate;
		int64_t oldCh_layuot = destCh_layuot;
		destSample_rate = destSampleRate;
		destCh_layuot = destChLayuot;
		destSample_format = destSampleFormat;
		if(static_cast<int64_t>(codecContext->channel_layout) == 0) {
			codecContext->channel_layout = AV_CH_LAYOUT_MONO;
		}
		reconvertAll(oldSample_format, oldSample_rate, oldCh_layuot);
	}
	srcSample_rate = codecContext->sample_rate;
	srcCh_layuot = static_cast<int64_t>(codecContext->channel_layout);
	srcSample_format = codecContext->sample_fmt;
	return true;
}

int AudioDecoder::getSrcSampleRate() {
	std::unique_lock<std::mutex> frameLocker(frameMutex);
	if(!codecContext) {
		return -1;
	}
	return codecContext->sample_rate;
}

int AudioDecoder::getSrcChannels() {
	std::unique_lock<std::mutex> frameLocker(frameMutex);
	if(!codecContext) {
		return -1;
	}
	return codecContext->channels;
}

AVSampleFormat AudioDecoder::getDestSampleFormat() {
	return destSample_format;
}

int AudioDecoder::getDestChannels() {
	return destCh_layuot > 0 ? av_get_channel_layout_nb_channels(static_cast<uint64_t>(destCh_layuot)) : 0;
}

int AudioDecoder::getDestSampleRate() {
	return destSample_rate;
}

uint32_t AudioDecoder::getDataFromFrame(AVFrame* decodedFrame, bool& isEmpty, uint8_t* data, uint32_t requairedDataSize) {
	uint32_t linesize = static_cast<uint32_t>(av_get_bytes_per_sample(static_cast<AVSampleFormat>(decodedFrame->format)) * decodedFrame->nb_samples);
	uint32_t fullSize = static_cast<uint32_t>(decodedFrame->channels) * linesize;
	uint32_t copyedSize = 0;
	if(!av_sample_fmt_is_planar(static_cast<AVSampleFormat>(decodedFrame->format))) {
		uint32_t copySize = std::min(requairedDataSize, fullSize - frameDataGivenAway);
		memcpy(&data[0], &decodedFrame->extended_data[0][frameDataGivenAway], copySize);
		copyedSize += copySize;
	}else {
		uint32_t indexInLastDataPtr = frameDataGivenAway < linesize ? frameDataGivenAway : frameDataGivenAway - (linesize * ftameDataPtrIndex);
		for(; ftameDataPtrIndex < static_cast<uint32_t>(decodedFrame->channels) && decodedFrame->extended_data[ftameDataPtrIndex] && requairedDataSize > 0; ++ ftameDataPtrIndex) {
			uint32_t copySize = std::min(requairedDataSize, linesize - indexInLastDataPtr);
			memcpy(&data[copyedSize], &decodedFrame->extended_data[ftameDataPtrIndex][indexInLastDataPtr], copySize);
			copyedSize += copySize;
			requairedDataSize -= copySize;
			indexInLastDataPtr = 0;
		}
	}
	frameDataGivenAway += copyedSize;
	if(frameDataGivenAway >= fullSize) {
		frameDataGivenAway = 0;
		ftameDataPtrIndex = 0;
		isEmpty = true;
	}
	return copyedSize;
}

bool AudioDecoder::convertFrame(AVFrame* dest, AVFrame* source) {
	if(convertContext != nullptr) {
		if(srcSample_rate != codecContext->sample_rate
		   || srcCh_layuot != static_cast<int64_t>(codecContext->channel_layout)
		   || srcSample_format != codecContext->sample_fmt) {
			swr_close(convertContext);
			swr_free(&convertContext);
			convertContext = nullptr;
		}
	}
	if(convertContext == nullptr) {
		if(codecContext->sample_rate > 0 && codecContext->sample_fmt != AVSampleFormat::AV_SAMPLE_FMT_NONE) {
			if(destSample_rate <= 0) {
				destSample_rate = codecContext->sample_rate;
			}
			if(codecContext->channel_layout == 0) {
				codecContext->channel_layout = AV_CH_LAYOUT_MONO;
			}
			if(destCh_layuot <= 0) {
				destCh_layuot = static_cast<int64_t>(codecContext->channel_layout);
			}
			if(destSample_format == AVSampleFormat::AV_SAMPLE_FMT_NONE) {
				destSample_format = codecContext->sample_fmt;
			}
			convertContext = swr_alloc_set_opts(nullptr,
									destCh_layuot,		 // out_ch_layout
									destSample_format,    // out_sample_fmt
									destSample_rate,      // out_sample_rate
									static_cast<int64_t>(codecContext->channel_layout),		// in_ch_layout
									codecContext->sample_fmt,			// in_sample_fmt
									codecContext->sample_rate,			// in_sample_rate
									0,                    // log_offset
									nullptr);             // log_ctx
			if(convertContext) {
				swr_init(convertContext);
			}else {
				return false;
			}
			srcSample_rate = codecContext->sample_rate;
			srcCh_layuot = static_cast<int64_t>(codecContext->channel_layout);
			srcSample_format = codecContext->sample_fmt;
			reconvertAll(AVSampleFormat::AV_SAMPLE_FMT_NONE, destSample_rate, destCh_layuot);
		}else {
			return false;
		}
	}
	dest->channel_layout = static_cast<uint64_t>(destCh_layuot);
	dest->sample_rate = destSample_rate;
	dest->format = destSample_format;
	dest->pkt_dts = source->pkt_dts;
	dest->pts = source->pts;
	if(swr_convert_frame(convertContext, dest, source) == 0) {
		return true;
	}else {
		return false;
	}
}

void AudioDecoder::reconvertAll(AVSampleFormat oldSample_format, int oldSample_rate, int64_t oldCh_layuot) {
	if(frameReadIndex != frameWriteIndex) {
		bool good = false;
		if(oldSample_format != AVSampleFormat::AV_SAMPLE_FMT_NONE) {
			SwrContext* tempContext = swr_alloc_set_opts(
													nullptr,
													destCh_layuot,		 // out_ch_layout
													destSample_format,    // out_sample_fmt
													destSample_rate,      // out_sample_rate
													oldCh_layuot,		// in_ch_layout
													oldSample_format,		// in_sample_fmt
													oldSample_rate,			// in_sample_rate
													0,                    // log_offset
													nullptr);             // log_ctx
			if(tempContext != nullptr) {
				int res = swr_init(tempContext);
				good = true;
				for(int isign = frameReadIndex; std::abs(isign - frameWriteIndex) > 0; ++ isign) { //rescale already decoded frames
					AVFrame* dest = av_frame_alloc();
					int temp = 0;
					auto deleter = [&](int*) {
						if(dest) av_frame_free(&dest);
					};
					std::unique_ptr<int, decltype(deleter)> frameDeleter(&temp, deleter);

					unsigned int i = static_cast<unsigned>(isign);
					dest->channel_layout = static_cast<uint64_t>(destCh_layuot);
					dest->sample_rate = destSample_rate;
					dest->format = destSample_format;
					dest->pkt_dts = frame[i].getPtr()->pkt_dts;
					dest->pts = frame[i].getPtr()->pts;
					res = swr_convert_frame(convertContext, dest, frame[i].getPtr());
					int linesize = av_get_bytes_per_sample(static_cast<AVSampleFormat>(frame[i].getPtr()->format)) * frame[i].getPtr()->nb_samples;
					dataSize -= static_cast<unsigned>(linesize * frame[i].getPtr()->channels) - frameDataGivenAway;
					frameDataGivenAway = 0;
					ftameDataPtrIndex = 0;
					frame[i].resetPtr();
					if(res == 0) {
						frame[i].setReferencedPtr(dest);
						frame[i].doSomething();
					}else {
						frame[i].setReferencedPtr(dest);
					}
					dest = nullptr;
					if(static_cast<unsigned>(isign) >= frame.size() - 1) {
						isign = -1;
					}
				}
				swr_close(tempContext);
				swr_free(&tempContext);
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
	}
}

void AudioDecoder::handleEndOfFile(std::unique_lock<std::mutex>& frameLocker) {
	if(swr_get_delay(convertContext, 1000) > 0) {
		if(stopping) return;

		if(((frameWriteIndex < frameReadIndex) && (frameReadIndex - frameWriteIndex < 6))//free space in the buffer is less then 6 items
		 ||((frameWriteIndex > frameReadIndex) && (frameWriteIndex - frameReadIndex > framesBufferSize - 6))) {//free space in the buffer is less then 6 items
			frameCond.wait(frameLocker, [&](){
				return (frameWriteIndex == frameReadIndex)
						|| (((frameWriteIndex < frameReadIndex) && (frameReadIndex - frameWriteIndex > 6)) //the buffer has free item
						 || ((frameWriteIndex > frameReadIndex) && (frameWriteIndex - frameReadIndex < framesBufferSize - 6))) //the buffer has free item
						|| stopping;});
			if(stopping) {
				return;
			}
		}

		if(convertFrame(frame[static_cast<unsigned>(frameWriteIndex)].getPtr(), nullptr)) {
			frame[static_cast<unsigned>(frameWriteIndex)].doSomething();
			frame[static_cast<unsigned>(frameWriteIndex ++)].markPtrHowReferenced();
			if(static_cast<unsigned>(frameWriteIndex) >= frame.size()) {
				frameWriteIndex = 0;
			}
		}
	}
}

void AudioDecoder::initFrameBuffer() {
	for(auto& frameContainer : frame) {
		frameContainer.setDeleter([](AVFrame* frame) {av_frame_free(&frame);});
		frameContainer.setUnreferencer([](AVFrame* frame) {av_frame_unref(frame);});
		frameContainer.setDoSomething([&](AVFrame* frame) {
			int linesize = av_get_bytes_per_sample(static_cast<AVSampleFormat>(frame->format)) * frame->nb_samples;
			dataSize += static_cast<unsigned>(linesize * frame->channels);
		});
		frameContainer.setUnreferencedPtr(av_frame_alloc());
	}
}
