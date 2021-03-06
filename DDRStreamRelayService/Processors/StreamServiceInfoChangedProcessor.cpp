#include "StreamServiceInfoChangedProcessor.h"
#include <memory>
#include "../../../Shared/proto/BaseCmd.pb.h"
#include "../../../Shared/src/Utility/DDRMacro.h"

#include "../../../Shared/src/Utility/Logger.h"
#include "../Client/GlobalManager.h"
#include "../AVStream/StreamRelayService.h"

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
	notifyStreamServiceInfoChanged* pRaw = reinterpret_cast<notifyStreamServiceInfoChanged*>(spMsg.get());


	vector<AVChannelConfig> info;
	for (auto channel : pRaw->channels())
	{
		info.push_back(channel);
	}

	GlobalManager::Instance()->StopTcpServer();
	GlobalManager::Instance()->StartTcpServer(info, pRaw->tcpport());

}