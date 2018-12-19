#include "LocalClientUdpProcessor.h"
#include <memory>
#include "../../../Shared/src/Utility/DDRMacro.h"
#include "../../../Shared/src/Utility/CommonFunc.h"
#include "../../../Shared/thirdparty/asio/include/asio.hpp"
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


	for (auto serverinfo : pRaw->lsinfos())
	{
		if (serverinfo.stype() == bcLSAddr_eServiceType_LocalServer)
		{
			DealLocalServer(serverinfo);
		}
		else if (serverinfo.stype() == bcLSAddr_eServiceType_RemoteServer)
		{
		}
		else if (serverinfo.stype() == bcLSAddr_eServiceType_RTSPStreamServer)
		{
		}
		else if (serverinfo.stype() == bcLSAddr_eServiceType_TalkBackServer)
		{
		}
	}
	



}
void LocalClientUdpProcessor::DealLocalServer(bcLSAddr_ServerInfo& serverinfo)
{
	std::string name = serverinfo.name();
	std::string ips;
	int port = serverinfo.port();


	auto localAddr = DDRFramework::GetLocalIPV4();


	std::vector<std::string> remoteAddrs;
	for (auto ip : serverinfo.ips())
	{
		remoteAddrs.push_back(ip);
	}

	auto rmap = DDRFramework::GetSameSegmentIPV4(localAddr, remoteAddrs);
	if (rmap.size() > 0)
	{
		auto conntectip = (rmap.begin())->second;

		TcpClientStart(conntectip, port);
		DebugLog("\nReceive Server Broadcast %s: %s", name.c_str(), conntectip.c_str());
	}
	else
	{



		DebugLog("\nReceive Server Broadcast But No IP in same segment");
	}

}

void LocalClientUdpProcessor::TcpClientStart(std::string serverip, int serverport)
{
	if (GlobalManager::Instance()->IsUdpWorking())
	{
		GlobalManager::Instance()->StopUdp();

		GlobalManager::Instance()->CreateTcpClient();
		GlobalManager::Instance()->GetTcpClient()->Start(4);
		std::ostringstream strport;
		strport << serverport;
		const std::string sPort(strport.str());
		GlobalManager::Instance()->GetTcpClient()->Connect(serverip, sPort);

	}




}