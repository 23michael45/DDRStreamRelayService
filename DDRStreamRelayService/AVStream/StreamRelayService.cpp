#include <Windows.h>
#include "StreamRelayService.h"
#include <thread>
#include <mutex>


#define high_resolution_clock system_clock  // xcy add. In order to compile at vs2017

#define LIBAVFORMAT_INTERRUPT_TIMEOUT_SECOND 30.f

std::mutex mutexInitParams;

StreamRelayService::StreamRelayService(int nSubThread)
	: MainSubsModel(nSubThread, 0, 100)
{
	nService = nSubThread;
}

StreamRelayService::~StreamRelayService()
{
	//avformat_close_input(&inFormatContext);

	///* close output */
	//if (outFormatContext && !(outFormat->flags & AVFMT_NOFILE))
	//	avio_closep(&outFormatContext->pb);
	//avformat_free_context(outFormatContext);

	//av_freep(&streamMapping);
#if 0 // test multithread init
	Quit();
#else
	EndSubThreads();
#endif
}

void StreamRelayService::Quit()
{
	EndSubThreads();
	if (bInitialized)
	{
		for (int i = 0; i < streamsInfo.size(); i++)
		{
			av_write_trailer(streamsInfo[i].outFormatContext);
		}
	}
	for (int i = 0; i < streamsInfo.size(); i++)
	{
		avformat_close_input(&streamsInfo[i].inFormatContext);

		/* close output */
		if (streamsInfo[i].outFormatContext && !(streamsInfo[i].outFormat->flags & AVFMT_NOFILE))
			avio_closep(&streamsInfo[i].outFormatContext->pb);
		avformat_free_context(streamsInfo[i].outFormatContext);
	}
}

void StreamRelayService::Quit(StreamInfo streamInfo)
{
	if (streamInfo.bInit)
		av_write_trailer(streamInfo.outFormatContext);
	avformat_close_input(&streamInfo.inFormatContext);

	/* close output */
	if (streamInfo.outFormatContext && !(streamInfo.outFormat->flags & AVFMT_NOFILE))
		avio_closep(&streamInfo.outFormatContext->pb);
	avformat_free_context(streamInfo.outFormatContext);
}

void StreamRelayService::Reset()
{
	EndSubThreads();
	for (int i = 0; i < streamsInfo.size(); i++)
	{
		avformat_close_input(&streamsInfo[i].inFormatContext);

		/* close output */
		if (streamsInfo[i].outFormatContext && !(streamsInfo[i].outFormat->flags & AVFMT_NOFILE))
			avio_closep(&streamsInfo[i].outFormatContext->pb);
		avformat_free_context(streamsInfo[i].outFormatContext);
	}
}

void StreamRelayService::Reset(StreamInfo streamInfo)
{
	avformat_close_input(&streamInfo.inFormatContext);

	/* close output */
	if (streamInfo.outFormatContext && !(streamInfo.outFormat->flags & AVFMT_NOFILE))
		avio_closep(&streamInfo.outFormatContext->pb);
	avformat_free_context(streamInfo.outFormatContext);

	av_freep(&streamInfo.streamMapping);
}

int StreamRelayService::Init(const std::vector<std::string>& inFilenames,
	                         const std::vector<std::string>& outFilenames)
{
	mbEndSubThreads = false;
	bInitialized = false;
	if (inFilenames.size() < 1 || outFilenames.size() < 1
		|| inFilenames.size() != outFilenames.size())
	{
		printf("StreamRelayService@Init(): input names or output names are wrong!\n");
		return STREAM_INIT_ERROR_INPUT_OUTPUT_SIZE;
	}
	for (int i = 0; i < outFilenames.size() - 1; i++)
		for (int j = i + 1; j < outFilenames.size(); j++)
		{
			if (strcmp(outFilenames[i].c_str(), outFilenames[j].c_str()) == 0)
			{
				printf("StreamRelayService@Init(): The output names are duplicated!\n");
				return STREAM_INIT_ERROR_OUTPUT_NAME_DUPLICATED;
			}
		}
	mInFilenames = inFilenames;
	mOutFilenames = outFilenames;
	streamsInfo.resize(0);
#if 0
	for (int k = 0; k < inFilenames.size(); k++)
	{
		StreamInfo streamInfo;
		streamInfo.outFormat = NULL;
		streamInfo.inFormatContext = NULL;
		streamInfo.outFormatContext = NULL;
		streamInfo.streamMapping = NULL;
		streamInfo.streamMappingSize = 0;
		int ret;
		int streamIndex = 0;
		if ((ret = avformat_open_input(&streamInfo.inFormatContext, inFilenames[k], 0, 0)) < 0) {
			fprintf(stderr, "StreamRelayService@Init(): Could not open input '%s'\n", inFilenames[k]);
			return false;
		}
		if ((ret = avformat_find_stream_info(streamInfo.inFormatContext, 0)) < 0) {
			fprintf(stderr, "StreamRelayService@Init(): Failed to retrieve input stream information\n");
			return false;
		}
		av_dump_format(streamInfo.inFormatContext, 0, inFilenames[k], 0); // print some information for input

		streamInfo.streamMappingSize = streamInfo.inFormatContext->nb_streams;
		streamInfo.streamMapping = (int*)av_mallocz_array(streamInfo.streamMappingSize, sizeof(*streamInfo.streamMapping));
		if (!streamInfo.streamMapping) {
			ret = AVERROR(ENOMEM);
			return false;
		}

		avformat_alloc_output_context2(&streamInfo.outFormatContext, NULL, "flv", outFilenames[k]);
		if (!streamInfo.outFormatContext) {
			fprintf(stderr, "StreamRelayService@Init(): Could not create output context\n");
			ret = AVERROR_UNKNOWN;
			return false;
		}
		streamInfo.outFormat = streamInfo.outFormatContext->oformat;

		for (int i = 0; i < streamInfo.inFormatContext->nb_streams; i++) {
			AVStream *outStream;
			AVStream *inStream = streamInfo.inFormatContext->streams[i];
			AVCodecParameters *inCodecpar = inStream->codecpar;

			if (inCodecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
				inCodecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
				inCodecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
				streamInfo.streamMapping[i] = -1;
				continue;
			}

			streamInfo.streamMapping[i] = streamIndex++;

			outStream = avformat_new_stream(streamInfo.outFormatContext, NULL);
			if (!outStream) {
				fprintf(stderr, "StreamRelayService@Init(): Failed allocating output stream!\n");
				ret = AVERROR_UNKNOWN;
				return false;
			}

			ret = avcodec_parameters_copy(outStream->codecpar, inCodecpar);
			if (ret < 0) {
				fprintf(stderr, "StreamRelayService@Init(): Failed to copy codec parameters!\n");
				return false;
			}
			/*out_stream->codecpar->bit_rate = 128000;
			out_stream->codecpar->*/
			outStream->codecpar->codec_tag = 0;
		}
		av_dump_format(streamInfo.outFormatContext, 0, outFilenames[k], 1);// print some information for output
		if (!(streamInfo.outFormat->flags & AVFMT_NOFILE)) {
			ret = avio_open(&streamInfo.outFormatContext->pb, outFilenames[k], AVIO_FLAG_WRITE);
			if (ret < 0) {
				fprintf(stderr, "StreamRelayService@Init(): Could not open output '%s'!\n", outFilenames[k]);
				return false;
			}
		}

		ret = avformat_write_header(streamInfo.outFormatContext, NULL);
		if (ret < 0) {
			fprintf(stderr, "StreamRelayService@Init(): Error occurred when opening output!\n");
			return false;
		}
		streamInfo.ptsInd = 0;
		streamsInfo.push_back(streamInfo);
		/*ptsInd.push_back(0);
		AVPacket pkt;
		pkts.push_back(pkt);*/
	}
	bInitialized = true;
#endif
#if 0 // test multithread init
	std::vector<std::thread> initThreads;
#endif
	mTimeBeforeBlockings.resize(inFilenames.size());
	for (int k = 0; k < inFilenames.size(); k++)
	{
		auto tmpTime = high_resolution_clock::now();
		mTimeBeforeBlockings[k] = tmpTime;
#if 0 // test multithread init
		initThreads.push_back(std::thread(&StreamRelayService::InitialThread, this, inFilenames[k].c_str(), outFilenames[k].c_str(), &mTimeBeforeBlockings[k]));
#endif
	}
#if 0
	for (int k = 0; k < initThreads.size(); k++)
	{
		initThreads[k].join();
	}
#endif
	/*if (streamsInfo.size() != inFilenames.size())
		return STREAM_INIT_ERROR_WEB_QUALITY;*/
#if 0 // test multithread init
	for (int k = 0; k < streamsInfo.size(); k++)
	{
		if (!streamsInfo[k].bInit)
			return STREAM_INIT_ERROR_WEB_QUALITY;
	}
#endif
	bInitialized = true;
	return STREAM_INIT_SUCCESS;
}

int VideoInterruptCallback(void *ctx)
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

void StreamRelayService::InitialThread(const char * inFilenames,
	                                   const char * outFilenames,
	                                   time_point<system_clock, system_clock::duration>* timeBeforeBlocking,
	                                   StreamInfo& streamInfo)
{
	streamInfo.bInit = false;
	streamInfo.outFormat = NULL;
	streamInfo.inFormatContext = avformat_alloc_context(); //NULL;
	streamInfo.outFormatContext = NULL;
	streamInfo.streamMapping = NULL;
	streamInfo.streamMappingSize = 0;
	int ret;
	int streamIndex = 0;
	AVDictionary *optionsDict = NULL;
	av_dict_set(&optionsDict, "stimeout", "10000000", 0);
	av_dict_set(&optionsDict, "tune", "zerolatency", 0);
	av_dict_set(&optionsDict, "probesize", "2048", 0);
	av_dict_set(&optionsDict, "max_analyze_duration", "1000", 0);
	av_dict_set(&optionsDict, "fflags", "nobuffer", 0);
	//av_dict_set(&optionsDict, "timeout", "100000000", 0);
	//AVInputFormat* pInputFormat = av_find_input_format("dshow");
	streamInfo.inFormatContext->interrupt_callback.callback = VideoInterruptCallback;
	streamInfo.inFormatContext->interrupt_callback.opaque = timeBeforeBlocking;
	*timeBeforeBlocking = high_resolution_clock::now();
	if ((ret = avformat_open_input(&streamInfo.inFormatContext, inFilenames, 0, &optionsDict)) < 0) { //&optionsDict
		fprintf(stderr, "StreamRelayService@Init(): Could not open input '%s'\n", inFilenames);
		//bInit = false;
		Reset(streamInfo);
		return;
	}
	if ((ret = avformat_find_stream_info(streamInfo.inFormatContext, 0)) < 0) {
		fprintf(stderr, "StreamRelayService@Init(): Failed to retrieve input stream information\n");
		//bInit = false;
		Reset(streamInfo);
		return;
	}
	//av_dump_format(streamInfo.inFormatContext, 0, inFilenames, 0); // print some information for input

	streamInfo.streamMappingSize = streamInfo.inFormatContext->nb_streams;
	streamInfo.streamMapping = (int*)av_mallocz_array(streamInfo.streamMappingSize, sizeof(*streamInfo.streamMapping));
	if (!streamInfo.streamMapping) {
		ret = AVERROR(ENOMEM);
		//bInit = false;
		Reset(streamInfo);
		return;
	}

	avformat_alloc_output_context2(&streamInfo.outFormatContext, NULL, "flv", outFilenames);
	if (!streamInfo.outFormatContext) {
		fprintf(stderr, "StreamRelayService@Init(): Could not create output context\n");
		ret = AVERROR_UNKNOWN;
		//bInit = false;
		Reset(streamInfo);
		return;
	}
	streamInfo.outFormat = streamInfo.outFormatContext->oformat;

	for (int i = 0; i < streamInfo.inFormatContext->nb_streams; i++) {
		AVStream *outStream;
		AVStream *inStream = streamInfo.inFormatContext->streams[i];
		AVCodecParameters *inCodecpar = inStream->codecpar;

		if (inCodecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
			inCodecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
			inCodecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
			streamInfo.streamMapping[i] = -1;
			continue;
		}

		streamInfo.streamMapping[i] = streamIndex++;

		outStream = avformat_new_stream(streamInfo.outFormatContext, NULL);
		if (!outStream) {
			fprintf(stderr, "StreamRelayService@Init(): Failed allocating output stream!\n");
			ret = AVERROR_UNKNOWN;
			//bInit = false;
			Reset(streamInfo);
			return;
		}

		ret = avcodec_parameters_copy(outStream->codecpar, inCodecpar);
		if (ret < 0) {
			fprintf(stderr, "StreamRelayService@Init(): Failed to copy codec parameters!\n");
			//bInit = false;
			return;
		}
		/*out_stream->codecpar->bit_rate = 128000;
		out_stream->codecpar->*/
		outStream->codecpar->codec_tag = 0;
	}
	//av_dump_format(streamInfo.outFormatContext, 0, outFilenames, 1);// print some information for output
	if (!(streamInfo.outFormat->flags & AVFMT_NOFILE)) {
		ret = avio_open(&streamInfo.outFormatContext->pb, outFilenames, AVIO_FLAG_WRITE);
		if (ret < 0) {
			fprintf(stderr, "StreamRelayService@Init(): Could not open output '%s'!\n", outFilenames);
			//bInit = false;
			Reset(streamInfo);
			return;
		}
	}

	ret = avformat_write_header(streamInfo.outFormatContext, NULL);
	if (ret < 0) {
		fprintf(stderr, "StreamRelayService@Init(): Error occurred when opening output!\n");
		//bInit = false;
		Reset(streamInfo);
		return;
	}
	streamInfo.ptsInd = 0;
	streamInfo.bInit = true;
}

void StreamRelayService::InitialThread(const char * inFilenames,
	                                   const char * outFilenames,
									   time_point<system_clock, system_clock::duration>* timeBeforeBlocking)
{
	StreamInfo streamInfo;
	streamInfo.bInit = false;
	streamInfo.outFormat = NULL;
	streamInfo.inFormatContext = avformat_alloc_context(); //NULL;
	streamInfo.outFormatContext = NULL;
	streamInfo.streamMapping = NULL;
	streamInfo.streamMappingSize = 0;
	int ret;
	int streamIndex = 0;
	AVDictionary *optionsDict = NULL;
	av_dict_set(&optionsDict, "stimeout", "10000000", 0);
	av_dict_set(&optionsDict, "tune", "zerolatency", 0);
	av_dict_set(&optionsDict, "probesize", "2048", 0);
	av_dict_set(&optionsDict, "max_analyze_duration", "1000", 0);
	av_dict_set(&optionsDict, "fflags", "nobuffer", 0);
	//av_dict_set(&optionsDict, "timeout", "100000000", 0);
	//AVInputFormat* pInputFormat = av_find_input_format("dshow");
	streamInfo.inFormatContext->interrupt_callback.callback = VideoInterruptCallback;
	streamInfo.inFormatContext->interrupt_callback.opaque = timeBeforeBlocking;
	*timeBeforeBlocking = high_resolution_clock::now();
	if ((ret = avformat_open_input(&streamInfo.inFormatContext, inFilenames, 0, &optionsDict)) < 0) { //&optionsDict
		fprintf(stderr, "StreamRelayService@Init(): Could not open input '%s'\n", inFilenames);
		//bInit = false;
		Reset(streamInfo);
		return;
	}
	if ((ret = avformat_find_stream_info(streamInfo.inFormatContext, 0)) < 0) {
		fprintf(stderr, "StreamRelayService@Init(): Failed to retrieve input stream information\n");
		//bInit = false;
		Reset(streamInfo);
		return;
	}
	//av_dump_format(streamInfo.inFormatContext, 0, inFilenames, 0); // print some information for input

	streamInfo.streamMappingSize = streamInfo.inFormatContext->nb_streams;
	streamInfo.streamMapping = (int*)av_mallocz_array(streamInfo.streamMappingSize, sizeof(*streamInfo.streamMapping));
	if (!streamInfo.streamMapping) {
		ret = AVERROR(ENOMEM);
		//bInit = false;
		Reset(streamInfo);
		return;
	}

	avformat_alloc_output_context2(&streamInfo.outFormatContext, NULL, "flv", outFilenames);
	if (!streamInfo.outFormatContext) {
		fprintf(stderr, "StreamRelayService@Init(): Could not create output context\n");
		ret = AVERROR_UNKNOWN;
		//bInit = false;
		Reset(streamInfo);
		return;
	}
	streamInfo.outFormat = streamInfo.outFormatContext->oformat;

	for (int i = 0; i < streamInfo.inFormatContext->nb_streams; i++) {
		AVStream *outStream;
		AVStream *inStream = streamInfo.inFormatContext->streams[i];
		AVCodecParameters *inCodecpar = inStream->codecpar;

		if (inCodecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
			inCodecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
			inCodecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
			streamInfo.streamMapping[i] = -1;
			continue;
		}

		streamInfo.streamMapping[i] = streamIndex++;

		outStream = avformat_new_stream(streamInfo.outFormatContext, NULL);
		if (!outStream) {
			fprintf(stderr, "StreamRelayService@Init(): Failed allocating output stream!\n");
			ret = AVERROR_UNKNOWN;
			//bInit = false;
			Reset(streamInfo);
			return;
		}

		ret = avcodec_parameters_copy(outStream->codecpar, inCodecpar);
		if (ret < 0) {
			fprintf(stderr, "StreamRelayService@Init(): Failed to copy codec parameters!\n");
			//bInit = false;
			return;
		}
		/*out_stream->codecpar->bit_rate = 128000;
		out_stream->codecpar->*/
		outStream->codecpar->codec_tag = 0;
	}
	//av_dump_format(streamInfo.outFormatContext, 0, outFilenames, 1);// print some information for output
	if (!(streamInfo.outFormat->flags & AVFMT_NOFILE)) {
		ret = avio_open(&streamInfo.outFormatContext->pb, outFilenames, AVIO_FLAG_WRITE);
		if (ret < 0) {
			fprintf(stderr, "StreamRelayService@Init(): Could not open output '%s'!\n", outFilenames);
			//bInit = false;
			Reset(streamInfo);
			return;
		}
	}

	ret = avformat_write_header(streamInfo.outFormatContext, NULL);
	if (ret < 0) {
		fprintf(stderr, "StreamRelayService@Init(): Error occurred when opening output!\n");
		//bInit = false;
		Reset(streamInfo);
		return;
	}
	streamInfo.ptsInd = 0;
	streamInfo.bInit = true;
	mutexInitParams.lock();
	streamsInfo.push_back(streamInfo);
	mutexInitParams.unlock();
	//bInit = true;
}

void StreamRelayService::QuitSubThreads()
{
	mbEndSubThreads = true;
	//Sleep(1000);
}

bool StreamRelayService::Respond()
{
	return MainSubsModel::Process() == 0 ? false : true;
}

bool StreamRelayService::RelayServiceThread(int subThreadID)
{
	while (1)
	{
		AVStream *inStream, *outStream;
		mTimeBeforeBlockings[subThreadID] = high_resolution_clock::now();
		int ret = av_read_frame(streamsInfo[subThreadID].inFormatContext, &streamsInfo[subThreadID].pkt);
		if (ret < 0)
		{
			mbEndSubThreads = true;
			return false;
		}

		inStream = streamsInfo[subThreadID].inFormatContext->streams[streamsInfo[subThreadID].pkt.stream_index];
		if (streamsInfo[subThreadID].pkt.stream_index >= streamsInfo[subThreadID].streamMappingSize ||
			streamsInfo[subThreadID].streamMapping[streamsInfo[subThreadID].pkt.stream_index] < 0) {
			av_packet_unref(&streamsInfo[subThreadID].pkt);
			//return true;
			continue;
		}

		streamsInfo[subThreadID].pkt.stream_index = streamsInfo[subThreadID].streamMapping[streamsInfo[subThreadID].pkt.stream_index];
		outStream = streamsInfo[subThreadID].outFormatContext->streams[streamsInfo[subThreadID].pkt.stream_index];
		streamsInfo[subThreadID].pkt.pts = streamsInfo[subThreadID].ptsInd++;
		streamsInfo[subThreadID].pkt.dts = streamsInfo[subThreadID].pkt.pts;
		streamsInfo[subThreadID].pkt.duration = av_rescale_q(streamsInfo[subThreadID].pkt.duration, inStream->time_base, outStream->time_base);
		streamsInfo[subThreadID].pkt.pos = -1;
		ret = av_interleaved_write_frame(streamsInfo[subThreadID].outFormatContext, &streamsInfo[subThreadID].pkt);
		if (ret < 0) {
			fprintf(stderr, "Error muxing packet\n");
			mbEndSubThreads = true;
			return false;
		}
		av_packet_unref(&streamsInfo[subThreadID].pkt);
		//return true;
		if (mbEndSubThreads)
			return true;
	}
	return true;
}

bool StreamRelayService::IsAllQuit()
{
	mbAllQuit = true;
	for (int i = 0; i < mbQuits.size(); i++)
	{
		mutexInitParams.lock();
		mbAllQuit = mbQuits[i];
		mutexInitParams.unlock();
	}
	return mbAllQuit;
}

bool StreamRelayService::_onRunOnce_Sub(int subThreadID)
{
#if 1 // test multithread init
	StreamInfo streamInfo;
	InitialThread(mInFilenames[subThreadID].c_str(), 
		          mOutFilenames[subThreadID].c_str(), 
				  &mTimeBeforeBlockings[subThreadID], 
				  streamInfo);
	if (!streamInfo.bInit)
		return false;
	/*mutexInitParams.lock();
	mbQuits.push_back(false);
	mutexInitParams.unlock();*/
#endif
	while (1)
	{
		AVStream *inStream, *outStream;
		mTimeBeforeBlockings[subThreadID] = high_resolution_clock::now();
#if 0 // test multithread init
		int ret = av_read_frame(streamsInfo[subThreadID].inFormatContext, &streamsInfo[subThreadID].pkt);
		if (ret < 0)
		{
			mbEndSubThreads = true;
			return false;
		}

		inStream = streamsInfo[subThreadID].inFormatContext->streams[streamsInfo[subThreadID].pkt.stream_index];
		if (streamsInfo[subThreadID].pkt.stream_index >= streamsInfo[subThreadID].streamMappingSize ||
			streamsInfo[subThreadID].streamMapping[streamsInfo[subThreadID].pkt.stream_index] < 0) {
			av_packet_unref(&streamsInfo[subThreadID].pkt);
			//return true;
			continue;
		}

		streamsInfo[subThreadID].pkt.stream_index = streamsInfo[subThreadID].streamMapping[streamsInfo[subThreadID].pkt.stream_index];
		outStream = streamsInfo[subThreadID].outFormatContext->streams[streamsInfo[subThreadID].pkt.stream_index];
		streamsInfo[subThreadID].pkt.pts = streamsInfo[subThreadID].ptsInd++;
		streamsInfo[subThreadID].pkt.dts = streamsInfo[subThreadID].pkt.pts;
		streamsInfo[subThreadID].pkt.duration = av_rescale_q(streamsInfo[subThreadID].pkt.duration, inStream->time_base, outStream->time_base);
		streamsInfo[subThreadID].pkt.pos = -1;
		ret = av_interleaved_write_frame(streamsInfo[subThreadID].outFormatContext, &streamsInfo[subThreadID].pkt);
		if (ret < 0) {
			fprintf(stderr, "Error muxing packet\n");
			mbEndSubThreads = true;
			return false;
		}
		av_packet_unref(&streamsInfo[subThreadID].pkt);
#else
		int ret = av_read_frame(streamInfo.inFormatContext, &streamInfo.pkt);
		if (ret < 0)
		{
			mbEndSubThreads = true;
			Quit(streamInfo);
			/*mutexInitParams.lock();
			mbQuits[subThreadID] = true;
			mutexInitParams.unlock();*/
			return false;
		}

		inStream = streamInfo.inFormatContext->streams[streamInfo.pkt.stream_index];
		if (streamInfo.pkt.stream_index >= streamInfo.streamMappingSize ||
			streamInfo.streamMapping[streamInfo.pkt.stream_index] < 0) {
			av_packet_unref(&streamInfo.pkt);
			//return true;
			continue;
		}

		streamInfo.pkt.stream_index = streamInfo.streamMapping[streamInfo.pkt.stream_index];
		outStream = streamInfo.outFormatContext->streams[streamInfo.pkt.stream_index];
		streamInfo.pkt.pts = streamInfo.ptsInd++;
		streamInfo.pkt.dts = streamInfo.pkt.pts;
		streamInfo.pkt.duration = av_rescale_q(streamInfo.pkt.duration, inStream->time_base, outStream->time_base);
		streamInfo.pkt.pos = -1;
		ret = av_interleaved_write_frame(streamInfo.outFormatContext, &streamInfo.pkt);
		if (ret < 0) {
			fprintf(stderr, "Error muxing packet\n");
			mbEndSubThreads = true;
			Quit(streamInfo);
			/*mutexInitParams.lock();
			mbQuits[subThreadID] = true;
			mutexInitParams.unlock();*/
			return false;
		}
		av_packet_unref(&streamInfo.pkt);
#endif
		//return true;
		if (mbEndSubThreads)
		{
			Quit(streamInfo);
			/*mutexInitParams.lock();
			mbQuits[subThreadID] = true;
			mutexInitParams.unlock();*/
			return true;
		}
	}
	Quit(streamInfo);
	/*mutexInitParams.lock();
	mbQuits[subThreadID] = true;
	mutexInitParams.unlock();*/
	return true;
}

void StreamRelayService::Start()
{
	std::vector<std::thread> RelayServiceThreads;
	for (int k = 0; k < streamsInfo.size(); k++)
	{
		auto tmpTime = high_resolution_clock::now();
		mTimeBeforeBlockings[k] = tmpTime;
		RelayServiceThreads.push_back(std::thread(&StreamRelayService::RelayServiceThread, this, k));
	}
	for (int k = 0; k < RelayServiceThreads.size(); k++)
	{
		RelayServiceThreads[k].join();
	}
}