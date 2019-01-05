#ifndef GlobalManager_h__
#define GlobalManager_h__

#include "../../../Shared/src/Utility/XmlLoader.h"
#include "../../../Shared/src/Network/UdpSocketBase.h"
#include "../../../Shared/src/Network/TcpClientBase.h"
#include "../../../Shared/src/Utility/Singleton.h"
#include "../../../Shared/src/Utility/LocalizationLoader.h"
#include "../../../Shared/src/Utility/GlobalManagerBase.h"
#include "StreamRelayTcpClient.h"
#include "StreamRelayTcpServer.h"
using namespace DDRFramework;
class GlobalManager : public DDRFramework::CSingleton<GlobalManager>, public GlobalManagerClientBase
{
public:
	GlobalManager();
	~GlobalManager();

public:
	virtual void Init() override;
	virtual bool StartUdp() override;


	void StartTcpServer(rspStreamServiceInfo& info);
	void StopTcpServer();
	
	std::shared_ptr<StreamRelayTcpServer> GetTcpServer();
	XmlLoader GetConfig()
	{
		return m_Config;
	}

private:

	std::shared_ptr<StreamRelayTcpServer> m_spTcpServer;

	XmlLoader m_Config;
};

#endif // GlobalManager_h__
