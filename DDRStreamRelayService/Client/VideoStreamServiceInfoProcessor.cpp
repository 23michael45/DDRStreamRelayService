#include "VideoStreamServiceInfoProcessor.h"
#include <memory>
#include "../../../Shared/proto/BaseCmd.pb.h"
#include "../../../Shared/src/Utility/DDRMacro.h"

#include "../../../Shared/src/Utility/Logger.h"
#include "GlobalManager.h"
#include "StreamRelayService.h"

using namespace DDRFramework;
using namespace DDRCommProto;

VideoStreamServiceInfoProcessor::VideoStreamServiceInfoProcessor(BaseMessageDispatcher& dispatcher) :BaseProcessor(dispatcher)
{ 
}


VideoStreamServiceInfoProcessor::~VideoStreamServiceInfoProcessor()
{
}

void VideoStreamServiceInfoProcessor::Process(std::shared_ptr<BaseSocketContainer> spSockContainer, std::shared_ptr<CommonHeader> spHeader, std::shared_ptr<google::protobuf::Message> spMsg)
{


	
}

void VideoStreamServiceInfoProcessor::AsyncProcess(std::shared_ptr<BaseSocketContainer> spSockContainer, std::shared_ptr<DDRCommProto::CommonHeader> spHeader, std::shared_ptr<google::protobuf::Message> spMsg)
{
	rspVideoStreamServiceInfo* pRaw = reinterpret_cast<rspVideoStreamServiceInfo*>(spMsg.get());

	std::vector<std::string> inputIPs;
	std::vector<std::string> outputIPs;

	for (auto channel : pRaw->channels())
	{
		inputIPs.push_back(channel.srcip());
		outputIPs.push_back(channel.dstip());
	}


	StreamRelayService src(pRaw->channels().size());
	src.Init(inputIPs, outputIPs);
	src.Launch();
	src.Respond();
}