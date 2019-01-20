#include <iostream>

#include "../../src/avffmpegwrapper.h"

#include "SDL.h"

#ifdef __MINGW32__
	#undef main
#endif

AVffmpegWrapper aVffmpegWrapper;

static void audio_callback(void* userdata, uint8_t* stream, int len) {
	if(len == 0) {
		return;
	}
	SDL_LockAudio();
	int* fileDescriptor = static_cast<int*>(userdata);
	uint8_t counter = 0;

	uint32_t result = aVffmpegWrapper.getAudioData(*fileDescriptor, &stream[0], static_cast<uint32_t>(len));
	len -= result;
	uint32_t dataWasGiven = result;
	while((len > 0) && (counter < 5)) {
		std::this_thread::sleep_for(std::chrono::microseconds(20));
		result -= aVffmpegWrapper.getAudioData(*fileDescriptor, &stream[0], static_cast<uint32_t>(len));
		len -= result;
		dataWasGiven += result;
		++ counter;
	}
	if(len > 0) {
		memset(&stream[dataWasGiven], 0, static_cast<size_t>(len));
	}

	SDL_UnlockAudio();
}

int main() {
	int width = 800;
	int height = 480;
	std::string path = "rtsp://184.72.239.149/vod/mp4:BigBuckBunny_115k.mov";
	int fileDescriptor = aVffmpegWrapper.openFile(path, AVfileContext::REPEATE_AND_RECONNECT, AVfileContext::VIDEO | AVfileContext::AUDIO);
	if(fileDescriptor == -1) {
		std::cout << "can't open file " << std::endl;
		return -1;
	}

	if(!aVffmpegWrapper.setVideoConvertingParameters(fileDescriptor, AV_PIX_FMT_YUV420P, SWS_FAST_BILINEAR, width, height)) {
		std::cout << "error setVideoConvertingParameters " << std::endl;
		return -1;
	}
	if(!aVffmpegWrapper.setAudioConvertingParameters(fileDescriptor, AV_SAMPLE_FMT_S16, AV_CH_LAYOUT_STEREO)) {
		std::cout << "error setAudioConvertingParameters " << std::endl;
		return -1;
	}

	if(SDL_Init(SDL_INIT_VIDEO) != 0) {
		std::cout << " Unable to init SDL: " << SDL_GetError() << std::endl;
		return -1;
	}

	SDL_Window* win = SDL_CreateWindow(path.c_str(), 100, 100, width, height, SDL_WINDOW_SHOWN);
	if(win == nullptr){
		std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
		return -1;
	}

	SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if(ren == nullptr){
		std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
		return -1;
	}

	SDL_Texture* texture = SDL_CreateTexture(
				ren,
				SDL_PIXELFORMAT_YV12,
				SDL_TEXTUREACCESS_STREAMING,
				width,
				height
			);
	if(texture == nullptr){
		std::cout << "SDL_CreateTextureFromSurface Error: " << SDL_GetError() << std::endl;
		return -1;
	}

	SDL_AudioSpec wanted_spec;
	// Set audio settings from codec info
	wanted_spec.freq = -1;
	wanted_spec.format = AUDIO_S16SYS;
	wanted_spec.channels = 2;
	wanted_spec.silence = 0;
	wanted_spec.samples = 1536;
	wanted_spec.callback = audio_callback;
	wanted_spec.userdata = &fileDescriptor;

	std::cout << "Started!" << std::endl;

	int linesize[3] = {width,
					   width / 2,
					   width / 2
					  };

	uint8_t** frameMap = new uint8_t*[3];
	frameMap[0] = new uint8_t[linesize[0] * height];
	frameMap[1] = new uint8_t[linesize[1] * height];
	frameMap[2] = new uint8_t[linesize[2] * height];

	while(!aVffmpegWrapper.endOfFile(fileDescriptor)) {
		if(wanted_spec.freq == -1 && aVffmpegWrapper.hasAudioStream(fileDescriptor)) {
			wanted_spec.freq = aVffmpegWrapper.audioSampleRate(fileDescriptor);
			if(SDL_OpenAudio(&wanted_spec, nullptr) < 0) {
			  std::cout << "SDL_OpenAudio Error: " << SDL_GetError() << std::endl;
			  return -1;
			}
			SDL_PauseAudio(0);
			std::cout << "audio ok!" << std::endl;
		}
		if(aVffmpegWrapper.getVideoData(fileDescriptor, &frameMap[0], &linesize[0])) {
			SDL_UpdateYUVTexture(
							texture,
							nullptr,
							frameMap[0],
							linesize[0],
							frameMap[1],
							linesize[1],
							frameMap[2],
							linesize[2]
						);
			SDL_RenderClear(ren);
			SDL_RenderCopy(ren, texture, nullptr, nullptr);
			SDL_RenderPresent(ren);
		}

		SDL_Event event;
		SDL_PollEvent(&event);
		switch(event.type) {
			case SDL_QUIT:
				SDL_PauseAudio(1);
				SDL_DestroyRenderer(ren);
				SDL_DestroyTexture(texture);
				SDL_DestroyWindow(win);
				std::cout << "sdl quit 1!" << std::endl;
				aVffmpegWrapper.closeFile(fileDescriptor);
				delete [] frameMap[2];
				delete [] frameMap[1];
				delete [] frameMap[0];
				delete [] frameMap;
				std::cout << "sdl quit!" << std::endl;
				SDL_Quit();
				return 0;
			default:
				break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
	}
	SDL_PauseAudio(1);
	SDL_DestroyRenderer(ren);
	SDL_DestroyTexture(texture);
	SDL_DestroyWindow(win);
	std::cout << "normal stop 1!" << std::endl;
	aVffmpegWrapper.closeFile(fileDescriptor);
	delete [] frameMap[2];
	delete [] frameMap[1];
	delete [] frameMap[0];
	delete [] frameMap;
	std::cout << "normal stop!" << std::endl;
	SDL_Quit();
	return 0;
}
