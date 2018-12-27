
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
#include <fstream>
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
		if (GlobalManager::Instance()->GetTcpServer())
		{
			for (auto spSessiont : GlobalManager::Instance()->GetTcpServer()->GetTcpSocketContainerMap())
			{
				std::string ip = spSessiont.second->GetSocket().remote_endpoint().address().to_string();
				printf_s("\n%s", ip.c_str());
			}

		}
		else
		{

			printf_s("\nNo TcpServer Created");
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



char* psrc;
int lenplayed = 0;
int totallen = 19 * 6400;
mal_context m_Context;
mal_device_config m_Config;
mal_device m_CaptureDevice;
mal_device m_PlaybackDevice;

void recv(mal_device* pDevice, mal_uint32 frameCount, const void* pSamples)
{
}

mal_uint32 send(mal_device* pDevice, mal_uint32 frameCount, void* pSamples)
{
	mal_uint32 samplesToRead = frameCount * pDevice->channels;

	int len = totallen - lenplayed;

	if (len < samplesToRead * sizeof(mal_int16))
	{

		memcpy(pSamples, psrc + lenplayed, len);

		lenplayed += len;
		return len / pDevice->channels;
	}
	else
	{

		memcpy(pSamples, psrc + lenplayed, samplesToRead * sizeof(mal_int16));

		lenplayed += samplesToRead * sizeof(mal_int16);
		return samplesToRead / pDevice->channels;
	}
	return 0;
}
void PlayAudio()
{
	std::string root = "D:/DevelopProj/Dadao/DDRFramework/www/wav/";


	psrc = new char[6400 * 19];
	char* p = psrc;

	for (int i = 0 ; i < 19;i++)
	{
		std::string fn = root + std::to_string(i) + ".txt";

		std::ifstream file(fn);
		std::string   line;

		while (std::getline(file, line))
		{
			std::stringstream   linestream(line);
			std::string         data;
			char                val;

			// If you have truly tab delimited data use getline() with third parameter.
			// If your data is just white space separated data
			// then the operator >> will do (it reads a space separated word into a string).
			std::getline(linestream, data, '\t');  // read up-to the first tab (discard tab).

			// Read the integers using the operator >>
			val = atoi(data.c_str());

			*p = val;
			p++;
		}

	}
	std::string ofn = root + "wav.wav";
	ofstream fout(ofn.c_str());
	fout.write(psrc, totallen);
	fout.flush();
	fout.close();



	if (mal_context_init(NULL, 0, NULL, &m_Context) != MAL_SUCCESS) {
		printf("Failed to initialize context.");
		return ;
	}

	m_Config = mal_device_config_init(mal_format_s16, 1, 16000, recv, send);


	printf("Playing...\n");
	if (mal_device_init(&m_Context, mal_device_type_playback, NULL, &m_Config, NULL, &m_PlaybackDevice) != MAL_SUCCESS) {
		mal_context_uninit(&m_Context);
		printf("Failed to initialize playback device.\n");
		return ;
	}

	if (mal_device_start(&m_PlaybackDevice) != MAL_SUCCESS) {
		mal_device_uninit(&m_PlaybackDevice);
		mal_context_uninit(&m_Context);
		printf("Failed to start playback device.\n");
		return ;
	}
}

int main(int argc, char **argv)
{
	GlobalManager::Instance()->StartUdp();

	GlobalManager::Instance()->CreateTcpClient();
	GlobalManager::Instance()->GetTcpClient()->Start(GlobalManager::Instance()->GetConfig().GetValue<int>("ThreadCount"));



	_ConsoleDebug::Instance()->ConsoleDebugLoop();
	return 0;
}

