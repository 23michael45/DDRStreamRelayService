#ifndef __STREAM_PLAY_H_INCLUDED__
#define __STREAM_PLAY_H_INCLUDED__
#pragma once
#define __STDC_CONSTANT_MACROS

//#define USE_SDL

#ifdef USE_SDL
#pragma comment (lib, "lib/SDL2main.lib")
#endif

#ifdef _WIN32
//Windows
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "ao/ao.h"
#ifdef USE_SDL
#include "SDL2/SDL.h"
#endif
};
#else
//Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#ifdef __cplusplus
};
#endif
#endif
#include <mutex>
#include <condition_variable>
#include <chrono>
#include "../AVStream/MyMTModules.h"
using namespace std::chrono;
using namespace DDRMTLib;
struct StreamFrameInfo
{
	AVFormatContext	*pFormatCtx;
	AVCodecContext	*pVideoCodecCtx;
	AVCodecContext	*pAudioCodecCtx;
	AVCodec			*pVideoCodec;
	AVCodec			*pAudioCodec;
	AVPacket		*packet;
	AVFrame			*pFrame;
	AVFrame         *pFrameRGB;
	struct SwrContext *pAudioConvertCtx;
	struct SwsContext *pVideoConvertCtx;
};

struct SDLThreadInfo
{
	int threadExit;
	int threadPause;
	float frameDelayTimeMs;
};

struct Image_FFMPEG
{
	unsigned char* data;
	int step;
	int width;
	int height;
	int cn;
};

struct CallbackTimeoutInfo
{
	time_point<system_clock, system_clock::duration> timeBeforeBlocking;
	bool bTimeout;
};

class StreamPlay : public MainSubsModel
{
public:
	StreamPlay();
	~StreamPlay();
	bool Init(const char* fileName);
	void SetPlayUrl(std::string url);
	// 1 - audio; 2 - video; 0 - invalid.
	int GrabFrame();
	bool RetrieveFrame(unsigned char** data, 
		               int* step, 
					   int* width, 
					   int* height, 
					   int* cn);
	bool RetrieveAudioFrame();
	void PlayAudio();

	void Start();
	void Stop(); // quit the playing loop.

	bool Respond();

	void TestSync();
	void TestMultiThread();
	bool SetMute(bool bIsMute);

protected:
	bool _onRunOnce_Sub(int) override;

private:
	StreamFrameInfo mStreamFrameInfo;
	char mInUrlName[256];
	int mAudioStream, mVideoStream;
	uint8_t	*mpOutBuffer;
	int mOutBufferSize;
	ao_device *mDevice;

	CallbackTimeoutInfo timeoutInfo;
	Image_FFMPEG frame;

	std::mutex m_mutexGetMute;
	bool m_bIsMute;

#ifdef USE_SDL
	SDL_AudioSpec mWantedSpec;
	SDL_Renderer* mSDLRenderer;
	SDL_Texture* mSDLTexture;
	//SDL_Rect mSDLRect;
	SDL_Event mEvent;
#endif
	bool bStop;

	//static int mThreadExit;
	//static int mThreadPause;
	SDLThreadInfo mSDLThreadInfo;
	

	//time_point<system_clock, system_clock::duration> mTimeBeforeBlocking;

	std::condition_variable mcvbAudioStart;
	std::mutex mMutexAudioStart;

	bool Init();
	bool InitAudioStream();
	bool InitVideoStream();
	bool DecodeAudio();
	bool VideoThread();
	void AudioThread();

	void DecodeVideo(AVCodecContext *decCtx, AVFrame *frame, AVPacket *pkt);
	static int VideoRefreshThread(void *opaque);
	void Quit();
};
#endif //__STREAM_PLAY_H_INCLUDED__