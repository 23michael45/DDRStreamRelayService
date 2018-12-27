#include "GlobalManager.h"
#include "../../../DDRLocalServer/DDR_LocalClient/Client/LocalClientUdpDispatcher.h"

GlobalManager::GlobalManager() :m_ConfigLoader("Config/StreamRelayService/StreamRelayConfig.xml")
{
}
GlobalManager::~GlobalManager()
{


}

bool GlobalManager::StartUdp()
{
	if (!m_spUdpClient)
	{
		m_spUdpClient = std::make_shared<UdpSocketBase>();
		m_spUdpClient->BindOnDisconnect(std::bind(&GlobalManager::OnUdpDisconnect, this, std::placeholders::_1));

	}
	if (m_spUdpClient)
	{
		m_spUdpClient->Start();
		m_spUdpClient->GetSerializer()->BindDispatcher(std::make_shared<LocalClientUdpDispatcher>());
		m_spUdpClient->StartReceive(GlobalManager::Instance()->m_ConfigLoader.GetValue<int>("UdpPort"));
	}
	return true;
}
void GlobalManager::StopUdp()
{
	if (m_spUdpClient && m_spUdpClient->IsWorking())
	{
		m_spUdpClient->StopReceive();
		m_spUdpClient->Stop();
		//m_spUdpClient.reset();donot reset here cause Stop is async ,it will release when OnDisconnect is called

	}
}
void GlobalManager::CreateUdp()
{

}
void GlobalManager::CreateTcpClient()
{
	m_spTcpClient = std::make_shared<StreamRelayTcpClient>();

}
void GlobalManager::ReleaseTcpClient()
{
	if (m_spTcpClient)
	{
		m_spTcpClient.reset();
	}
}
bool GlobalManager::IsUdpWorking()
{
	return m_spUdpClient != nullptr;
}
bool GlobalManager::IsTcpClientWorking()
{
	return m_spTcpClient != nullptr;
}

void GlobalManager::StartTcpServer(int port)
{
	std::string servername = m_ConfigLoader.GetValue("ServerName");
	std::string threadCount = m_ConfigLoader.GetValue("ThreadCount");

	if (!m_spTcpServer)
	{
		m_spTcpServer = std::make_shared<StreamRelayTcpServer>(port);
	}
	m_spTcpServer->Start(std::stoi(threadCount));

}
void GlobalManager::StopTcpServer()
{
	if (m_spTcpServer)
	{
		m_spTcpServer->Stop();
	}
}
bool GlobalManager::IsTcpServerWorking()
{
	return m_spTcpServer != nullptr;
}


std::shared_ptr<StreamRelayTcpClient> GlobalManager::GetTcpClient()
{
	return m_spTcpClient;
}
std::shared_ptr<UdpSocketBase> GlobalManager::GetUdpClient()
{
	return m_spUdpClient;
}
std::shared_ptr<StreamRelayTcpServer> GlobalManager::GetTcpServer()
{
	return m_spTcpServer;
}
void GlobalManager::OnUdpDisconnect(UdpSocketBase& container)
{
	if (m_spUdpClient)
	{
		m_spUdpClient.reset();
	}
}