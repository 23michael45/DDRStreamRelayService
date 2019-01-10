#pragma warning(disable :4996)
#include "DDRStreamPlay.h"
#include <thread>

#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio
#define LIBAVFORMAT_INTERRUPT_TIMEOUT_SECOND 30.f

// Audio play: libao or SDL
#define USE_LIBAO 1
#define USE_SDL_AUDIO 0


//Refresh Event
#define SFM_REFRESH_EVENT  (SDL_USEREVENT + 1)

#define SFM_BREAK_EVENT  (SDL_USEREVENT + 2)

#define high_resolution_clock system_clock  // xcy add. In order to compile at vs2017

StreamPlay::StreamPlay()
	: MainSubsModel(1, 0, 100)
{
	m_bIsMute = false;
}

StreamPlay::~StreamPlay()
{
	EndSubThreads();
	//Quit();
}

/* The audio function callback takes the following parameters:
* stream: A pointer to the audio buffer to be filled
* len: The length (in bytes) of the audio buffer
*/

#ifdef USE_SDL
static  Uint8  *gAudioChunk;
static  Uint32  gAudioLen;
static  Uint8  *gAudioPos;
void  FillAudioCallback(void *udata, Uint8 *stream, int len){
	//SDL 2.0
	SDL_memset(stream, 0, len);
	if (gAudioLen == 0)		/*  Only  play  if  we  have  data  left  */
		return;
	len = (len>gAudioLen ? gAudioLen : len);	/*  Mix  as  much  data  as  possible  */

	SDL_MixAudio(stream, gAudioPos, len, SDL_MIX_MAXVOLUME);
	gAudioPos += len;
	gAudioLen -= len;
}
#endif

int StreamInterruptCallback(void *ctx)
{
	CallbackTimeoutInfo* pTimeOutInfo = (CallbackTimeoutInfo*)ctx;
	auto curTime = high_resolution_clock::now();
	if (duration_cast<duration<float>>(curTime - pTimeOutInfo->timeBeforeBlocking).count() > LIBAVFORMAT_INTERRUPT_TIMEOUT_SECOND)
	{
		//printf("abort%f\n", duration_cast<duration<float>>(curTime - pTimeOutInfo->timeBeforeBlocking).count());
		pTimeOutInfo->bTimeout = true;
		return 1;
	}
	else
	{
		//printf("normal..................................................%f\n", duration_cast<duration<float>>(curTime - pTimeOutInfo->timeBeforeBlocking).count());
		pTimeOutInfo->bTimeout = false;
		return 0;
	}
}

//int gThreadExit = 0;
//int gThreadPause = 0;
int StreamPlay::VideoRefreshThread(void *opaque)
{
	SDLThreadInfo* threadInfo = (SDLThreadInfo*)opaque;
	threadInfo->threadExit = 0;
	threadInfo->threadPause = 0;
#ifdef USE_SDL
	while (!threadInfo->threadExit) {
		if (!threadInfo->threadPause){
			SDL_Event event;
			event.type = SFM_REFRESH_EVENT;
			SDL_PushEvent(&event);
		}
		//printf("VideoRefreshThread\n");
		SDL_Delay(threadInfo->frameDelayTimeMs);// 1000.0 / 23.98);
	}
	threadInfo->threadExit = 0;
	threadInfo->threadPause = 0;
	//Break
	SDL_Event event;
	event.type = SFM_BREAK_EVENT;
	SDL_PushEvent(&event);
#endif
	return 0;
}

bool StreamPlay::Init()
{
	mStreamFrameInfo.pFormatCtx = avformat_alloc_context(); //NULL;
	//Open
	AVDictionary *optionsDict = NULL;
	av_dict_set(&optionsDict, "stimeout", "2000000", 0);
	av_dict_set(&optionsDict, "tune", "zerolatency", 0);
	//av_dict_set(&optionsDict, "probesize", "32", 0);
	av_dict_set(&optionsDict, "fflags", "nobuffer", 0);
	av_dict_set(&optionsDict, "max_analyze_duration", "1000", 0);
	//av_dict_set(&optionsDict, "rtbufsize", "10000000", 0);
	//av_dict_set(&optionsDict, "timeout", "1000", 0);
	mStreamFrameInfo.pFormatCtx->interrupt_callback.callback = StreamInterruptCallback;
	mStreamFrameInfo.pFormatCtx->interrupt_callback.opaque = &timeoutInfo;
	timeoutInfo.timeBeforeBlocking = high_resolution_clock::now();
	timeoutInfo.bTimeout = false;
	if (avformat_open_input(&mStreamFrameInfo.pFormatCtx, mInUrlName, NULL, &optionsDict) != 0){
		printf("StreamPlay@Init(): Couldn't open input stream.\n");
		return false;
	}
	// Retrieve stream information
	if (avformat_find_stream_info(mStreamFrameInfo.pFormatCtx, NULL)<0){
		printf("StreamPlay@Init(): Couldn't find stream information.\n");
		return false;
	}
	// Dump valid information onto standard error
	//av_dump_format(mStreamFrameInfo.pFormatCtx, 0, mInUrlName, false);
	// Find the first audio stream
	mAudioStream = -1;
	mVideoStream = -1;
	for (int i = 0; i < mStreamFrameInfo.pFormatCtx->nb_streams; i++){
		if (mStreamFrameInfo.pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
			mAudioStream = i;
		}
		else if (mStreamFrameInfo.pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
			mVideoStream = i;
		}
	}

	if (mAudioStream == -1 && mVideoStream == -1){
		printf("StreamPlay@Init(): Didn't find a stream (audio or video).\n");
		return false;
	}

	mStreamFrameInfo.packet = (AVPacket *)av_malloc(sizeof(AVPacket));
	av_init_packet(mStreamFrameInfo.packet);

	mStreamFrameInfo.pFrame = av_frame_alloc();
	if (!mStreamFrameInfo.pFrame) {
		printf("StreamPlay@Init(): Could not allocate frame.\n");
		return false;
	}

#ifdef USE_SDL
	//Init
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		printf("StreamPlay@Init(): Could not initialize SDL - %s\n", SDL_GetError());
		return false;
	}
#endif
	mSDLThreadInfo.frameDelayTimeMs = 0;
	// Init Audio Stream
	if (mAudioStream >= 0)
	{
		if (!InitAudioStream())
			return false;
	}

	// Init Video Stream
	if (mVideoStream >= 0)
	{
		if (!InitVideoStream())
			return false;
	}

	bStop = false;
	return true;
}

bool StreamPlay::Init(const char* fileName)
{
	memset(mInUrlName, 0, 256);
	strcpy(mInUrlName, fileName);
#if 0 // test init alone
	mStreamFrameInfo.pFormatCtx = avformat_alloc_context(); //NULL;
	//Open
	AVDictionary *optionsDict = NULL;
	av_dict_set(&optionsDict, "stimeout", "2000000", 0);
	av_dict_set(&optionsDict, "tune", "zerolatency", 0);
	//av_dict_set(&optionsDict, "probesize", "32", 0);
	av_dict_set(&optionsDict, "fflags", "nobuffer", 0);
	av_dict_set(&optionsDict, "max_analyze_duration", "1000", 0);
	//av_dict_set(&optionsDict, "rtbufsize", "10000000", 0);
	//av_dict_set(&optionsDict, "timeout", "1000", 0);
	mStreamFrameInfo.pFormatCtx->interrupt_callback.callback = StreamInterruptCallback;
	mStreamFrameInfo.pFormatCtx->interrupt_callback.opaque = &timeoutInfo;
	timeoutInfo.timeBeforeBlocking = high_resolution_clock::now();
	timeoutInfo.bTimeout = false;
	if (avformat_open_input(&mStreamFrameInfo.pFormatCtx, mInUrlName, NULL, &optionsDict) != 0){
		printf("StreamPlay@Init(): Couldn't open input stream.\n");
		return false;
	}
	// Retrieve stream information
	if (avformat_find_stream_info(mStreamFrameInfo.pFormatCtx, NULL)<0){
		printf("StreamPlay@Init(): Couldn't find stream information.\n");
		return false;
	}
	// Dump valid information onto standard error
	av_dump_format(mStreamFrameInfo.pFormatCtx, 0, mInUrlName, false);
	// Find the first audio stream
	mAudioStream = -1;
	mVideoStream = -1;
	for (int i = 0; i < mStreamFrameInfo.pFormatCtx->nb_streams; i++){
		if (mStreamFrameInfo.pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
			mAudioStream = i;
		}
		else if (mStreamFrameInfo.pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
			mVideoStream = i;
		}
	}

	if (mAudioStream == -1 && mVideoStream == -1){
		printf("StreamPlay@Init(): Didn't find a stream (audio or video).\n");
		return false;
	}

	mStreamFrameInfo.packet = (AVPacket *)av_malloc(sizeof(AVPacket));
	av_init_packet(mStreamFrameInfo.packet);

	mStreamFrameInfo.pFrame = av_frame_alloc();
	if (!mStreamFrameInfo.pFrame) {
		printf("StreamPlay@Init(): Could not allocate frame.\n");
		return false;
	}

#ifdef USE_SDL
	//Init
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		printf("StreamPlay@Init(): Could not initialize SDL - %s\n", SDL_GetError());
		return false;
	}
#endif
	mSDLThreadInfo.frameDelayTimeMs = 0;
	// Init Audio Stream
	if (mAudioStream >= 0)
	{
		if (!InitAudioStream())
			return false;
	}

	// Init Video Stream
	if (mVideoStream >= 0)
	{
		if (!InitVideoStream())
			return false;
	}

	bStop = false;
#endif
	return true;
}

void StreamPlay::SetPlayUrl(std::string url)
{
	memset(mInUrlName, 0, 256);
	strcpy(mInUrlName, url.c_str());
}

bool StreamPlay::InitAudioStream()
{
	// Get a pointer to the codec context for the audio stream
	mStreamFrameInfo.pAudioCodecCtx = mStreamFrameInfo.pFormatCtx->streams[mAudioStream]->codec;

	// Find the decoder for the audio stream
	mStreamFrameInfo.pAudioCodec = avcodec_find_decoder(mStreamFrameInfo.pAudioCodecCtx->codec_id);
	if (mStreamFrameInfo.pAudioCodec == NULL){
		printf("StreamPlay@Init(): Audio Codec not found.\n");
		return false;
	}

	// Open codec
	if (avcodec_open2(mStreamFrameInfo.pAudioCodecCtx, mStreamFrameInfo.pAudioCodec, NULL) < 0){
		printf("StreamPlay@Init(): Could not open Audio codec.\n");
		return false;
	}



	//Out Audio Param
	uint64_t outChannelLayout = AV_CH_LAYOUT_STEREO;
	//nb_samples: AAC-1024 MP3-1152
	int outNbSamples = mStreamFrameInfo.pAudioCodecCtx->frame_size;
	AVSampleFormat outSampleFmt = AV_SAMPLE_FMT_S16;// mAudioFrameInfo.pCodecCtx->sample_fmt; //
	/*int outSampleRate = 44100;
	if (mStreamFrameInfo.pAudioCodecCtx->sample_rate > 44100)
		outSampleRate = mStreamFrameInfo.pAudioCodecCtx->sample_rate;*/
	int outSampleRate = mStreamFrameInfo.pAudioCodecCtx->sample_rate > 44100 ? mStreamFrameInfo.pAudioCodecCtx->sample_rate : 44100; //mAudioFrameInfo.pCodecCtx->sample_rate; //44100
	int outChannels = av_get_channel_layout_nb_channels(outChannelLayout);
	//Out Buffer Size
	mOutBufferSize = av_samples_get_buffer_size(NULL, outChannels, outNbSamples, outSampleFmt, 1);

	mpOutBuffer = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE * 2);
	//mStreamFrameInfo.pFrame = av_frame_alloc();

	//FIX:Some Codec's Context Information is missing
	int64_t inChannelLayout = av_get_default_channel_layout(mStreamFrameInfo.pAudioCodecCtx->channels);
	//Swr
	mStreamFrameInfo.pAudioConvertCtx = swr_alloc();
	mStreamFrameInfo.pAudioConvertCtx = swr_alloc_set_opts(mStreamFrameInfo.pAudioConvertCtx, outChannelLayout, outSampleFmt, outSampleRate,
		inChannelLayout, mStreamFrameInfo.pAudioCodecCtx->sample_fmt, mStreamFrameInfo.pAudioCodecCtx->sample_rate, 0, NULL);
	swr_init(mStreamFrameInfo.pAudioConvertCtx);

#if USE_LIBAO
	ao_initialize();
	int driver = ao_default_driver_id();
	ao_sample_format sformat;

	if (outSampleFmt == AV_SAMPLE_FMT_U8){
		printf("U8\n");

		sformat.bits = 8;
	}
	else if (outSampleFmt == AV_SAMPLE_FMT_S16){
		printf("S16\n");
		sformat.bits = 16;
	}
	else if (outSampleFmt == AV_SAMPLE_FMT_S32){
		printf("S32\n");
		sformat.bits = 32;
	}
	else
	{
		sformat.bits = 16;
	}
	sformat.channels = mStreamFrameInfo.pAudioCodecCtx->channels;
	sformat.rate = mStreamFrameInfo.pAudioCodecCtx->sample_rate;
	sformat.byte_format = AO_FMT_NATIVE;
	sformat.matrix = 0;
	mDevice = ao_open_live(driver, &sformat, NULL);
	if (mDevice == NULL)
	{
		printf("StreamPlay@Init(): couldn't open the audio device!");
		return false;
	}
#endif

#if USE_SDL_AUDIO
	//SDL_AudioSpec
	mWantedSpec.freq = outSampleRate;
	mWantedSpec.format = AUDIO_S16SYS;
	mWantedSpec.channels = outChannels;
	mWantedSpec.silence = 0;
	mWantedSpec.samples = outNbSamples;
	mWantedSpec.callback = FillAudioCallback;
	mWantedSpec.userdata = mStreamFrameInfo.pAudioCodecCtx;

	if (SDL_OpenAudio(&mWantedSpec, NULL)<0){
		printf("StreamPlay@Init(): can't open audio.\n");
		return false;
	}
#endif
	return true;
}

bool StreamPlay::InitVideoStream()
{
	

	// Find the decoder for the video stream
	mStreamFrameInfo.pVideoCodec = avcodec_find_decoder(mStreamFrameInfo.pFormatCtx->streams[mVideoStream]->codecpar->codec_id);
	if (mStreamFrameInfo.pVideoCodec == NULL){
		printf("StreamPlay@InitVideoStream(): Video Codec not found.\n");
		return false;
	}
	int den = mStreamFrameInfo.pFormatCtx->streams[mVideoStream]->avg_frame_rate.den;
	int num = mStreamFrameInfo.pFormatCtx->streams[mVideoStream]->avg_frame_rate.num;
	if (den != 0 && num != 0)
		mSDLThreadInfo.frameDelayTimeMs = 1000.f * den / num;
	// Get a pointer to the codec context for the video stream
	mStreamFrameInfo.pVideoCodecCtx = avcodec_alloc_context3(mStreamFrameInfo.pVideoCodec); //mStreamFrameInfo.pFormatCtx->streams[mVideoStream]->codec;

	avcodec_parameters_to_context(mStreamFrameInfo.pVideoCodecCtx, mStreamFrameInfo.pFormatCtx->streams[mVideoStream]->codecpar);

	// Open codec
	if (avcodec_open2(mStreamFrameInfo.pVideoCodecCtx, mStreamFrameInfo.pVideoCodec, NULL) < 0){
		printf("StreamPlay@InitVideoStream(): Could not open Video codec.\n");
		return false;
	}

	mStreamFrameInfo.pFrameRGB = av_frame_alloc();
	int width = mStreamFrameInfo.pVideoCodecCtx->width;
	int height = mStreamFrameInfo.pVideoCodecCtx->height;
	unsigned char *outBuffer = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_RGB24,
		                                                                           width, height, 1));
	if (!outBuffer)
	{
		printf("StreamPlay@InitVideoStream(): Could not malloc the outBuffer.\n");
		return false;
	}
	if (av_image_fill_arrays(mStreamFrameInfo.pFrameRGB->data,
		mStreamFrameInfo.pFrameRGB->linesize,
		outBuffer,
		AV_PIX_FMT_RGB24,
		width, height, 1) < 0)
	{
		printf("StreamPlay@InitVideoStream(): Could not setup the data pointers.\n");
		return false;
	}

	mStreamFrameInfo.pVideoConvertCtx = sws_getContext(width, height, 
		                                               mStreamFrameInfo.pVideoCodecCtx->pix_fmt,
													   width, height, 
													   AV_PIX_FMT_RGB24, SWS_BICUBIC,
													   NULL, NULL, NULL);


	frame.width = mStreamFrameInfo.pVideoCodecCtx->width;
	frame.height = mStreamFrameInfo.pVideoCodecCtx->height;
	frame.cn = 3;
	frame.step = 0;
	frame.data = NULL;

#ifdef USE_SDL
	SDL_Window* screen = SDL_CreateWindow("DaDaoPlayer", 
		                                  SDL_WINDOWPOS_UNDEFINED, 
										  SDL_WINDOWPOS_UNDEFINED,
										  width, height,
										  SDL_WINDOW_OPENGL);
	if (!screen) {
		printf("SDL: could not create window - exiting:%s\n", SDL_GetError());
		return false;
	}
	mSDLRenderer = SDL_CreateRenderer(screen, -1, 0);
	//IYUV: Y + U + V  (3 planes)
	//YV12: Y + V + U  (3 planes)
	mSDLTexture = SDL_CreateTexture(mSDLRenderer, 
		                            SDL_PIXELFORMAT_RGB24, 
									SDL_TEXTUREACCESS_STREAMING, 
									width, height);
#endif
	/*mSDLRect.x = 0;
	mSDLRect.y = 0;
	mSDLRect.w = width;
	mSDLRect.h = height;*/
	mSDLThreadInfo.threadExit = 0;
	mSDLThreadInfo.threadPause = 0;
	

	return true;
}

void StreamPlay::AudioThread()
{
	while (1)
	{
		std::unique_lock<std::mutex> lck(mMutexAudioStart);
		//while (mcvbDeviceID.wait_for(lck, std::chrono::milliseconds(10000)) == std::cv_status::timeout);
		mcvbAudioStart.wait(lck);
		if (!DecodeAudio())
			return;
		/*std::unique_lock<std::mutex> lck1(mMutexAudioEnd);
		mcvbAudioEnd.notify_one();*/
	}
}

bool StreamPlay::DecodeAudio()
{
	int ret = -1;
	if (mStreamFrameInfo.packet->stream_index == mAudioStream){
		int gotFrame = 0;
		ret = avcodec_decode_audio4(mStreamFrameInfo.pAudioCodecCtx, mStreamFrameInfo.pFrame, &gotFrame, mStreamFrameInfo.packet);
		if (ret < 0) {
			printf("Error in decoding audio frame.\n");
			return false;
		}

#if 1 // compatible with AV_SAMPLE_FMT_FLTP
		if (gotFrame)
		{
			if (mStreamFrameInfo.pAudioCodecCtx->sample_fmt == AV_SAMPLE_FMT_FLTP)
			{
				int nb_samples = mStreamFrameInfo.pFrame->nb_samples;
				int channels = mStreamFrameInfo.pFrame->channels;
				int outputBufferLen = nb_samples & channels * 2;
				auto outputBuffer = (int16_t*)mpOutBuffer;

				for (int i = 0; i < nb_samples; i++)
				{
					for (int c = 0; c < channels; c++)
					{
						float* extended_data = (float*)mStreamFrameInfo.pFrame->extended_data[c];
						outputBuffer[i * channels + c] = (int16_t)(extended_data[i] * 32767.0f);
					}
				}
				//return;
			}
			else
			{
				swr_convert(mStreamFrameInfo.pAudioConvertCtx,
					&mpOutBuffer,
					MAX_AUDIO_FRAME_SIZE,
					(const uint8_t **)mStreamFrameInfo.pFrame->extended_data,
					mStreamFrameInfo.pFrame->nb_samples);

				/*printf("index:%5d\t pts:%lld\t packet size:%d\n", index, mAudioFrameInfo.packet->pts, mAudioFrameInfo.packet->size);
				index++;*/
				//ao_play(mDevice, (char*)mpOutBuffer, mOutBufferSize);
			}
#if USE_LIBAO
			ao_play(mDevice, (char*)mpOutBuffer, mOutBufferSize);
#endif
#if USE_SDL_AUDIO
			while (gAudioLen > 0)//Wait until finish
				SDL_Delay(1);

			//Set audio buffer (PCM data)
			gAudioChunk = (Uint8 *)mpOutBuffer;
			//Audio buffer length
			gAudioLen = mOutBufferSize;
			gAudioPos = gAudioChunk;

			//Play
			SDL_PauseAudio(0);
#endif
		}
#endif

		//if (gotFrame > 0){
		//	swr_convert(mStreamFrameInfo.pAudioConvertCtx,
		//		&mpOutBuffer,
		//		MAX_AUDIO_FRAME_SIZE,
		//		(const uint8_t **)mStreamFrameInfo.pFrame->extended_data,
		//		mStreamFrameInfo.pFrame->nb_samples);

		//	/*printf("index:%5d\t pts:%lld\t packet size:%d\n", index, mAudioFrameInfo.packet->pts, mAudioFrameInfo.packet->size);
		//	index++;*/
		//	ao_play(mDevice, (char*)mpOutBuffer, mOutBufferSize);
		//}
		
	}
	//av_free_packet(mStreamFrameInfo.packet);
	return true;
}

void StreamPlay::DecodeVideo(AVCodecContext *decCtx, AVFrame *frame, AVPacket *pkt)
{
	char buf[1024];
	int ret;

	ret = avcodec_send_packet(decCtx, pkt);
	if (ret < 0) {
		fprintf(stderr, "Error sending a packet for decoding\n");
		exit(1);
	}

	while (!avcodec_receive_frame(decCtx, frame)) {
		sws_scale(mStreamFrameInfo.pVideoConvertCtx,
			(const unsigned char* const*)mStreamFrameInfo.pFrame->data,
			mStreamFrameInfo.pFrame->linesize, 0,
			mStreamFrameInfo.pVideoCodecCtx->height,
			mStreamFrameInfo.pFrameRGB->data,
			mStreamFrameInfo.pFrameRGB->linesize);
#ifdef USE_SDL
		//SDL---------------------------
		SDL_UpdateTexture(mSDLTexture, NULL,
			mStreamFrameInfo.pFrameRGB->data[0],
			mStreamFrameInfo.pFrameRGB->linesize[0]);
		SDL_RenderClear(mSDLRenderer);
		//SDL_RenderCopy( sdlRenderer, sdlTexture, &sdlRect, &sdlRect );  
		SDL_RenderCopy(mSDLRenderer, mSDLTexture, NULL, NULL);
		SDL_RenderPresent(mSDLRenderer);
#endif
	}
}

bool StreamPlay::VideoThread()
{
	int ret = -1;
	if (mStreamFrameInfo.packet->stream_index == mVideoStream){
		/*if (mStreamFrameInfo.packet->size)
			DecodeVideo(mStreamFrameInfo.pVideoCodecCtx, mStreamFrameInfo.pFrame, mStreamFrameInfo.packet);*/

		//printf("after SDL_WaitEvent\n");
		//SDL_WaitEvent(&mEvent);
		if (1){//(mEvent.type == SFM_REFRESH_EVENT){
			int gotFrame = 0;
			ret = avcodec_decode_video2(mStreamFrameInfo.pVideoCodecCtx, mStreamFrameInfo.pFrame, &gotFrame, mStreamFrameInfo.packet);
			if (ret < 0){
				printf("Error in decoding video frame.\n");
				return false;
			}
			if (gotFrame)
			{
				sws_scale(mStreamFrameInfo.pVideoConvertCtx,
					(const unsigned char* const*)mStreamFrameInfo.pFrame->data,
					mStreamFrameInfo.pFrame->linesize, 0,
					mStreamFrameInfo.pVideoCodecCtx->height,
					mStreamFrameInfo.pFrameRGB->data,
					mStreamFrameInfo.pFrameRGB->linesize);
#ifdef USE_SDL
				//SDL---------------------------
				SDL_UpdateTexture(mSDLTexture, NULL,
					mStreamFrameInfo.pFrameRGB->data[0],
					mStreamFrameInfo.pFrameRGB->linesize[0]);
				SDL_RenderClear(mSDLRenderer);
				//SDL_RenderCopy( sdlRenderer, sdlTexture, &sdlRect, &sdlRect );  
				SDL_RenderCopy(mSDLRenderer, mSDLTexture, NULL, NULL);
				SDL_RenderPresent(mSDLRenderer);
				//SDL End-----------------------
#endif
			}
		}
	}
	return true;
}

void StreamPlay::TestMultiThread()
{
	std::thread audioThread = std::thread(&StreamPlay::AudioThread, this);
#ifdef USE_SDL
	SDL_Event evt;
#endif
	while (1)
	{
		/*std::unique_lock<std::mutex> lckEnd(mMutexAudioEnd);
		while (mcvbAudioEnd.wait_for(lckEnd, std::chrono::milliseconds(100)) == std::cv_status::timeout);*/

		int ret = -1;
		ret = av_read_frame(mStreamFrameInfo.pFormatCtx, mStreamFrameInfo.packet);
		if (ret < 0)
		{
			break;
		}
		if (mStreamFrameInfo.packet->stream_index == mVideoStream)
		{
			/*std::unique_lock<std::mutex> lck(mMutexVideoStart);
			mcvbVideoStart.wait(lck);*/
			DecodeVideo(mStreamFrameInfo.pVideoCodecCtx, mStreamFrameInfo.pFrame, mStreamFrameInfo.packet);
		}
		else if (mStreamFrameInfo.packet->stream_index == mAudioStream)
		{
			std::unique_lock<std::mutex> lckStart(mMutexAudioStart);
			mcvbAudioStart.notify_one();
		}
#ifdef USE_SDL
		SDL_PollEvent(&evt);
		switch (evt.type)
		{
		case SDL_QUIT:
			bStop = true;
			break;
		default:
			break;
		}
#endif
		if (bStop)
		{
			audioThread.join();
			break;
		}
	}
}

/*
	xcy add +++ 20181105
	设置是否静音。主要是在收到其他播放指令时会用到
*/
bool StreamPlay::SetMute(bool bIsMute)
{
	if (m_mutexGetMute.try_lock())
	{
		m_bIsMute = bIsMute;
		m_mutexGetMute.unlock();
		return true;
	}
	return false;
}

void StreamPlay::TestSync()
{
#ifdef USE_SDL
	SDL_Event evt;
#endif
	while (1)
	{
		int ret = -1;
		ret = av_read_frame(mStreamFrameInfo.pFormatCtx, mStreamFrameInfo.packet);
		if (ret < 0)
		{
			break;
		}
		if (mStreamFrameInfo.packet->stream_index == mVideoStream)
		{
			DecodeVideo(mStreamFrameInfo.pVideoCodecCtx, mStreamFrameInfo.pFrame, mStreamFrameInfo.packet);
		}
		else if (mStreamFrameInfo.packet->stream_index == mAudioStream)
		{
			if (!DecodeAudio())
				break;
		}
#ifdef USE_SDL
		SDL_PollEvent(&evt);
		switch (evt.type)
		{
		case SDL_QUIT:
			bStop = true;
			break;
		default:
			break;
		}
#endif
		if (bStop)
			break;
	}
}

int StreamPlay::GrabFrame()
{
	int type = 0;
	//bool valid = false;
	int countErrs = 0;
	const int maxNumberOfAttempts = 1 << 9;
	if (!mStreamFrameInfo.pFormatCtx
		|| !mStreamFrameInfo.pFormatCtx->streams[mVideoStream])
		return type;
	while (!type)
	{
		av_free_packet(mStreamFrameInfo.packet);
		if (timeoutInfo.bTimeout)
		{
			//valid = false;
			type = 0;
			break;
		}
		timeoutInfo.timeBeforeBlocking = high_resolution_clock::now();
		int ret = av_read_frame(mStreamFrameInfo.pFormatCtx, mStreamFrameInfo.packet);
		if (ret == AVERROR(EAGAIN)) 
			continue;
		if (mStreamFrameInfo.packet->stream_index != mVideoStream
			&& mStreamFrameInfo.packet->stream_index != mAudioStream)
		{
			av_free_packet(mStreamFrameInfo.packet);
			countErrs++;
			if (countErrs > maxNumberOfAttempts)
				break;
			continue;
		}
		int gotFrame = 0;
		if (mStreamFrameInfo.packet->stream_index == mVideoStream)
		{			
			ret = avcodec_decode_video2(mStreamFrameInfo.pVideoCodecCtx,
				mStreamFrameInfo.pFrame,
				&gotFrame,
				mStreamFrameInfo.packet);
			if (ret < 0){
				printf("Error in decoding video frame.\n");
				return 0;
			}
			if (gotFrame)
			{
				//valid = true;
				type = 2;
				timeoutInfo.timeBeforeBlocking = high_resolution_clock::now();
			}
			else
			{
				countErrs++;
				if (countErrs > maxNumberOfAttempts)
					break;
			}
		}
		else if (mStreamFrameInfo.packet->stream_index == mAudioStream)
		{
			ret = avcodec_decode_audio4(mStreamFrameInfo.pAudioCodecCtx, mStreamFrameInfo.pFrame, &gotFrame, mStreamFrameInfo.packet);
			if (ret < 0) {
				printf("Error in decoding audio frame.\n");
				return 0;
			}
			if (gotFrame)
			{
				//valid = true;
				type = 1;
				timeoutInfo.timeBeforeBlocking = high_resolution_clock::now();
			}
			else
			{
				countErrs++;
				if (countErrs > maxNumberOfAttempts)
					break;
			}
		}
	}
	return type;
}

bool StreamPlay::RetrieveAudioFrame()
{
#if 0
	if (!mStreamFrameInfo.pFormatCtx->streams[mAudioStream]
		|| !mStreamFrameInfo.pFrame->data[0])
		return false;
	if (mStreamFrameInfo.pAudioCodecCtx->sample_fmt == AV_SAMPLE_FMT_FLTP)
	{
		int nb_samples = mStreamFrameInfo.pFrame->nb_samples;
		int channels = mStreamFrameInfo.pFrame->channels;
		int outputBufferLen = nb_samples & channels * 2;
		auto outputBuffer = (int16_t*)mpOutBuffer;

		for (int i = 0; i < nb_samples; i++)
		{
			for (int c = 0; c < channels; c++)
			{
				float* extended_data = (float*)mStreamFrameInfo.pFrame->extended_data[c];
				outputBuffer[i * channels + c] = (int16_t)(extended_data[i] * 32767.0f);
			}
		}
		//return;
	}
	else
	{
		swr_convert(mStreamFrameInfo.pAudioConvertCtx,
			&mpOutBuffer,
			MAX_AUDIO_FRAME_SIZE,
			(const uint8_t **)mStreamFrameInfo.pFrame->extended_data,
			mStreamFrameInfo.pFrame->nb_samples);

		/*printf("index:%5d\t pts:%lld\t packet size:%d\n", index, mAudioFrameInfo.packet->pts, mAudioFrameInfo.packet->size);
		index++;*/
		//ao_play(mDevice, (char*)mpOutBuffer, mOutBufferSize);
	}
	return true;
#else
	if (!mStreamFrameInfo.pFormatCtx->streams[mAudioStream]
		|| !mStreamFrameInfo.pFrame->data[0])
		return false;
	
	int nb_samples = mStreamFrameInfo.pFrame->nb_samples;
	int channels = mStreamFrameInfo.pFrame->channels;
	int outputBufferLen = nb_samples & channels * 2;
	auto outputBuffer = (int16_t*)mpOutBuffer;

	for (int i = 0; i < nb_samples; i++)
	{
		for (int c = 0; c < channels; c++)
		{
			float* extended_data = (float*)mStreamFrameInfo.pFrame->extended_data[c];
			outputBuffer[i * channels + c] = (int16_t)(extended_data[i] * 32767.0f);
			//outputBuffer[i * channels + c] = (int16_t)(extended_data[i] * 32767.0f * 0.5f); // 乘以一个0.5就表示声音降级，也可以用其他的小于1的浮点数
		}
	}
	return true;
#endif
}

void StreamPlay::PlayAudio()
{
	//// xcy add +++ 20181105
	if (!m_mutexGetMute.try_lock())
	{
		return;
	}

	if (m_bIsMute)
	{
		m_mutexGetMute.unlock();
		return;
	}
	m_mutexGetMute.unlock();
	// xcy add --- 20181105

#if USE_LIBAO
	ao_play(mDevice, (char*)mpOutBuffer, mOutBufferSize);
#endif
#if USE_SDL_AUDIO
	while (gAudioLen > 0)//Wait until finish
		SDL_Delay(1);

	//Set audio buffer (PCM data)
	gAudioChunk = (Uint8 *)mpOutBuffer;
	//Audio buffer length
	gAudioLen = mOutBufferSize;
	gAudioPos = gAudioChunk;

	//Play
	SDL_PauseAudio(0);
#endif
}

bool StreamPlay::RetrieveFrame(unsigned char** data,
	                           int* step,
							   int* width,
							   int* height,
							   int* cn)
{
	if (!mStreamFrameInfo.pFormatCtx->streams[mVideoStream]
		|| !mStreamFrameInfo.pFrame->data[0])
		return false;

	if (frame.data == NULL)
	{
		int bufferWidth = mStreamFrameInfo.pVideoCodecCtx->width;
		int bufferHeight = mStreamFrameInfo.pVideoCodecCtx->height;
		mStreamFrameInfo.pVideoConvertCtx = sws_getCachedContext(mStreamFrameInfo.pVideoConvertCtx,
			                                                     bufferWidth, bufferHeight,
			                                                     mStreamFrameInfo.pVideoCodecCtx->pix_fmt,
																 bufferWidth, bufferHeight,
																 AV_PIX_FMT_RGB24,
																 SWS_BICUBIC,
																 NULL, NULL, NULL);
		if (mStreamFrameInfo.pVideoConvertCtx == NULL)
			return false;

		av_frame_unref(mStreamFrameInfo.pFrameRGB);
		mStreamFrameInfo.pFrameRGB->format = AV_PIX_FMT_BGR24;
		mStreamFrameInfo.pFrameRGB->width = bufferWidth;
		mStreamFrameInfo.pFrameRGB->height = bufferHeight;
		if (0 != av_frame_get_buffer(mStreamFrameInfo.pFrameRGB, 32))
		{
			printf("OutOfMemory\n");
			return false;
		}

		frame.width = bufferWidth;
		frame.height = bufferHeight;
		frame.cn = 3;
		frame.data = mStreamFrameInfo.pFrameRGB->data[0];
		frame.step = mStreamFrameInfo.pFrameRGB->linesize[0];
	}

	sws_scale(mStreamFrameInfo.pVideoConvertCtx,
		      (const unsigned char* const*)mStreamFrameInfo.pFrame->data,
			  mStreamFrameInfo.pFrame->linesize, 0,
			  mStreamFrameInfo.pVideoCodecCtx->height,
			  mStreamFrameInfo.pFrameRGB->data,
			  mStreamFrameInfo.pFrameRGB->linesize);

	*data = frame.data;
	*step = frame.step;
	*width = frame.width;
	*height = frame.height;
	*cn = frame.cn;
}

bool StreamPlay::Respond()
{
	return MainSubsModel::Process() == 0 ? false : true;
}

bool StreamPlay::_onRunOnce_Sub(int)
{
#if 1 // test init alone
	if (!Init())
		return true;
#endif
#if 1
	while (1)
	{
		int type = GrabFrame();
		if (type == 1)
		{
			if (RetrieveAudioFrame())
			{
				PlayAudio();
			}
			else
				return false;
		}
		else if (type == 0)
			return false;
		if (bStop)
		{
			Quit();
			break;
		}
	}
#else
	while (1){
		//mTimeBeforeBlocking = high_resolution_clock::now();
		//printf("start%d\n", index);
#ifdef USE_SDL
		SDL_WaitEvent(&mEvent);
		if (mEvent.type == SFM_REFRESH_EVENT){
#endif
			while (1){
				int ret = -1;
				timeoutInfo.timeBeforeBlocking = high_resolution_clock::now();
				ret = av_read_frame(mStreamFrameInfo.pFormatCtx, mStreamFrameInfo.packet);
				if (ret < 0)
				{
					mSDLThreadInfo.threadExit = 1;
					return false;
				}
				if (mStreamFrameInfo.packet->stream_index == mVideoStream){
					break;
				}
				else if (mStreamFrameInfo.packet->stream_index == mAudioStream)
					if (!DecodeAudio())
						return false;
				if (bStop)
				{
					Quit();
					return true;
				}
			}
			/*if (mStreamFrameInfo.packet->stream_index == mAudioStream
			|| mStreamFrameInfo.packet->stream_index == mVideoStream)
			break;*/

			//printf("end%d\n", index);
			//std::thread audioThread = std::thread(&StreamPlay::AudioThread, this);
			/*if (!AudioThread())
			return;*/
			if (!VideoThread())
				return false;
			//audioThread.join();
			av_free_packet(mStreamFrameInfo.packet);
#ifdef USE_SDL
		}

		else if (mEvent.type == SDL_KEYDOWN){
			//Pause
			if (mEvent.key.keysym.sym == SDLK_SPACE)
				mSDLThreadInfo.threadPause = !mSDLThreadInfo.threadPause;
		}
		else if (mEvent.type == SDL_QUIT){
			mSDLThreadInfo.threadExit = 1;
		}
		else if (mEvent.type == SFM_BREAK_EVENT){
			return;
		}
#endif		
		if (bStop)
		{
			Quit();
			return true;
		}
	}
#endif
	return true;
}

void StreamPlay::Stop()
{
	bStop = true;
}

void StreamPlay::Start()
{
	int index = 0;
#ifdef USE_SDL
	SDL_Thread* video_tid = SDL_CreateThread(VideoRefreshThread, NULL, (void*)&mSDLThreadInfo);
#endif

	while (1){
		//mTimeBeforeBlocking = high_resolution_clock::now();
		//printf("start%d\n", index);
#ifdef USE_SDL
		SDL_WaitEvent(&mEvent);
		if (mEvent.type == SFM_REFRESH_EVENT){
#endif
			while (1){
				int ret = -1;
				timeoutInfo.timeBeforeBlocking = high_resolution_clock::now();
				ret = av_read_frame(mStreamFrameInfo.pFormatCtx, mStreamFrameInfo.packet);
				if (ret < 0)
				{
					mSDLThreadInfo.threadExit = 1;
					return;
				}
				if (mStreamFrameInfo.packet->stream_index == mVideoStream){
					break;
				}
				else if (mStreamFrameInfo.packet->stream_index == mAudioStream)
					if (!DecodeAudio())
						return;
			}
			/*if (mStreamFrameInfo.packet->stream_index == mAudioStream
				|| mStreamFrameInfo.packet->stream_index == mVideoStream)
				break;*/

			//printf("end%d\n", index);
			//std::thread audioThread = std::thread(&StreamPlay::AudioThread, this);
			/*if (!AudioThread())
				return;*/
			if (!VideoThread())
				return;
			//audioThread.join();
			av_free_packet(mStreamFrameInfo.packet);
#ifdef USE_SDL
		}

		else if (mEvent.type == SDL_KEYDOWN){
			//Pause
			if (mEvent.key.keysym.sym == SDLK_SPACE)
				mSDLThreadInfo.threadPause = !mSDLThreadInfo.threadPause;
		}
		else if (mEvent.type == SDL_QUIT){
			mSDLThreadInfo.threadExit = 1;
		}
		else if (mEvent.type == SFM_BREAK_EVENT){
			return;
		}
#endif		
		if (bStop)
			break;
	}
}

void StreamPlay::Quit()
{
	if (mStreamFrameInfo.pAudioConvertCtx != NULL)
		swr_free(&mStreamFrameInfo.pAudioConvertCtx);
	if (mStreamFrameInfo.pVideoConvertCtx != NULL)
		sws_freeContext(mStreamFrameInfo.pVideoConvertCtx);

	if (mpOutBuffer != NULL)
		av_free(mpOutBuffer);
	// Close the codec
	if (mStreamFrameInfo.pAudioCodecCtx != NULL)
		avcodec_close(mStreamFrameInfo.pAudioCodecCtx);
	if (mStreamFrameInfo.pVideoCodecCtx != NULL)
		avcodec_close(mStreamFrameInfo.pVideoCodecCtx);
	// Close the input
	if (mStreamFrameInfo.pFormatCtx != NULL)
		avformat_close_input(&mStreamFrameInfo.pFormatCtx);

	if (mStreamFrameInfo.pFrame != NULL)
		av_frame_free(&mStreamFrameInfo.pFrame);

	if (mStreamFrameInfo.pFrameRGB != NULL)
		av_frame_free(&mStreamFrameInfo.pFrameRGB);

#if USE_LIBAO
	ao_shutdown();
#endif
#if USE_SDL_AUDIO
	SDL_CloseAudio();//Close SDL
#endif	
#ifdef USE_SDL
	SDL_Quit();
#endif

}