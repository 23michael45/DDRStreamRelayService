#ifndef __AUDIO_PLAY_H_INCLUDED__
#define __AUDIO_PLAY_H_INCLUDED__
#pragma once
#define __STDC_CONSTANT_MACROS

#ifdef _WIN32
//Windows
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include "ao/ao.h"
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
#include "MyMTModules.h"
#include <chrono>
using namespace DDRMTLib;
using namespace std::chrono;
struct AudioFrameInfo
{
	AVFormatContext	*pFormatCtx;
	AVCodecContext	*pCodecCtx;
	AVCodec			*pCodec;
	AVPacket		*packet;
	AVFrame			*pFrame;
	struct SwrContext *pAudioConvertCtx;
};

class AudioPlay : public MainSubsModel
{
public:
	AudioPlay();
	~AudioPlay();
	bool Init(const char* fileName);
	void Start();
	void Stop(); // quit the playing loop.

	bool Respond();
	void QuitSubThreads();

protected:
	bool _onRunOnce_Sub(int) override;
	
private:
	AudioFrameInfo mAudioFrameInfo;
	const char* mInUrlName;
	int mAudioStream;
	uint8_t	*mpOutBuffer;
	int mOutBufferSize;
	ao_device *mDevice;
	bool bStop;

	time_point<system_clock, system_clock::duration> mTimeBeforeBlocking;

	void Quit();
};
#endif //__AUDIO_PLAY_H_INCLUDED__