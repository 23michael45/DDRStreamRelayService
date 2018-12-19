#include "LoginProcessor.h"
#include <memory>
#include "../../../Shared/proto/BaseCmd.pb.h"
#include "../../../Shared/src/Utility/DDRMacro.h"

#include "../../../Shared/src/Utility/Logger.h"
#include "GlobalManager.h"
using namespace DDRFramework;
using namespace DDRCommProto;

LoginProcessor::LoginProcessor(BaseMessageDispatcher& dispatcher) :BaseProcessor(dispatcher)
{
	m_RecvCount = 0;
}


LoginProcessor::~LoginProcessor()
{
}

void LoginProcessor::Process(std::shared_ptr<BaseSocketContainer> spSockContainer, std::shared_ptr<CommonHeader> spHeader, std::shared_ptr<google::protobuf::Message> spMsg)
{

	respLogin* pRaw = reinterpret_cast<respLogin*>(spMsg.get());


	respLogin_eLoginRetCode retcode = pRaw->retcode();

	m_RecvCount++;


	GlobalManager::Instance()->GetTcpClient()->StartHeartBeat();

	if (m_RecvCount % 100 == 0)
	{
		DebugLog("\n--------------------------------------------Login :%i", retcode);

	}
}