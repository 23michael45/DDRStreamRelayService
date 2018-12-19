#ifndef GlobalManager_h__
#define GlobalManager_h__

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

	void CreateUdp();
	void ReleaseUdp();
	bool IsUdpWorking();

	void CreateTcp();
	void ReleaseTcp();
	bool IsTcpWorking();

	
	std::shared_ptr<LocalTcpClient> GetTcpClient();
	std::shared_ptr<UdpSocketBase> GetUdpClient();

private:

	void OnUdpDisconnect(UdpSocketBase& container);

	std::shared_ptr<LocalTcpClient> m_spTcpClient;
	std::shared_ptr<UdpSocketBase> m_spUdpClient;
};

#endif // GlobalManager_h__
