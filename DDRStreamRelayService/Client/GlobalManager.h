#ifndef GlobalManager_h__
#define GlobalManager_h__

#include "../../../Shared/src/Utility/XmlLoader.h"
#include "../../../Shared/src/Network/UdpSocketBase.h"
#include "../../../Shared/src/Network/TcpClientBase.h"
#include "../../../Shared/src/Utility/Singleton.h"
#include "LocalTcpClient.h"

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


	void CreateTcp();
	void ReleaseTcp();
	bool IsTcpWorking();

	
	std::shared_ptr<LocalTcpClient> GetTcpClient();
	std::shared_ptr<UdpSocketBase> GetUdpClient();

	XmlLoader& GetConfig()
	{
		return m_ConfigLoader;
	}

private:

	void CreateUdp();
	void ReleaseUdp();
	void OnUdpDisconnect(UdpSocketBase& container);

	std::shared_ptr<LocalTcpClient> m_spTcpClient;
	std::shared_ptr<UdpSocketBase> m_spUdpClient;

	XmlLoader m_ConfigLoader;
};

#endif // GlobalManager_h__
