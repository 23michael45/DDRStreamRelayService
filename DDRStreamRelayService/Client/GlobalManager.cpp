#include "GlobalManager.h"
#include "../../../DDRLocalServer/DDR_LocalClient/Client/LocalClientUdpDispatcher.h"

GlobalManager::GlobalManager() :m_ConfigLoader("Config/StreamRelayConfig.xml")
{
}
GlobalManager::~GlobalManager()
{

}

bool GlobalManager::StartUdp()
{
	if (m_spUdpClient)
	{
		return false;
	}
	CreateUdp(); 
	if (m_spUdpClient)
	{
		m_spUdpClient->Start();
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

	}
}
void GlobalManager::CreateUdp()
{
	m_spUdpClient = std::make_shared<UdpSocketBase>();
	m_spUdpClient->BindOnDisconnect(std::bind(&GlobalManager::OnUdpDisconnect, this, std::placeholders::_1));

}
void GlobalManager::ReleaseUdp()
{
	if (m_spUdpClient)
	{
		m_spUdpClient.reset();
	}
}

void GlobalManager::CreateTcp()
{
	m_spTcpClient = std::make_shared<LocalTcpClient>();

}
void GlobalManager::ReleaseTcp()
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
bool GlobalManager::IsTcpWorking()
{
	return m_spTcpClient != nullptr;
}
std::shared_ptr<LocalTcpClient> GlobalManager::GetTcpClient()
{
	return m_spTcpClient;
}
std::shared_ptr<UdpSocketBase> GlobalManager::GetUdpClient()
{
	return m_spUdpClient;
}
void GlobalManager::OnUdpDisconnect(UdpSocketBase& container)
{
	ReleaseUdp();
}