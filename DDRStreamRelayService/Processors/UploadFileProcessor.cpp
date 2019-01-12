

#include <memory>
#include "../../../Shared/proto/BaseCmd.pb.h"
#include "UploadFileProcessor.h"
#include "../../../Shared/src/Utility/DDRMacro.h"
#include "../../../Shared/src/Utility/Logger.h"
#include "../Client/GlobalManager.h"

using namespace DDRFramework;
using namespace DDRCommProto;

UploadFileProcessor::UploadFileProcessor(BaseMessageDispatcher& dispatcher) :BaseProcessor(dispatcher)
{
}


UploadFileProcessor::~UploadFileProcessor()
{
}

void UploadFileProcessor::Process(std::shared_ptr<BaseSocketContainer> spSockContainer, std::shared_ptr<CommonHeader> spHeader, std::shared_ptr<google::protobuf::Message> spMsg)
{

	notifyUploadFile* pRaw = reinterpret_cast<notifyUploadFile*>(spMsg.get());
	if (pRaw)
	{

	}


}

void UploadFileProcessor::AsyncProcess(std::shared_ptr<BaseSocketContainer> spSockContainer, std::shared_ptr<DDRCommProto::CommonHeader> spHeader, std::shared_ptr<google::protobuf::Message> spMsg)
{


}
