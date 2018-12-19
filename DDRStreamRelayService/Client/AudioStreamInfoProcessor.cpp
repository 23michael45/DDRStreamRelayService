#include "AudioStreamInfoProcessor.h"
#include <memory>
#include "../../../Shared/proto/BaseCmd.pb.h"
#include "../../../Shared/src/Utility/DDRMacro.h"

#include "../../../Shared/src/Utility/Logger.h"
#include "GlobalManager.h"
using namespace DDRFramework;
using namespace DDRCommProto;

AudioStreamInfoProcessor::AudioStreamInfoProcessor(BaseMessageDispatcher& dispatcher) :BaseProcessor(dispatcher)
{ 
}


AudioStreamInfoProcessor::~AudioStreamInfoProcessor()
{
}

void AudioStreamInfoProcessor::Process(std::shared_ptr<BaseSocketContainer> spSockContainer, std::shared_ptr<CommonHeader> spHeader, std::shared_ptr<google::protobuf::Message> spMsg)
{

	rspLogin* pRaw = reinterpret_cast<rspLogin*>(spMsg.get());


	rspLogin_eLoginRetCode retcode = pRaw->retcode();
	if (retcode == rspLogin_eLoginRetCode_success)
	{

		GlobalManager::Instance()->GetTcpClient()->RequestVideoStreamInfo();
		GlobalManager::Instance()->GetTcpClient()->StartHeartBeat();
	}
	else
	{
		DebugLog("\nLogin Error");
	}


}