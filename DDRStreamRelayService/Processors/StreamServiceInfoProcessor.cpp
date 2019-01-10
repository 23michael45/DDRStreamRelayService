#include "StreamServiceInfoProcessor.h"
#include <memory>
#include "../../../Shared/proto/BaseCmd.pb.h"
#include "../../../Shared/src/Utility/DDRMacro.h"

#include "../../../Shared/src/Utility/Logger.h"
#include "../Client/GlobalManager.h"
#include "../AVStream/StreamRelayService.h"

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
	
	GlobalManager::Instance()->StartTcpServer(*pRaw);


	
}