#include "../Client/GlobalManager.h"
#include "../../../DDRLocalServer/DDR_LocalClient/Client/LocalClientUdpDispatcher.h"
#include "../Processors/LocalClientUdpProcessor.h"

GlobalManager::GlobalManager() :m_Config("Config/StreamRelayService/StreamRelayConfig.xml")
{

}
GlobalManager::~GlobalManager()
{
}
void GlobalManager::Init()
{
	if (!m_spUdpClient)
	{
		m_spUdpClient = std::make_shared<UdpSocketBase>();
		m_spUdpClient->BindOnDisconnect(std::bind(&GlobalManagerClientBase::OnUdpDisconnect, this, std::placeholders::_1));
		StartUdp();
	}
	if (!m_spTcpClient)
	{
		m_spTcpClient = std::make_shared<StreamRelayTcpClient>();
		m_spTcpClient->Start(m_Config.GetValue<int>("ThreadCount"));
	}
}
bool GlobalManager::StartUdp()
{
	if (m_spUdpClient)
	{
		m_spUdpClient->Start();

		auto spDispatcher = std::make_shared<BaseUdpMessageDispatcher>();
		spDispatcher->AddProcessor<bcLSAddr, LocalClientUdpProcessor>();
		m_spUdpClient->GetSerializer()->BindDispatcher(spDispatcher);
		m_spUdpClient->StartReceive(m_GlobalConfig.GetValue<int>("UdpPort"));
	}
	return true;
}

void GlobalManager::StartTcpServer(vector<AVChannelConfig> info,int port)
{
	std::string servername = m_Config.GetValue("ServerName");
	std::string threadCount = m_Config.GetValue("ThreadCount");

	if (!m_spTcpServer)
	{
		m_spTcpServer = std::make_shared<StreamRelayTcpServer>(info,port);
	}
	m_spTcpServer->Start(std::stoi(threadCount));

}
void GlobalManager::StopTcpServer()
{
	if (m_spTcpServer)
	{
		m_spTcpServer->Stop();

		int s = m_spTcpServer.use_count();

		m_spTcpServer.reset();
	}
}



std::shared_ptr<StreamRelayTcpServer> GlobalManager::GetTcpServer()
{
	return m_spTcpServer;
}