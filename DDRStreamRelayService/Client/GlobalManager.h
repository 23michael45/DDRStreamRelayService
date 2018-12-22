#ifndef GlobalManager_h__
#define GlobalManager_h__

#include "../../../Shared/src/Utility/XmlLoader.h"
#include "../../../Shared/src/Network/UdpSocketBase.h"
#include "../../../Shared/src/Network/TcpClientBase.h"
#include "../../../Shared/src/Utility/Singleton.h"
#include "../../../Shared/src/Utility/LocalizationLoader.h"
#include "StreamRelayTcpClient.h"
#include "StreamRelayTcpServer.h"
using namespace DDRFramework;
class GlobalManager : public DDRFramework::CSingleton<GlobalManager>
{
public:
	GlobalManager();
	~GlobalManager();

public:
	bool StartUdp();
	void StopUdp();
	bool IsUdpWorking();


	void CreateTcpClient();
	void ReleaseTcpClient();
	bool IsTcpClientWorking();


	void StartTcpServer(int port);
	void StopTcpServer();
	bool IsTcpServerWorking();
	
	std::shared_ptr<StreamRelayTcpClient> GetTcpClient();
	std::shared_ptr<StreamRelayTcpServer> GetTcpServer();
	std::shared_ptr<UdpSocketBase> GetUdpClient();

	XmlLoader& GetConfig()
	{
		return m_ConfigLoader;
	}	
	LocalizationLoader& GetLocalizationConfig()
	{
		return m_LocalizationConfig;
	}

private:

	void CreateUdp();
	void OnUdpDisconnect(UdpSocketBase& container);

	std::shared_ptr<StreamRelayTcpClient> m_spTcpClient;
	std::shared_ptr<StreamRelayTcpServer> m_spTcpServer;
	std::shared_ptr<UdpSocketBase> m_spUdpClient;

	XmlLoader m_ConfigLoader;

	LocalizationLoader m_LocalizationConfig;

};

#endif // GlobalManager_h__
