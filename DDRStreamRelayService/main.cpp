
#include<iostream>
#include "../../Shared/src/Network/MessageSerializer.h"
#include "../../Shared/src/Network/TcpServerBase.h"
#include "../../Shared/src/Network/TcpClientBase.h"
#include "../../Shared/proto/BaseCmd.pb.h"
#include "../../Shared/src/Utility/DDRMacro.h"
#include "Client/LocalClientUdpDispatcher.h"
#include <thread>
#include <chrono>
#include "Client/LocalTcpClient.h"
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


void UdpClientStart()
{

	GlobalManager::Instance()->StartUdp();


}
char gQuit = 0;
int main(int argc, char **argv)
{
	//MoveWorkingDir();
	/*if (argc < 2) {
		printf("usage: %s configFileName.txt\n", argv[0]);
		return 1;
	}*/

	UdpClientStart();



	//StreamMainService sms("configFileName.txt");
	//sms.Start();
	//std::cout << "音视频管理服务正常退出 ...\n";
	//Sleep(100);
	////sms.StartTestOnlyInitPlay();
	////sms.TestRelayAndPlay();
	////system("pause");

	while (!gQuit)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	return 0;
}


