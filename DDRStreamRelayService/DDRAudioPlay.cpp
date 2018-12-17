#pragma warning(disable :4996)
#include "DDRAudioPlay.h"

//#include "ao/ao.h"

#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio
#define LIBAVFORMAT_INTERRUPT_TIMEOUT_SECOND 15.f

AudioPlay::AudioPlay()
	: MainSubsModel(1, 0, 100)
{

}

AudioPlay::~AudioPlay()
{
	EndSubThreads();
	Quit();
}

void AudioPlay::Quit()
{
	if (mAudioFrameInfo.pAudioConvertCtx)
		swr_free(&mAudioFrameInfo.pAudioConvertCtx);

	if (mpOutBuffer)
		av_free(mpOutBuffer);
	// Close the codec
	if (mAudioFrameInfo.pCodecCtx)
		avcodec_close(mAudioFrameInfo.pCodecCtx);
	// Close the video file
	if (mAudioFrameInfo.pFormatCtx)
		avformat_close_input(&mAudioFrameInfo.pFormatCtx);

	if (mAudioFrameInfo.pFrame)
		av_frame_free(&mAudioFrameInfo.pFrame);

	ao_shutdown();
}

int AudioPlayInterruptCallback(void *ctx)
{
	time_point<system_clock, system_clock::duration>* pTimeBeforeBlocking = (time_point<system_clock, system_clock::duration>*)ctx;
	auto curTime = high_resolution_clock::now();
	if (duration_cast<duration<float>>(curTime - *pTimeBeforeBlocking).count() > LIBAVFORMAT_INTERRUPT_TIMEOUT_SECOND)
	{
		//printf("abort%f\n", duration_cast<duration<float>>(curTime - *pTimeBeforeBlocking).count());
		return 1;
	}
	else
	{
		//printf("normal..................................................%f\n", duration_cast<duration<float>>(curTime - *pTimeBeforeBlocking).count());
		return 0;
	}
}

bool AudioPlay::Init(const char* fileName)
{
	mInUrlName = fileName;
	mAudioFrameInfo.pFormatCtx = avformat_alloc_context(); //NULL;
	//Open
	AVDictionary *optionsDict = NULL;
	av_dict_set(&optionsDict, "stimeout", "2000000", 0);
	av_dict_set(&optionsDict, "tune", "zerolatency", 0);
	//av_dict_set(&optionsDict, "probesize", "32", 0);
	av_dict_set(&optionsDict, "fflags", "nobuffer", 0);
	av_dict_set(&optionsDict, "max_analyze_duration", "1000", 0);
	//av_dict_set(&optionsDict, "timeout", "1000", 0);
	mAudioFrameInfo.pFormatCtx->interrupt_callback.callback = AudioPlayInterruptCallback;
	mAudioFrameInfo.pFormatCtx->interrupt_callback.opaque = &mTimeBeforeBlocking;
	mTimeBeforeBlocking = high_resolution_clock::now();
	if (avformat_open_input(&mAudioFrameInfo.pFormatCtx, mInUrlName, NULL, &optionsDict) != 0){
		printf("AudioPlay@Init(): Couldn't open input stream.\n");
		return false;
	}
	// Retrieve stream information
	if (avformat_find_stream_info(mAudioFrameInfo.pFormatCtx, NULL)<0){
		printf("AudioPlay@Init(): Couldn't find stream information.\n");
		return false;
	}
	// Dump valid information onto standard error
	av_dump_format(mAudioFrameInfo.pFormatCtx, 0, mInUrlName, false);
	// Find the first audio stream
	mAudioStream = -1;
	for (int i = 0; i < mAudioFrameInfo.pFormatCtx->nb_streams; i++)
		if (mAudioFrameInfo.pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
			mAudioStream = i;
			break;
		}

	if (mAudioStream == -1){
		printf("AudioPlay@Init(): Didn't find a audio stream.\n");
		return false;
	}

	// Get a pointer to the codec context for the audio stream
	mAudioFrameInfo.pCodecCtx = mAudioFrameInfo.pFormatCtx->streams[mAudioStream]->codec;

	// Find the decoder for the audio stream
	mAudioFrameInfo.pCodec = avcodec_find_decoder(mAudioFrameInfo.pCodecCtx->codec_id);
	if (mAudioFrameInfo.pCodec == NULL){
		printf("AudioPlay@Init(): Codec not found.\n");
		return false;
	}

	// Open codec
	if (avcodec_open2(mAudioFrameInfo.pCodecCtx, mAudioFrameInfo.pCodec, NULL)<0){
		printf("AudioPlay@Init(): Could not open codec.\n");
		return false;
	}

	mAudioFrameInfo.packet = (AVPacket *)av_malloc(sizeof(AVPacket));
	av_init_packet(mAudioFrameInfo.packet);

	//Out Audio Param
	uint64_t outChannelLayout = AV_CH_LAYOUT_STEREO;
	//nb_samples: AAC-1024 MP3-1152
	int outNbSamples = mAudioFrameInfo.pCodecCtx->frame_size;
	AVSampleFormat outSampleFmt = AV_SAMPLE_FMT_S16;;// mAudioFrameInfo.pCodecCtx->sample_fmt; //
	int outSampleRate = 44100; //mAudioFrameInfo.pCodecCtx->sample_rate; //
	int outChannels = av_get_channel_layout_nb_channels(outChannelLayout);
	//Out Buffer Size
	mOutBufferSize = av_samples_get_buffer_size(NULL, outChannels, outNbSamples, outSampleFmt, 1);

	mpOutBuffer = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE * 2);
	mAudioFrameInfo.pFrame = av_frame_alloc();

	//FIX:Some Codec's Context Information is missing
	int64_t inChannelLayout = av_get_default_channel_layout(mAudioFrameInfo.pCodecCtx->channels);
	//Swr
	mAudioFrameInfo.pAudioConvertCtx = swr_alloc();
	mAudioFrameInfo.pAudioConvertCtx = swr_alloc_set_opts(mAudioFrameInfo.pAudioConvertCtx, outChannelLayout, outSampleFmt, outSampleRate,
		inChannelLayout, mAudioFrameInfo.pCodecCtx->sample_fmt, mAudioFrameInfo.pCodecCtx->sample_rate, 0, NULL);
	swr_init(mAudioFrameInfo.pAudioConvertCtx);

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
	sformat.channels = mAudioFrameInfo.pCodecCtx->channels;
	sformat.rate = mAudioFrameInfo.pCodecCtx->sample_rate;
	sformat.byte_format = AO_FMT_NATIVE;
	sformat.matrix = 0;
	mDevice = ao_open_live(driver, &sformat, NULL);
	if (mDevice == NULL)
	{
		printf("AudioPlay@Init(): couldn't open the device!");
		return false;
	}
	bStop = false;
	return true;
}

bool AudioPlay::Respond()
{
	return MainSubsModel::Process() == 0 ? false : true;
}

void AudioPlay::QuitSubThreads()
{
	bStop = true;
}

bool AudioPlay::_onRunOnce_Sub(int)
{
	while (1){
		mTimeBeforeBlocking = high_resolution_clock::now();
		//printf("start%d\n", index);
		int ret = av_read_frame(mAudioFrameInfo.pFormatCtx, mAudioFrameInfo.packet);
		if (ret < 0)
			break;
		//printf("end%d\n", index);
		//index++;
		if (mAudioFrameInfo.packet->stream_index == mAudioStream){
			int gotFrame = 0;
			ret = avcodec_decode_audio4(mAudioFrameInfo.pCodecCtx, mAudioFrameInfo.pFrame, &gotFrame, mAudioFrameInfo.packet);
			if (ret < 0) {
				printf("Error in decoding audio frame.\n");
				return false;
			}
			if (gotFrame > 0){
				swr_convert(mAudioFrameInfo.pAudioConvertCtx,
					&mpOutBuffer,
					MAX_AUDIO_FRAME_SIZE,
					(const uint8_t **)mAudioFrameInfo.pFrame->data,
					mAudioFrameInfo.pFrame->nb_samples);

				/*printf("index:%5d\t pts:%lld\t packet size:%d\n", index, mAudioFrameInfo.packet->pts, mAudioFrameInfo.packet->size);
				index++;*/
				ao_play(mDevice, (char*)mpOutBuffer, mOutBufferSize);
			}
		}
		av_free_packet(mAudioFrameInfo.packet);
		if (bStop)
			break;
	}
	return true;
}

void AudioPlay::Stop()
{
	bStop = true;
}

void AudioPlay::Start()
{
	int index = 0;
	while (1){
		mTimeBeforeBlocking = high_resolution_clock::now();
		//printf("start%d\n", index);
		int ret = av_read_frame(mAudioFrameInfo.pFormatCtx, mAudioFrameInfo.packet);
		if (ret < 0)
			break;
		//printf("end%d\n", index);
		index++;
		if (mAudioFrameInfo.packet->stream_index == mAudioStream){
			int gotFrame = 0;
			ret = avcodec_decode_audio4(mAudioFrameInfo.pCodecCtx, mAudioFrameInfo.pFrame, &gotFrame, mAudioFrameInfo.packet);
			if (ret < 0) {
				printf("Error in decoding audio frame.\n");
				return;
			}
			if (gotFrame > 0){
				swr_convert(mAudioFrameInfo.pAudioConvertCtx,
					        &mpOutBuffer, 
							MAX_AUDIO_FRAME_SIZE,
							(const uint8_t **)mAudioFrameInfo.pFrame->data,
							mAudioFrameInfo.pFrame->nb_samples);

				/*printf("index:%5d\t pts:%lld\t packet size:%d\n", index, mAudioFrameInfo.packet->pts, mAudioFrameInfo.packet->size);
				index++;*/
				ao_play(mDevice, (char*)mpOutBuffer, mOutBufferSize);
			}
		}
		av_free_packet(mAudioFrameInfo.packet);
		if (bStop)
			break;
		
	}
}
