#include "StreamRelayTcpServer.h"
#include "../../../Shared/src/Network/TcpSocketContainer.h"
#include "../../../Shared/src/Network/MessageSerializer.h"
#include "../../../Shared/src/Network/BaseMessageDispatcher.h"

#include "../../../Shared/src/Utility/XmlLoader.h"



StreamRelayTcpServer::StreamRelayTcpServer(int port):TcpServerBase(port)
{
}


StreamRelayTcpServer::~StreamRelayTcpServer()
{
	DebugLog("\nStreamRelayTcpServer Destroy")
}


std::shared_ptr<TcpSessionBase> StreamRelayTcpServer::BindSerializerDispatcher()
{
	BIND_IOCONTEXT_SERIALIZER_DISPATCHER(m_IOContext, TcpSessionBase, MessageSerializer, BaseMessageDispatcher, BaseHeadRuleRouter)
		return spTcpSessionBase;
}


std::shared_ptr<TcpSessionBase> StreamRelayTcpServer::GetTcpSessionBySocket(tcp::socket* pSocket)
{
	if (m_SessionMap.find(pSocket) != m_SessionMap.end())
	{
		return m_SessionMap[pSocket];
	}
	return nullptr;
}

std::map<tcp::socket*, std::shared_ptr<TcpSessionBase>>& StreamRelayTcpServer::GetTcpSocketContainerMap()
{
	return m_SessionMap;
}

std::shared_ptr<TcpSessionBase> StreamRelayTcpServer::StartAccept()
{
	auto spSession = TcpServerBase::StartAccept();
	spSession->BindOnHookReceive(std::bind(&StreamRelayTcpServer::OnHookReceive, shared_from_base(),std::placeholders::_1));
	return spSession;
}

void StreamRelayTcpServer::OnHookReceive(asio::streambuf& buf)
{
	static int i = 0;
	i += buf.size();
	DebugLog("\n%i",i );
}