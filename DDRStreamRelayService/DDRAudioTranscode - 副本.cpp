#pragma once
#include <Windows.h>
#include "DDRAudioTranscode.h"
#include "zWalleAudio.h"
#include "LocalNetworkNode\LNNFactory.h"
/* The output bit rate in bit/s */
#define OUTPUT_BIT_RATE 32000
/* The number of output channels */
#define OUTPUT_CHANNELS 2
#define LIBAVFORMAT_INTERRUPT_TIMEOUT_SECOND 20.f

bool ServerCB(void *pVecBytes, void *pArg){
	zWalleAudio *audio = (zWalleAudio*)pArg;
	DDRLNN::ResizeVecBytes(pVecBytes, 6400);
	if (true == audio->IsInOpen()){
		BYTE DATA[6400];
		if (6400 == audio->In(DATA)){
			//printf("发送6400字节\n");
			memcpy(DDRLNN::GetVecBytesHead(pVecBytes), DATA, 6400);
			return true;
		}
	}
	return false;
}

AudioTranscode::AudioTranscode()
	: MainSubsModel(1, 0, 100)
{
	/*if (!Init())
	{
		printf("AudioTranscode@Init(): 初始化失败，请检查！");
		return;
	}*/
}

AudioTranscode::~AudioTranscode()
{
	Quit();
}
#if 0 // 2018.9.29 useless
void AudioTranscode::SetInput(char* input)
{
	strcpy(mInFilename, input);
}

void AudioTranscode::SetOutput(char* output)
{
	strcpy(mOutFilename, output);
}
#endif

void AudioTranscode::QuitSubThread()
{
	bQuitSubThread = true;
}

void AudioTranscode::Quit()
{
	EndSubThreads();
	/* Write the trailer of the output file container. */
	/*if (mStreamInfo.outputFormatContext)
		WriteOutputFileTrailer(mStreamInfo.outputFormatContext);*/

	if (mStreamInfo.fifo)
		av_audio_fifo_free(mStreamInfo.fifo);
	swr_free(&mStreamInfo.resampleContext);
	if (mStreamInfo.outputCodecContext)
		avcodec_free_context(&mStreamInfo.outputCodecContext);
	if (mStreamInfo.outputFormatContext) {
		avio_closep(&mStreamInfo.outputFormatContext->pb);
		avformat_free_context(mStreamInfo.outputFormatContext);
	}
	if (mStreamInfo.inputCodecContext)
		avcodec_free_context(&mStreamInfo.inputCodecContext);
	if (mStreamInfo.inputFormatContext)
		avformat_close_input(&mStreamInfo.inputFormatContext);
}

bool AudioTranscode::Respond()
{
	return MainSubsModel::Process() == 0 ? false : true;
}

void AudioTranscode::Start()
{
	while (1) {
		/* Use the encoder's desired frame size for processing. */
		const int outputFrameSize = mStreamInfo.outputCodecContext->frame_size;
		int finished = 0;

		/* Make sure that there is one frame worth of samples in the FIFO
		* buffer so that the encoder can do its work.
		* Since the decoder's and the encoder's frame size may differ, we
		* need to FIFO buffer to store as many frames worth of input samples
		* that they make up at least one frame worth of output samples. */
		while (av_audio_fifo_size(mStreamInfo.fifo) < outputFrameSize) {
			/* Decode one frame worth of audio samples, convert it to the
			* output sample format and put it into the FIFO buffer. */
			if (ReadDecodeConvertAndStore(mStreamInfo.fifo, 
				                          mStreamInfo.inputFormatContext,
										  mStreamInfo.inputCodecContext,
										  mStreamInfo.outputCodecContext,
										  mStreamInfo.resampleContext, &finished))
				return;

			/* If we are at the end of the input file, we continue
			* encoding the remaining audio samples to the output file. */
			if (finished)
				break;
		}

		/* If we have enough samples for the encoder, we encode them.
		* At the end of the file, we pass the remaining samples to
		* the encoder. */
		while (av_audio_fifo_size(mStreamInfo.fifo) >= outputFrameSize ||
			(finished && av_audio_fifo_size(mStreamInfo.fifo) > 0))
			/* Take one frame worth of audio samples from the FIFO buffer,
			* encode it and write it to the output file. */
			if (LoadEncodeAndWrite(mStreamInfo.fifo, 
				                   mStreamInfo.outputFormatContext,
				                   mStreamInfo.outputCodecContext))
				return;

		/* If we are at the end of the input file and have encoded
		* all remaining samples, we can exit this loop and finish. */
		if (finished) {
			int dataWritten;
			/* Flush the encoder as it may have delayed frames. */
			do {
				dataWritten = 0;
				if (EncodeAudioFrame(NULL, 
					mStreamInfo.outputFormatContext,
					mStreamInfo.outputCodecContext, &dataWritten))
					return;
			} while (dataWritten);
			break;
		}
	}
}

bool AudioTranscode::_onRunOnce_Sub(int)
{
	/*****for local audio push*****/
#if 1
	zWalleAudio s_audio;
	std::shared_ptr<DDRLNN::LNNNodeGroupInterface> LNGPush;
	LNGPush = DDRLNN::CreateNodeGroup();
	if (strncmp(mLocalNodeName, "LNN", 3) == 0)
	{
		if (-1 == s_audio.InOpen()){
			printf("本地音频推流服务初始化失败，请检查！\n");
			return false;
		}
		
		int posStart = 0;
		std::string localName = std::string(mLocalNodeName);
		int pos = localName.find(":", posStart);
		int secPos = localName.find(":", pos + 1);
		std::string socketType = localName.substr(pos + 1, secPos - pos - 1);
		if (strcmp("UDP", socketType.c_str()) == 0) // 2018.10.24 UDP may have problems???
			LNGPush->AddUDPServer(mLocalNodeName);
		//LNGPush->AddTCPServer(mAudioInNames[0].c_str(), 0);
		else if (strcmp("TCP", socketType.c_str()) == 0)
			LNGPush->AddTCPServer(mLocalNodeName, 0);
		else
		{
			printf("本地音频推流未指定TCP/UDP，请检查！\n");
			return false;
		}
		s_audio.InStart();
		LNGPush->SetServerOption(mLocalNodeName, 1024 * 1024 * 8, 1024 * 256);
		LNGPush->_scb_SetArg_NewDataPublish(mLocalNodeName, &s_audio);
		LNGPush->_scb_SetFunc_NewDataPublish(mLocalNodeName, ServerCB);
		LNGPush->StartRunning();
	}
#endif
	/*****for local audio push*****/
	while (1)
	{
		/* Use the encoder's desired frame size for processing. */
		const int outputFrameSize = mStreamInfo.outputCodecContext->frame_size;
		int finished = 0;
		mTimeBeforeBlocking = high_resolution_clock::now();
		/* Make sure that there is one frame worth of samples in the FIFO
		* buffer so that the encoder can do its work.
		* Since the decoder's and the encoder's frame size may differ, we
		* need to FIFO buffer to store as many frames worth of input samples
		* that they make up at least one frame worth of output samples. */
		//printf("before ReadDecodeConvertAndStore()\n");
		while (av_audio_fifo_size(mStreamInfo.fifo) < outputFrameSize) {
			/* Decode one frame worth of audio samples, convert it to the
			* output sample format and put it into the FIFO buffer. */
			if (ReadDecodeConvertAndStore(mStreamInfo.fifo,
				mStreamInfo.inputFormatContext,
				mStreamInfo.inputCodecContext,
				mStreamInfo.outputCodecContext,
				mStreamInfo.resampleContext, &finished))
			{
				/*s_audio.InStop();
				LNGPush->StopRunning();*/
				return false;
			}

			/* If we are at the end of the input file, we continue
			* encoding the remaining audio samples to the output file. */
			if (finished)
			{
				s_audio.InStop();
				LNGPush->StopRunning();
				return false;
			}
		}
		//printf("before LoadEncodeAndWrite()\n");
		/* If we have enough samples for the encoder, we encode them.
		* At the end of the file, we pass the remaining samples to
		* the encoder. */
		while (av_audio_fifo_size(mStreamInfo.fifo) >= outputFrameSize ||
			(finished && av_audio_fifo_size(mStreamInfo.fifo) > 0))
			/* Take one frame worth of audio samples from the FIFO buffer,
			* encode it and write it to the output file. */
			if (LoadEncodeAndWrite(mStreamInfo.fifo,
				mStreamInfo.outputFormatContext,
				mStreamInfo.outputCodecContext))
			{
				s_audio.InStop();
				LNGPush->StopRunning();
				return false;
			}

		/* If we are at the end of the input file and have encoded
		* all remaining samples, we can exit this loop and finish. */
		if (finished) {
			int dataWritten;
			/* Flush the encoder as it may have delayed frames. */
			do {
				dataWritten = 0;
				if (EncodeAudioFrame(NULL,
					mStreamInfo.outputFormatContext,
					mStreamInfo.outputCodecContext, &dataWritten))
				{
					s_audio.InStop();
					LNGPush->StopRunning();
					return false;
				}
			} while (dataWritten);
			s_audio.InStop();
			LNGPush->StopRunning();
			return true;
		}
		if (bQuitSubThread)
			break;
	}
	s_audio.InStop();
	LNGPush->StopRunning();
	return true;
}

bool AudioTranscode::Init(const char *inFilename, const char *outFilename, const char* localNodeName)
{
	strcpy(mInFilename, inFilename);
	strcpy(mOutFilename, outFilename);
	strcpy(mLocalNodeName, localNodeName);
	mStreamInfo.fifo = NULL;
	mStreamInfo.inputCodecContext = NULL;
	mStreamInfo.inputFormatContext = avformat_alloc_context(); //NULL;
	mStreamInfo.outputCodecContext = NULL;
	mStreamInfo.outputFormatContext = NULL;
	mStreamInfo.resampleContext = NULL;
	avdevice_register_all();
	if (OpenInputFile(&mStreamInfo.inputFormatContext,
		              &mStreamInfo.inputCodecContext))
	{
		printf("AudioTranscode@Init(): 打开输入失败，请检查！\n");
		return false;
	}
	if (OpenOutputFile(mStreamInfo.inputCodecContext,
		               &mStreamInfo.outputFormatContext,
		               &mStreamInfo.outputCodecContext))
	{
		printf("AudioTranscode@Init(): 打开输出失败，请检查！\n");
		return false;
	}
	/* Initialize the resampler to be able to convert audio sample formats. */
	if (InitResampler(mStreamInfo.inputCodecContext,
		              mStreamInfo.outputCodecContext,
		              &mStreamInfo.resampleContext))
	{
		printf("AudioTranscode@Init(): 初始化采样器失败，请检查！\n");
		return false;
	}
	/* Initialize the FIFO buffer to store audio samples to be encoded. */
	if (InitFIFO(&mStreamInfo.fifo, mStreamInfo.outputCodecContext))
	{
		printf("AudioTranscode@Init(): 初始化FIFO缓存失败，请检查！\n");
		return false;
	}
	/* Write the header of the output file container. */
	if (WriteOutputFileHeader(mStreamInfo.outputFormatContext))
	{
		printf("AudioTranscode@Init(): 输出帧头写入失败，请检查！\n");
		return false;
	}

	mPts = 0;
	bQuitSubThread = false;
	return true;
}

int AudioInterruptCallback(void *ctx)
{
	time_point<system_clock, system_clock::duration>* pTimeBeforeBlocking = (time_point<system_clock, system_clock::duration>*)ctx;
	auto curTime = high_resolution_clock::now();
	if (duration_cast<duration<float>>(curTime - *pTimeBeforeBlocking).count() > LIBAVFORMAT_INTERRUPT_TIMEOUT_SECOND)
	{
		//printf("audio_abort%f\n", duration_cast<duration<float>>(curTime - *pTimeBeforeBlocking).count());
		return 1;
	}
	else
	{
		//printf("audio_normal_________________%f\n", duration_cast<duration<float>>(curTime - *pTimeBeforeBlocking).count());
		return 0;
	}
}

int AudioTranscode::OpenInputFile(AVFormatContext **inputFormatContext,
	                              AVCodecContext **inputCodecContext)
{
	AVCodecContext *avctx;
	AVCodec *input_codec;
	int error;

	/* Open the input file to read from it. */
	AVDictionary *optionsDict = NULL;
	av_dict_set(&optionsDict, "stimeout", "2000000", 0);
	av_dict_set(&optionsDict, "tune", "zerolatency", 0);
	av_dict_set(&optionsDict, "probesize", "32", 0);
	av_dict_set(&optionsDict, "fflags", "nobuffer", 0);
	av_dict_set(&optionsDict, "timeout", "10000000", 0);
	//av_dict_set(&optionsDict, "rtbufsize", "10000000", 0);
#if 1 // 2018.9.30 interrupt callback function for blocking functions of ffmpeg.
	(*inputFormatContext)->interrupt_callback.callback = AudioInterruptCallback;
	(*inputFormatContext)->interrupt_callback.opaque = &mTimeBeforeBlocking;
#endif
	mTimeBeforeBlocking = high_resolution_clock::now();
	AVInputFormat* pInputFormat = av_find_input_format("dshow");
	if ((error = avformat_open_input(inputFormatContext, mInFilename, pInputFormat,
		&optionsDict)) < 0) {
		char buf[256];
		av_strerror(error, buf, sizeof(buf));
		printf("Could not open input file :%s,ret:%d\n", buf, error);
		/*fprintf(stderr, "Could not open input file '%s' (error '%s')\n",
		filename, av_err2str(error));*/
		*inputFormatContext = NULL;
		return error;
	}

	/* Get information on the input file (number of streams etc.). */
	if ((error = avformat_find_stream_info(*inputFormatContext, NULL)) < 0) {
		char buf[256];
		av_strerror(error, buf, sizeof(buf));
		printf("Could not open find stream info :%s,ret:%d\n", buf, error);
		/*fprintf(stderr, "Could not open find stream info (error '%s')\n",
		av_err2str(error));*/
		avformat_close_input(inputFormatContext);
		return error;
	}
	/*Log information*/
	//av_dump_format(*inputFormatContext, 0, mInFilename, false);

	/* Make sure that there is only one stream in the input file. */
	if ((*inputFormatContext)->nb_streams != 1) {
		fprintf(stderr, "Expected one audio input stream, but found %d\n",
			(*inputFormatContext)->nb_streams);
		avformat_close_input(inputFormatContext);
		return AVERROR_EXIT;
	}

	/* Find a decoder for the audio stream. */
	if (!(input_codec = avcodec_find_decoder((*inputFormatContext)->streams[0]->codecpar->codec_id))) {
		fprintf(stderr, "Could not find input codec\n");
		avformat_close_input(inputFormatContext);
		return AVERROR_EXIT;
	}

	/* Allocate a new decoding context. */
	avctx = avcodec_alloc_context3(input_codec);
	if (!avctx) {
		fprintf(stderr, "Could not allocate a decoding context\n");
		avformat_close_input(inputFormatContext);
		return AVERROR(ENOMEM);
	}

	/* Initialize the stream parameters with demuxer information. */
	error = avcodec_parameters_to_context(avctx, (*inputFormatContext)->streams[0]->codecpar);
	if (error < 0) {
		avformat_close_input(inputFormatContext);
		avcodec_free_context(&avctx);
		return error;
	}

	/* Open the decoder for the audio stream to use it later. */
	if ((error = avcodec_open2(avctx, input_codec, NULL)) < 0) {
		char buf[256];
		av_strerror(error, buf, sizeof(buf));
		printf("Could not open input codec :%s,ret:%d\n", buf, error);
		/*fprintf(stderr, "Could not open input codec (error '%s')\n",
		av_err2str(error));*/
		avcodec_free_context(&avctx);
		avformat_close_input(inputFormatContext);
		return error;
	}

	/* Save the decoder context for easier access later. */
	*inputCodecContext = avctx;

	return 0;
}

int AudioTranscode::OpenOutputFile(AVCodecContext *inputCodecContext,
	                               AVFormatContext **outputFormatContext,
	                               AVCodecContext **outputCodecContext)
{
	AVCodecContext *avctx = NULL;
	AVIOContext *outputIOContext = NULL;
	AVStream *stream = NULL;
	AVCodec *outputCodec = NULL;
	int error;

	/* Open the output file to write to it. */
	if ((error = avio_open(&outputIOContext, mOutFilename,
		AVIO_FLAG_WRITE)) < 0) {
		char buf[256];
		av_strerror(error, buf, sizeof(buf));
		printf("Could not open output file :%s,ret:%d\n", buf, error);
		/*fprintf(stderr, "Could not open output file '%s' (error '%s')\n",
		filename, av_err2str(error));*/
		return error;
	}

	/* Create a new format context for the output container format. */
	if (!(*outputFormatContext = avformat_alloc_context())) {
		fprintf(stderr, "Could not allocate output format context\n");
		return AVERROR(ENOMEM);
	}

	/* Associate the output file (pointer) with the container format context. */
	(*outputFormatContext)->pb = outputIOContext;

	/* Guess the desired container format based on the file extension. */
	if (!((*outputFormatContext)->oformat = av_guess_format("flv", NULL,    //av_guess_format(NULL, filename, NULL)
		NULL))) {
		fprintf(stderr, "Could not find output file format\n");
		goto cleanup;
	}

	if (!((*outputFormatContext)->url = av_strdup(mOutFilename))) {
		fprintf(stderr, "Could not allocate url.\n");
		error = AVERROR(ENOMEM);
		goto cleanup;
	}

	/* Find the encoder to be used by its name. */
	if (!(outputCodec = avcodec_find_encoder(AV_CODEC_ID_AAC))) {
		fprintf(stderr, "Could not find an AAC encoder.\n");
		goto cleanup;
	}

	/* Create a new audio stream in the output file container. */
	if (!(stream = avformat_new_stream(*outputFormatContext, NULL))) {
		fprintf(stderr, "Could not create new stream\n");
		error = AVERROR(ENOMEM);
		goto cleanup;
	}

	avctx = avcodec_alloc_context3(outputCodec);
	if (!avctx) {
		fprintf(stderr, "Could not allocate an encoding context\n");
		error = AVERROR(ENOMEM);
		goto cleanup;
	}

	/* Set the basic encoder parameters.
	* The input file's sample rate is used to avoid a sample rate conversion. */
	avctx->channels = OUTPUT_CHANNELS;
	avctx->channel_layout = av_get_default_channel_layout(OUTPUT_CHANNELS);
	avctx->sample_rate = inputCodecContext->sample_rate;
	avctx->sample_fmt = outputCodec->sample_fmts[0];
	avctx->bit_rate = OUTPUT_BIT_RATE;

	/* Allow the use of the experimental AAC encoder. */
	avctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

	/* Set the sample rate for the container. */
	stream->time_base.den = inputCodecContext->sample_rate;
	stream->time_base.num = 1;

	/* Some container formats (like MP4) require global headers to be present.
	* Mark the encoder so that it behaves accordingly. */
	if ((*outputFormatContext)->oformat->flags & AVFMT_GLOBALHEADER)
		avctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	/* Open the encoder for the audio stream to use it later. */
	if ((error = avcodec_open2(avctx, outputCodec, NULL)) < 0) {
		char buf[256];
		av_strerror(error, buf, sizeof(buf));
		printf("Could not open output codec :%s,ret:%d\n", buf, error);
		/*fprintf(stderr, "Could not open output codec (error '%s')\n",
		av_err2str(error));*/
		goto cleanup;
	}

	error = avcodec_parameters_from_context(stream->codecpar, avctx);
	if (error < 0) {
		fprintf(stderr, "Could not initialize stream parameters\n");
		goto cleanup;
	}

	/* Save the encoder context for easier access later. */
	*outputCodecContext = avctx;

	return 0;

cleanup:
	avcodec_free_context(&avctx);
	avio_closep(&(*outputFormatContext)->pb);
	avformat_free_context(*outputFormatContext);
	*outputFormatContext = NULL;
	return error < 0 ? error : AVERROR_EXIT;
}

void AudioTranscode::InitPacket(AVPacket *packet)
{
	av_init_packet(packet);
	/* Set the packet data and size so that it is recognized as being empty. */
	packet->data = NULL;
	packet->size = 0;
}

int AudioTranscode::InitInputFrame(AVFrame **frame)
{
	if (!(*frame = av_frame_alloc())) {
		fprintf(stderr, "Could not allocate input frame\n");
		return AVERROR(ENOMEM);
	}
	return 0;
}

int AudioTranscode::InitResampler(AVCodecContext *inputCodecContext,
	                              AVCodecContext *outputCodecContext,
	                              SwrContext **resampleContext)
{
	int error;

	/*
	* Create a resampler context for the conversion.
	* Set the conversion parameters.
	* Default channel layouts based on the number of channels
	* are assumed for simplicity (they are sometimes not detected
	* properly by the demuxer and/or decoder).
	*/
	*resampleContext = swr_alloc_set_opts(NULL,
		av_get_default_channel_layout(outputCodecContext->channels),
		outputCodecContext->sample_fmt,
		outputCodecContext->sample_rate,
		av_get_default_channel_layout(inputCodecContext->channels),
		inputCodecContext->sample_fmt,
		inputCodecContext->sample_rate,
		0, NULL);
	if (!*resampleContext) {
		fprintf(stderr, "Could not allocate resample context\n");
		return AVERROR(ENOMEM);
	}
	/*
	* Perform a sanity check so that the number of converted samples is
	* not greater than the number of samples to be converted.
	* If the sample rates differ, this case has to be handled differently
	*/
	av_assert0(outputCodecContext->sample_rate == inputCodecContext->sample_rate);

	/* Open the resampler with the specified parameters. */
	if ((error = swr_init(*resampleContext)) < 0) {
		fprintf(stderr, "Could not open resample context\n");
		swr_free(resampleContext);
		return error;
	}
	return 0;
}

int AudioTranscode::InitFIFO(AVAudioFifo **fifo, AVCodecContext *outputCodecContext)
{
	/* Create the FIFO buffer based on the specified output sample format. */
	if (!(*fifo = av_audio_fifo_alloc(outputCodecContext->sample_fmt,
		outputCodecContext->channels, 1))) {
		fprintf(stderr, "Could not allocate FIFO\n");
		return AVERROR(ENOMEM);
	}
	return 0;
}

int AudioTranscode::WriteOutputFileHeader(AVFormatContext *outputFormatContext)
{
	int error;
	error = avformat_write_header(outputFormatContext, NULL);
	if (error < 0) {
		char buf[256];
		av_strerror(error, buf, sizeof(buf));
		printf("Could not write output file header :%s,ret:%d\n", buf, error);
		return error;
	}
	return 0;
}

int AudioTranscode::DecodeAudioFrame(AVFrame *frame,
	                                 AVFormatContext *inputFormatContext,
									 AVCodecContext *inputCodecContext,
									 int *dataPresent, int *finished)
{
	/* Packet used for temporary storage. */
	AVPacket inputPacket;
	int error;
	InitPacket(&inputPacket);

	/* Read one audio frame from the input file into a temporary packet. */
	if ((error = av_read_frame(inputFormatContext, &inputPacket)) < 0) {
		/* If we are at the end of the file, flush the decoder below. */
		if (error == AVERROR_EOF)
			*finished = 1;
		else {
			char buf[256];
			av_strerror(error, buf, sizeof(buf));
			printf("Could not read frame :%s,ret:%d\n", buf, error);
			/*fprintf(stderr, "Could not read frame (error '%s')\n",
			av_err2str(error));*/
			return error;
		}
	}

	/* Send the audio frame stored in the temporary packet to the decoder.
	* The input audio stream decoder is used to do this. */
	if ((error = avcodec_send_packet(inputCodecContext, &inputPacket)) < 0) {
		char buf[256];
		av_strerror(error, buf, sizeof(buf));
		printf("Could not send packet for decoding :%s,ret:%d\n", buf, error);
		/*fprintf(stderr, "Could not send packet for decoding (error '%s')\n",
		av_err2str(error));*/
		return error;
	}

	/* Receive one frame from the decoder. */
	error = avcodec_receive_frame(inputCodecContext, frame);
	/* If the decoder asks for more data to be able to decode a frame,
	* return indicating that no data is present. */
	if (error == AVERROR(EAGAIN)) {
		error = 0;
		goto cleanup;
		/* If the end of the input file is reached, stop decoding. */
	}
	else if (error == AVERROR_EOF) {
		*finished = 1;
		error = 0;
		goto cleanup;
	}
	else if (error < 0) {
		char buf[256];
		av_strerror(error, buf, sizeof(buf));
		printf("Could not decode frame :%s,ret:%d\n", buf, error);
		/*fprintf(stderr, "Could not decode frame (error '%s')\n",
		av_err2str(error));*/
		goto cleanup;
		/* Default case: Return decoded data. */
	}
	else {
		*dataPresent = 1;
		goto cleanup;
	}

cleanup:
	av_packet_unref(&inputPacket);
	return error;
}

int AudioTranscode::InitConvertedSamples(uint8_t ***convertedInputSamples,
	                                     AVCodecContext *outputCodecContext,
	                                     int frameSize)
{
	int error;

	/* Allocate as many pointers as there are audio channels.
	* Each pointer will later point to the audio samples of the corresponding
	* channels (although it may be NULL for interleaved formats).
	*/
	if (!(*convertedInputSamples = (uint8_t**)calloc(outputCodecContext->channels,
		sizeof(**convertedInputSamples)))) {
		fprintf(stderr, "Could not allocate converted input sample pointers\n");
		return AVERROR(ENOMEM);
	}

	/* Allocate memory for the samples of all channels in one consecutive
	* block for convenience. */
	if ((error = av_samples_alloc(*convertedInputSamples, NULL,
		outputCodecContext->channels,
		frameSize,
		outputCodecContext->sample_fmt, 0)) < 0) {
		char buf[256];
		av_strerror(error, buf, sizeof(buf));
		printf("Could not allocate converted input samples :%s,ret:%d\n", buf, error);
		/*fprintf(stderr,
		"Could not allocate converted input samples (error '%s')\n",
		av_err2str(error));*/
		av_freep(&(*convertedInputSamples)[0]);
		free(*convertedInputSamples);
		return error;
	}
	return 0;
}

int AudioTranscode::ConvertSamples(const uint8_t **inputData,
	                               uint8_t **convertedData, 
								   const int frameSize,
	                               SwrContext *resampleContext)
{
	int error;

	/* Convert the samples using the resampler. */
	if ((error = swr_convert(resampleContext,
		convertedData, frameSize,
		inputData, frameSize)) < 0) {
		char buf[256];
		av_strerror(error, buf, sizeof(buf));
		printf("Could not convert input samples :%s,ret:%d\n", buf, error);
		return error;
	}

	return 0;
}

int AudioTranscode::AddSamplesToFIFO(AVAudioFifo *fifo,
	                                 uint8_t **convertedInputSamples,
	                                 const int frameSize)
{
	int error;

	/* Make the FIFO as large as it needs to be to hold both,
	* the old and the new samples. */
	if ((error = av_audio_fifo_realloc(fifo, av_audio_fifo_size(fifo) + frameSize)) < 0) {
		fprintf(stderr, "Could not reallocate FIFO\n");
		return error;
	}

	/* Store the new samples in the FIFO buffer. */
	if (av_audio_fifo_write(fifo, (void **)convertedInputSamples,
		frameSize) < frameSize) {
		fprintf(stderr, "Could not write data to FIFO\n");
		return AVERROR_EXIT;
	}
	return 0;
}

int AudioTranscode::ReadDecodeConvertAndStore(AVAudioFifo *fifo,
	                                          AVFormatContext *inputFormatContext,
											  AVCodecContext *inputCodecContext,
											  AVCodecContext *outputCodecContext,
											  SwrContext *resamplerContext,
											  int *finished)
{
	/* Temporary storage of the input samples of the frame read from the file. */
	AVFrame *inputFrame = NULL;
	/* Temporary storage for the converted input samples. */
	uint8_t **convertedInputSamples = NULL;
	int dataPresent = 0;
	int ret = AVERROR_EXIT;

	/* Initialize temporary storage for one input frame. */
	if (InitInputFrame(&inputFrame))
		goto cleanup;
	/* Decode one frame worth of audio samples. */
	if (DecodeAudioFrame(inputFrame, inputFormatContext,
		inputCodecContext, &dataPresent, finished))
		goto cleanup;
	/* If we are at the end of the file and there are no more samples
	* in the decoder which are delayed, we are actually finished.
	* This must not be treated as an error. */
	if (*finished) {
		ret = 0;
		goto cleanup;
	}
	/* If there is decoded data, convert and store it. */
	if (dataPresent) {
		/* Initialize the temporary storage for the converted input samples. */
		if (InitConvertedSamples(&convertedInputSamples, outputCodecContext,
			inputFrame->nb_samples))
			goto cleanup;

		/* Convert the input samples to the desired output sample format.
		* This requires a temporary storage provided by converted_input_samples. */
		if (ConvertSamples((const uint8_t**)inputFrame->extended_data, convertedInputSamples,
			inputFrame->nb_samples, resamplerContext))
			goto cleanup;

		/* Add the converted input samples to the FIFO buffer for later processing. */
		if (AddSamplesToFIFO(fifo, convertedInputSamples,
			inputFrame->nb_samples))
			goto cleanup;
		ret = 0;
	}
	ret = 0;

cleanup:
	if (convertedInputSamples) {
		av_freep(&convertedInputSamples[0]);
		free(convertedInputSamples);
	}
	av_frame_free(&inputFrame);

	return ret;
}

int AudioTranscode::InitOutputFrame(AVFrame **frame,
	                                AVCodecContext *outputCodecContext,
	                                int frameSize)
{
	int error;

	/* Create a new frame to store the audio samples. */
	if (!(*frame = av_frame_alloc())) {
		fprintf(stderr, "Could not allocate output frame\n");
		return AVERROR_EXIT;
	}

	/* Set the frame's parameters, especially its size and format.
	* av_frame_get_buffer needs this to allocate memory for the
	* audio samples of the frame.
	* Default channel layouts based on the number of channels
	* are assumed for simplicity. */
	(*frame)->nb_samples = frameSize;
	(*frame)->channel_layout = outputCodecContext->channel_layout;
	(*frame)->format = outputCodecContext->sample_fmt;
	(*frame)->sample_rate = outputCodecContext->sample_rate;

	/* Allocate the samples of the created frame. This call will make
	* sure that the audio frame can hold as many samples as specified. */
	if ((error = av_frame_get_buffer(*frame, 0)) < 0) {
		char buf[256];
		av_strerror(error, buf, sizeof(buf));
		printf("Could not allocate output frame samples :%s,ret:%d\n", buf, error);
		av_frame_free(frame);
		return error;
	}

	return 0;
}

int AudioTranscode::EncodeAudioFrame(AVFrame *frame,
	                                 AVFormatContext *outputFormatContext,
	                                 AVCodecContext *outputCodecContext,
	                                 int *dataPresent)
{
	/* Packet used for temporary storage. */
	AVPacket outputPacket;
	int error;
	InitPacket(&outputPacket);

	/* Set a timestamp based on the sample rate for the container. */
	if (frame) {
		frame->pts = mPts;
		mPts += frame->nb_samples;
	}

	/* Send the audio frame stored in the temporary packet to the encoder.
	* The output audio stream encoder is used to do this. */
	error = avcodec_send_frame(outputCodecContext, frame);
	/* The encoder signals that it has nothing more to encode. */
	if (error == AVERROR_EOF) {
		error = 0;
		goto cleanup;
	}
	else if (error < 0) {
		char buf[256];
		av_strerror(error, buf, sizeof(buf));
		printf("Could not send packet for encoding :%s,ret:%d\n", buf, error);
		/*fprintf(stderr, "Could not send packet for encoding (error '%s')\n",
		av_err2str(error));*/
		return error;
	}

	/* Receive one encoded frame from the encoder. */
	error = avcodec_receive_packet(outputCodecContext, &outputPacket);
	/* If the encoder asks for more data to be able to provide an
	* encoded frame, return indicating that no data is present. */
	if (error == AVERROR(EAGAIN)) {
		error = 0;
		goto cleanup;
		/* If the last frame has been encoded, stop encoding. */
	}
	else if (error == AVERROR_EOF) {
		error = 0;
		goto cleanup;
	}
	else if (error < 0) {
		char buf[256];
		av_strerror(error, buf, sizeof(buf));
		printf("Could not encode frame :%s,ret:%d\n", buf, error);
		/*fprintf(stderr, "Could not encode frame (error '%s')\n",
		av_err2str(error));*/
		goto cleanup;
		/* Default case: Return encoded data. */
	}
	else {
		*dataPresent = 1;
	}

	/* Write one audio frame from the temporary packet to the output file. */
	if (*dataPresent &&
		(error = av_write_frame(outputFormatContext, &outputPacket)) < 0) {
		char buf[256];
		av_strerror(error, buf, sizeof(buf));
		printf("Could not write frame :%s,ret:%d\n", buf, error);
		/*fprintf(stderr, "Could not write frame (error '%s')\n",
		av_err2str(error));*/
		goto cleanup;
	}

cleanup:
	av_packet_unref(&outputPacket);
	return error;
}

int AudioTranscode::LoadEncodeAndWrite(AVAudioFifo *fifo,
	                                   AVFormatContext *output_format_context,
									   AVCodecContext *output_codec_context)
{
	/* Temporary storage of the output samples of the frame written to the file. */
	AVFrame *outputFrame;
	/* Use the maximum number of possible samples per frame.
	* If there is less than the maximum possible frame size in the FIFO
	* buffer use this number. Otherwise, use the maximum possible frame size. */
	const int frameSize = FFMIN(av_audio_fifo_size(fifo),
		output_codec_context->frame_size);
	int dataWritten;

	/* Initialize temporary storage for one output frame. */
	if (InitOutputFrame(&outputFrame, output_codec_context, frameSize))
		return AVERROR_EXIT;

	/* Read as many samples from the FIFO buffer as required to fill the frame.
	* The samples are stored in the frame temporarily. */
	if (av_audio_fifo_read(fifo, (void **)outputFrame->data, frameSize) < frameSize) {
		fprintf(stderr, "Could not read data from FIFO\n");
		av_frame_free(&outputFrame);
		return AVERROR_EXIT;
	}

	/* Encode one frame worth of audio samples. */
	if (EncodeAudioFrame(outputFrame, output_format_context,
		output_codec_context, &dataWritten)) {
		av_frame_free(&outputFrame);
		return AVERROR_EXIT;
	}
	av_frame_free(&outputFrame);
	return 0;
}

int AudioTranscode::WriteOutputFileTrailer(AVFormatContext *outputFormatContext)
{
	int error;
	if ((error = av_write_trailer(outputFormatContext)) < 0) {
		char buf[256];
		av_strerror(error, buf, sizeof(buf));
		printf("Could not write output file trailer :%s,ret:%d\n", buf, error);
		return error;
	}
	return 0;
}