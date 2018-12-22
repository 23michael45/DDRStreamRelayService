#include "StreamRelayTcpClient.h"
#include "../../../Shared/src/Network/TcpSocketContainer.h"
#include "../../../Shared/src/Network/MessageSerializer.h"
#include "../../../Shared/src/Network/BaseMessageDispatcher.h"
#include "Client/LocalClientDispatcher.h"
#include "Client/LocalClientUdpDispatcher.h"


#include "../../../Shared/proto/BaseCmd.pb.h"
#include "GlobalManager.h"
using namespace DDRCommProto;

StreamRelayTcpClient::StreamRelayTcpClient()
{
}


StreamRelayTcpClient::~StreamRelayTcpClient()
{
	DebugLog("\nDestroy StreamRelayTcpClient")
}

std::shared_ptr<TcpClientSessionBase> StreamRelayTcpClient::BindSerializerDispatcher()
{
	BIND_IOCONTEXT_SERIALIZER_DISPATCHER(m_IOContext, TcpClientSessionBase, MessageSerializer, LocalClientDispatcher,BaseHeadRuleRouter)
		return spTcpClientSessionBase;
}
void StreamRelayTcpClient::OnConnected(TcpSocketContainer& container)
{

	DebugLog("\nOnConnectSuccess! StreamRelayTcpClient");
	auto spreq = std::make_shared<reqLogin>();
	spreq->set_username(GlobalManager::Instance()->GetConfig().GetValue("ServerName"));
	spreq->set_type(eCltType::eLSMStreamRelay);

	Send(spreq);
	spreq.reset();

}
void StreamRelayTcpClient::OnDisconnect(TcpSocketContainer& container)
{
	TcpClientBase::OnDisconnect(container);
	GlobalManager::Instance()->StopTcpServer();

	GlobalManager::Instance()->StartUdp();
}
void StreamRelayTcpClient::StartHeartBeat()
{
	/*m_HeartBeatTimerID = m_Timer.add(std::chrono::seconds(1) , [](timer_id id)
	{

	});*/

	m_HeartBeatTimerID = m_Timer.add(std::chrono::seconds(1), std::bind(&StreamRelayTcpClient::SendHeartBeatOnce, shared_from_base(),std::placeholders::_1), std::chrono::seconds(1));
}
void StreamRelayTcpClient::StopHeartBeat()
{
	m_Timer.remove(m_HeartBeatTimerID);
}
void StreamRelayTcpClient::SendHeartBeatOnce(timer_id id)
{
	auto sp = std::make_shared<HeartBeat>();
	sp->set_whatever("hb");

	Send(sp);
	sp.reset();
}

void StreamRelayTcpClient::RequestStreamInfo()
{
	auto sp = std::make_shared<reqStreamServiceInfo>();
	sp->set_name(GlobalManager::Instance()->GetConfig().GetValue("ServerName"));
	
	Send(sp);
	sp.reset();
}