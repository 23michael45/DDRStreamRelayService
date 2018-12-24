
#include<iostream>
#include "../../Shared/src/Network/MessageSerializer.h"
#include "../../Shared/src/Network/TcpServerBase.h"
#include "../../Shared/src/Network/TcpClientBase.h"
#include "../../Shared/src/Network/HttpServer.h"
#include "../../Shared/proto/BaseCmd.pb.h"
#include "../../Shared/src/Utility/DDRMacro.h"
#include "Client/LocalClientUdpDispatcher.h"
#include <thread>
#include <chrono>
#include "Client/GlobalManager.h"
using namespace DDRFramework;
using namespace DDRCommProto;
void MoveWorkingDir()
{
	char modName[500];
	::GetModuleFileNameA(NULL, modName, 500);
	int dirLastPos = -1;
	for (int i = strlen(modName) - 1; i >= 0; --i)
	{
		if ('\\' == modName[i])
		{
			dirLastPos = i;
			break;
		}
	}

	if (dirLastPos != -1)
	{
		modName[dirLastPos + 1] = '\0';
		::SetCurrentDirectoryA(modName);
	}
}

class _ConsoleDebug : public DDRFramework::ConsoleDebug, public CSingleton<_ConsoleDebug>
{
public:
	_ConsoleDebug()
	{
		AddCommand("ls sc", std::bind(&_ConsoleDebug::ListServerConnections, this));
		AddCommand("ls cc", std::bind(&_ConsoleDebug::ListClientConnection, this));

		AddCommand("alarm", std::bind(&_ConsoleDebug::Alarm, this));
	}
	void ListServerConnections()
	{
		printf_s("\nServer Connections");
		for (auto spSessiont : GlobalManager::Instance()->GetTcpServer()->GetTcpSocketContainerMap())
		{
			std::string ip = spSessiont.second->GetSocket().remote_endpoint().address().to_string();
			printf_s("\n%s", ip.c_str());
		}
	}
	void ListClientConnection()
	{
		printf_s("\nClient Connection");
		auto spSession = GlobalManager::Instance()->GetTcpClient()->GetConnectedSession();
		if(spSession)
		{
			std::string ip = spSession->GetSocket().remote_endpoint().address().to_string();
			printf_s("\n%s", ip.c_str());
		}
		else
		{
			printf_s("\nClient Not Connection");
		}
	}
	void Alarm()
	{
		printf_s("\nSend Alarm");
		auto spSession = GlobalManager::Instance()->GetTcpClient()->GetConnectedSession();
		if (spSession)
		{

			auto spreq = std::make_shared<reqStreamRelayAlarm>();
			spreq->set_error("SRS_Err_Remote_Server_Error");
			spreq->add_to(eCltType::eAndroidClient);
			spreq->add_to(eCltType::ePCClient);

			spSession->Send(spreq);
			spreq.reset();
		}
		else
		{
			printf_s("\nClient Not Connection");
		}
	}
};
int main(int argc, char **argv)
{
	GlobalManager::Instance()->StartUdp();

	GlobalManager::Instance()->CreateTcpClient();
	GlobalManager::Instance()->GetTcpClient()->Start(GlobalManager::Instance()->GetConfig().GetValue<int>("ThreadCount"));



	_ConsoleDebug::Instance()->ConsoleDebugLoop();
	return 0;
}


