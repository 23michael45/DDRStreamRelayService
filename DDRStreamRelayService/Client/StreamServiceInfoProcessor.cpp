#include "StreamServiceInfoProcessor.h"
#include <memory>
#include "../../../Shared/proto/BaseCmd.pb.h"
#include "../../../Shared/src/Utility/DDRMacro.h"

#include "../../../Shared/src/Utility/Logger.h"
#include "GlobalManager.h"
#include "StreamRelayService.h"

using namespace DDRFramework;
using namespace DDRCommProto;

StreamServiceInfoProcessor::StreamServiceInfoProcessor(BaseMessageDispatcher& dispatcher) :BaseProcessor(dispatcher)
{ 
}


StreamServiceInfoProcessor::~StreamServiceInfoProcessor()
{
}

void StreamServiceInfoProcessor::Process(std::shared_ptr<BaseSocketContainer> spSockContainer, std::shared_ptr<CommonHeader> spHeader, std::shared_ptr<google::protobuf::Message> spMsg)
{


	
}

void StreamServiceInfoProcessor::AsyncProcess(std::shared_ptr<BaseSocketContainer> spSockContainer, std::shared_ptr<DDRCommProto::CommonHeader> spHeader, std::shared_ptr<google::protobuf::Message> spMsg)
{
	rspStreamServiceInfo* pRaw = reinterpret_cast<rspStreamServiceInfo*>(spMsg.get());



	std::vector<AVChannelConfig> VideoChannels;
	std::vector<AVChannelConfig> AudioChannels;
	std::vector<AVChannelConfig> AVChannels;
	for (auto channel : pRaw->channels())
	{

		if (channel.streamtype() == ChannelStreamType::Video)
		{
			VideoChannels.push_back(channel);
		}
		if (channel.streamtype() == ChannelStreamType::Audio)
		{
			AudioChannels.push_back(channel);
		}
		if (channel.streamtype() == ChannelStreamType::VideoAudio)
		{
			AVChannels.push_back(channel);
		}
	}
	GlobalManager::Instance()->StartTcpServer(pRaw->tcpport());

	GlobalManager::Instance()->GetTcpServer()->StartVideo(VideoChannels);


	
}