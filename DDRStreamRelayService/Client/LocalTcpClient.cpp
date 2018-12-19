#include "LocalTcpClient.h"
#include "../../../Shared/src/Network/TcpSocketContainer.h"
#include "../../../Shared/src/Network/MessageSerializer.h"
#include "../../../Shared/src/Network/BaseMessageDispatcher.h"
#include "Client/LocalClientDispatcher.h"
#include "Client/LocalClientUdpDispatcher.h"


#include "../../../Shared/proto/BaseCmd.pb.h"
#include "GlobalManager.h"
using namespace DDRCommProto;

LocalTcpClient::LocalTcpClient()
{
}


LocalTcpClient::~LocalTcpClient()
{
}

std::shared_ptr<TcpClientSessionBase> LocalTcpClient::BindSerializerDispatcher()
{
	BIND_IOCONTEXT_SERIALIZER_DISPATCHER(m_IOContext, TcpClientSessionBase, MessageSerializer, LocalClientDispatcher,BaseHeadRuleRouter)
		return spTcpClientSessionBase;
}
void LocalTcpClient::OnConnected(TcpSocketContainer& container)
{

	DebugLog("\nOnConnectSuccess! LocalTcpClient");
	auto spreq = std::make_shared<reqLogin>();
	spreq->set_username("LocalTcpClient_XX");

	if (m_spClient && m_spClient->IsConnected())
	{
		m_spClient->Send(spreq);
	}
	spreq.reset();

}
void LocalTcpClient::OnDisconnect(TcpSocketContainer& container)
{
	TcpClientBase::OnDisconnect(container);



	GlobalManager::Instance()->CreateUdp();
	GlobalManager::Instance()->GetUdpClient()->Start();
	GlobalManager::Instance()->GetUdpClient()->GetSerializer()->BindDispatcher(std::make_shared<LocalClientUdpDispatcher>());
	GlobalManager::Instance()->GetUdpClient()->StartReceive(28888);
}

void LocalTcpClient::StartHeartBeat()
{
	/*m_HeartBeatTimerID = m_Timer.add(std::chrono::seconds(1) , [](timer_id id)
	{

	});*/

	m_HeartBeatTimerID = m_Timer.add(std::chrono::seconds(1), std::bind(&LocalTcpClient::SendHeartBeatOnce, shared_from_base(),std::placeholders::_1), std::chrono::seconds(1));
}
void LocalTcpClient::StopHeartBeat()
{
	m_Timer.remove(m_HeartBeatTimerID);
}
void LocalTcpClient::SendHeartBeatOnce(timer_id id)
{
	heartBeat hb;
	auto sphb = std::make_shared<heartBeat>();
	sphb->set_whatever("hb");

	if (m_spClient && m_spClient->IsConnected())
	{
		m_spClient->Send(sphb);
	}
	sphb.reset();
}