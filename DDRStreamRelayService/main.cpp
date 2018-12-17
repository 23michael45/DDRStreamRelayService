#include "StreamMainService.h"
#include<iostream>
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

int main(int argc, char **argv)
{
	MoveWorkingDir();
	/*if (argc < 2) {
		printf("usage: %s configFileName.txt\n", argv[0]);
		return 1;
	}*/

	StreamMainService sms("configFileName.txt");
	sms.Start();
	std::cout << "音视频管理服务正常退出 ...\n";
	Sleep(100);
	//sms.StartTestOnlyInitPlay();
	//sms.TestRelayAndPlay();
	//system("pause");
	return 0;
}


