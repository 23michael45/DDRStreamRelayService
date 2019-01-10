#ifndef __STREAM_RELAY_SERVICE_H_INCLUDED__
#define __STREAM_RELAY_SERVICE_H_INCLUDED__

#define __STDC_CONSTANT_MACROS

#define STREAM_INIT_ERROR_INPUT_OUTPUT_SIZE -1
#define STREAM_INIT_ERROR_OUTPUT_NAME_DUPLICATED -2
#define STREAM_INIT_ERROR_WEB_QUALITY -3
#define STREAM_INIT_SUCCESS 0
#define STREAM_STATUS_ERROR_OUTPUT_NAME_DUPLICATED -999

#ifdef _WIN32
//Windows
extern "C"
{
	//#include "libavformat/avformat.h"
	//#include "libavutil/mathematics.h"
	//#include "libavutil/time.h"

	//#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
};
#endif

#include "../AVStream/MyMTModules.h"
#include <vector>
#include <chrono>
using namespace DDRMTLib;
using namespace std::chrono;

struct StreamInfo
{
	bool bInit;
	AVOutputFormat *outFormat;
	AVFormatContext *inFormatContext;
	AVFormatContext *outFormatContext;
	int *streamMapping;
	int streamMappingSize;

	int64_t ptsInd;
	AVPacket pkt;
};

class StreamRelayService : public MainSubsModel
{
public:
	StreamRelayService(int nSubThread);
	~StreamRelayService();
	void Start();
	int Init(const std::vector<std::string>& inFilenames, 
		     const std::vector<std::string>& outFilenames);
	bool Respond();
	void Quit();
	void Reset();
	void QuitSubThreads();
	bool IsAllQuit();

protected:
	bool _onRunOnce_Sub(int subThreadID) override;

private:
	int nService;
	std::vector<StreamInfo> streamsInfo;
	std::vector<int64_t> ptsInd;
	std::vector<AVPacket> pkts;
	bool bInitialized;
	bool mbEndSubThreads;
	std::vector<std::string> mInFilenames, mOutFilenames;

	std::vector<time_point<system_clock, system_clock::duration>> mTimeBeforeBlockings;
	bool mbAllQuit;
	std::vector<bool> mbQuits;

	void Reset(StreamInfo streamInfo);
	void Quit(StreamInfo streamInfo);
	void InitialThread(const char * inFilenames,
		               const char * outFilenames,
					   time_point<system_clock, system_clock::duration>* timeBeforeBlocking);
	void InitialThread(const char * inFilenames,
		               const char * outFilenames,
					   time_point<system_clock, system_clock::duration>* timeBeforeBlocking,
					   StreamInfo& streamInfo);

	bool RelayServiceThread(int subThreadID);

	//int InterruptCallback(void *ctx);
};
#endif //__STREAM_RELAY_SERVICE_H_INCLUDED__