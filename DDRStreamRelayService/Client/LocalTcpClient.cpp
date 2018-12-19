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
	spreq->set_username(GlobalManager::Instance()->GetConfig().GetValue("ServerName"));
	spreq->set_type(eCltType::eLSMStreamRelay);

	if (m_spClient && m_spClient->IsConnected())
	{
		m_spClient->Send(spreq);
	}
	spreq.reset();

}
void LocalTcpClient::OnDisconnect(TcpSocketContainer& container)
{
	TcpClientBase::OnDisconnect(container);

	GlobalManager::Instance()->StartUdp();
}
void LocalTcpClient::Send(std::shared_ptr<google::protobuf::Message> spmsg)
{

	if (m_spClient && m_spClient->IsConnected())
	{
		m_spClient->Send(spmsg);
	}
};
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
	auto sp = std::make_shared<HeartBeat>();
	sp->set_whatever("hb");

	Send(sp);
	sp.reset();
}


void LocalTcpClient::RequestVideoStreamInfo()
{
	auto sp = std::make_shared<reqVideoStreamInfo>();
	sp->set_name(GlobalManager::Instance()->GetConfig().GetValue("ServerName"));

	Send(sp);
	sp.reset();
}
void LocalTcpClient::RequestAudioStreamInfo()
{
	auto sp = std::make_shared<reqAudioStreamInfo>();
	sp->set_name(GlobalManager::Instance()->GetConfig().GetValue("ServerName"));


	Send(sp);
	sp.reset();
}