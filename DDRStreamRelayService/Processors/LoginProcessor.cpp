#include "LoginProcessor.h"
#include <memory>
#include "../../../Shared/proto/BaseCmd.pb.h"
#include "../../../Shared/src/Utility/DDRMacro.h"

#include "../../../Shared/src/Utility/Logger.h"
#include "../Client/GlobalManager.h"
using namespace DDRFramework;
using namespace DDRCommProto;

LoginProcessor::LoginProcessor(BaseMessageDispatcher& dispatcher) :BaseProcessor(dispatcher)
{ 
}


LoginProcessor::~LoginProcessor()
{
}

void LoginProcessor::Process(std::shared_ptr<BaseSocketContainer> spSockContainer, std::shared_ptr<CommonHeader> spHeader, std::shared_ptr<google::protobuf::Message> spMsg)
{

	rspLogin* pRaw = reinterpret_cast<rspLogin*>(spMsg.get());


	rspLogin_eLoginRetCode retcode = pRaw->retcode();
	if (retcode == rspLogin_eLoginRetCode_success)
	{

		auto spClientBase = GlobalManager::Instance()->GetTcpClient();
		auto spClient = std::dynamic_pointer_cast<StreamRelayTcpClient>(spClientBase);
		if (spClient)
		{
			spClient->RequestStreamInfo();
			spClient->StartHeartBeat();

		}
	}
	else
	{
		DebugLog("Login Error");
	}


}