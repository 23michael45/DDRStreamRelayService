#include "LocalClientUdpProcessor.h"
#include <memory>
#include "../../../Shared/proto/BaseCmd.pb.h"
#include "../../../Shared/src/Utility/DDRMacro.h"
#include "LocalTcpClient.h"
#include "GlobalManager.h"

using namespace DDRFramework;
using namespace DDRCommProto;

LocalClientUdpProcessor::LocalClientUdpProcessor(BaseMessageDispatcher& dispatcher) :BaseProcessor(dispatcher)
{
}


LocalClientUdpProcessor::~LocalClientUdpProcessor()
{
	DebugLog("\nLocalClientUdpProcessor Destroy");
}

void LocalClientUdpProcessor::Process(std::shared_ptr<BaseSocketContainer> spSockContainer, std::shared_ptr<CommonHeader> spHeader, std::shared_ptr<google::protobuf::Message> spMsg)
{
	auto bodytype = spHeader->bodytype();

	bcLSAddr* pRaw = reinterpret_cast<bcLSAddr*>(spMsg.get());

	std::string name = pRaw->lsinfo().name();
	std::string ips;
	int port = pRaw->lsinfo().port();

	std::string conntectip;
	for (auto ip : pRaw->lsinfo().ips())
	{
		ips += ":" + ip;
		if (ip.find("192.168.1.") != std::string::npos)
		{
			conntectip = ip;
		}
	}



	TcpClientStart(conntectip,port);
	

	DebugLog("\nReceive Server Broadcast %s: %s" ,name.c_str(),ips.c_str());


}

void LocalClientUdpProcessor::TcpClientStart(std::string serverip, int serverport)
{
	if (GlobalManager::Instance()->IsUdpWorking())
	{
		GlobalManager::Instance()->GetUdpClient()->StopReceive();
		GlobalManager::Instance()->GetUdpClient()->Stop();

		GlobalManager::Instance()->CreateTcp();
		GlobalManager::Instance()->GetTcpClient()->Start(4);
		std::ostringstream strport;
		strport << serverport;
		const std::string sPort(strport.str());
		GlobalManager::Instance()->GetTcpClient()->Connect(serverip, sPort);

	}




}