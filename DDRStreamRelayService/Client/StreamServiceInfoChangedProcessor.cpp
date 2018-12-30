#include "StreamServiceInfoChangedProcessor.h"
#include <memory>
#include "../../../Shared/proto/BaseCmd.pb.h"
#include "../../../Shared/src/Utility/DDRMacro.h"

#include "../../../Shared/src/Utility/Logger.h"
#include "GlobalManager.h"
#include "StreamRelayService.h"

using namespace DDRFramework;
using namespace DDRCommProto;

StreamServiceInfoChangedProcessor::StreamServiceInfoChangedProcessor(BaseMessageDispatcher& dispatcher) :BaseProcessor(dispatcher)
{ 
}


StreamServiceInfoChangedProcessor::~StreamServiceInfoChangedProcessor()
{
}

void StreamServiceInfoChangedProcessor::Process(std::shared_ptr<BaseSocketContainer> spSockContainer, std::shared_ptr<CommonHeader> spHeader, std::shared_ptr<google::protobuf::Message> spMsg)
{


	
}

void StreamServiceInfoChangedProcessor::AsyncProcess(std::shared_ptr<BaseSocketContainer> spSockContainer, std::shared_ptr<DDRCommProto::CommonHeader> spHeader, std::shared_ptr<google::protobuf::Message> spMsg)
{
	rspStreamServiceInfo* pRaw = reinterpret_cast<rspStreamServiceInfo*>(spMsg.get());



	std::vector<AVChannelConfig> channels;
	for (auto channel : pRaw->channels())
	{
		channels.push_back(channel);
	}

	GlobalManager::Instance()->GetTcpServer()->StartRemoteVideo(channels);


	
}