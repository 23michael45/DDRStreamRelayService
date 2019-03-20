
#include<iostream>
#include "../../Shared/src/Network/MessageSerializer.h"
#include "../../Shared/src/Network/TcpServerBase.h"
#include "../../Shared/src/Network/TcpClientBase.h"
#include "../../Shared/src/Network/HttpServer.h"
#include "../../Shared/proto/BaseCmd.pb.h"
#include "../../Shared/src/Utility/DDRMacro.h"
#include "../../Shared/src/Utility/CommonFunc.h"
#include "../../Shared/src/Network/HttpClient.h"
#include "Client/LocalClientUdpDispatcher.h"
#include <thread>
#include <chrono>
#include <fstream>
#include "Client/GlobalManager.h"
#include <functional>
#include <regex>
#include "../../../Shared/thirdparty/cppfs/include/cppfs/windows/LocalFileSystem.h"

#include "../../Shared/src/Utility/AudioCodec.h"
#include "Client/StreamRelayTcpServer.h"
#include "Client/HttpFileServer.h"

#include "../../Shared/src/Utility/MiniDump.h"
#include "Client/FileManager.h"

#include "opencv2/opencv.hpp"
#include "opencv2/imgproc.hpp"
#include "Version.h"

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
		AddCommand("playwav", std::bind(&_ConsoleDebug::PlayAudio, this));

		AddCommand("play", std::bind(&_ConsoleDebug::PlayTxt, this));
		AddCommand("regex", std::bind(&_ConsoleDebug::Regex, this));

		AddCommand("string", std::bind(&_ConsoleDebug::StringTest, this));

		AddCommand("audio", std::bind(&_ConsoleDebug::TestAudioPriority, this));


		AddCommand("notify", std::bind(&_ConsoleDebug::NotifyAlarm, this));
		AddCommand("checkfiles", std::bind(&_ConsoleDebug::CheckFiles, this));


		AddCommand("startsend", std::bind(&_ConsoleDebug::StartSend, this));
		AddCommand("stopsend", std::bind(&_ConsoleDebug::StopSend, this));


		AddCommand("ver", std::bind(&_ConsoleDebug::PrintVersion, this));

		AddCommand("crash", std::bind(&_ConsoleDebug::Crash, this));
	}
	void PrintVersion()
	{
		printf_s("\nBuild Time:%s", g_BuildTime.c_str());
		printf_s("\nBuild Version:%s", g_Version.c_str());
		printf_s("\nBuild Mode:%s", g_DMode.c_str());

	}
	void Crash()
	{
		vector<int> *pv = new vector<int>;
		pv = nullptr;
		pv->push_back(1);
	}
	void TestAudioPriority()
	{
		rspStreamServiceInfo info;

		//StreamRelayTcpServer s(info);
		//s.StartPlayTxt(std::string("1111111111"), 1);
		auto sp = std::make_shared<StreamRelayTcpServer>(info);

		sp->StartPlayTxt(std::string("1111111111"), 1);

		std::this_thread::sleep_for(chrono::seconds(1));
		sp->StartPlayTxt(std::string("2222222222"), 2);
		//std::this_thread::sleep_for(chrono::seconds(1));
		//sp->StartPlayTxt(std::string("3333333333"), 3);
		std::this_thread::sleep_for(chrono::seconds(1));

		//sp->StartPlayTxt(std::string("4444444444"), 2);


	}

	void StringTest()
	{
		std::string s = "中文";
		std::wstring ws = L"中文";

		std::string ss = DDRFramework::WStringToString(ws);

		int len = s.length();
		len = ws.length();
		len = ss.length();

		if (std::regex_match(s, std::regex("(.*)文")))
		{

			DebugLog("Match")
		}

		len = 0;
	}
	void Regex()
	{
		std::string file = "ro.ot";
		if (std::regex_match(file, std::regex("(.*)(\\.)(.*)")))
		{
			DebugLog("Match");
		}
		else
		{

			DebugLog("Not Match");
		}
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

			auto spreq = std::make_shared<notifyBaseStatus>();
			spreq->set_batt(1.0f);

			spSession->Send(spreq);
			spreq.reset();
		}
		else
		{
			printf_s("\nClient Not Connection");
		}
	}

	void PlayTxt()
	{
		//auto vec = split(m_CurrentCmd, ':');
		//if (vec.size() == 3)
		//{
		//	DDVoiceInteraction::GetInstance()->RunTTS(vec[1].c_str(), atoi(vec[2].c_str()));
		//	//GlobalManager::Instance()->GetTcpServer()->PlayAudio(vec[1], atoi(vec[2].c_str()));
		//}

		asio::streambuf buf;
		std::string fn("3.wav");
		ifstream is;
		is.open(fn.c_str());

		std::ostream oshold(&buf);
		is.seekg(0, is.end);
		int length = is.tellg();
		is.seekg(0, is.beg);
		char * buffer = new char[length];

		is.read(buffer, length);


		oshold.write(buffer,length);
		oshold.flush();

		is.close();

		auto spbuf = DDVoiceInteraction::Instance()->GetVoiceBuf(std::string("111111111111111"));

		//PlayFile(buf);
		PlayFile(*spbuf.get());
	}





	char* psrc;
	int lenplayed = 0;
	int totallen = 19 * 6400;
	mal_context m_Context;
	mal_device_config m_Config;
	mal_device m_CaptureDevice;
	mal_device m_PlaybackDevice;

	void recv_func(mal_device* pDevice, mal_uint32 frameCount, const void* pSamples)
	{
	}

	mal_uint32 send_func(mal_device* pDevice, mal_uint32 frameCount, void* pSamples)
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

		for (int i = 0; i < 19; i++)
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
			return;
		}
		auto recvFunc = std::bind(&_ConsoleDebug::recv_func, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
		auto sendFunc = std::bind(&_ConsoleDebug::send_func, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
		m_Config = mal_device_config_init(mal_format_s16, 1, 16000, recvFunc, sendFunc);


		printf("Playing...\n");
		if (mal_device_init(&m_Context, mal_device_type_playback, NULL, &m_Config, NULL, &m_PlaybackDevice) != MAL_SUCCESS) {
			mal_context_uninit(&m_Context);
			printf("Failed to initialize playback device.\n");
			return;
		}

		if (mal_device_start(&m_PlaybackDevice) != MAL_SUCCESS) {
			mal_device_uninit(&m_PlaybackDevice);
			mal_context_uninit(&m_Context);
			printf("Failed to start playback device.\n");
			return;
		}
	}




	mal_uint32 on_send_frames_to_device_wav(mal_device* pDevice, mal_uint32 frameCount, void* pSamples)
	{
		mal_decoder* pDecoder = (mal_decoder*)pDevice->pUserData;
		if (pDecoder == NULL) {
			return 0;
		}

		if (pDecoder->memory.currentReadPos == pDecoder->memory.dataSize)
		{
			mal_device_uninit(pDevice);
			mal_decoder_uninit(pDecoder);
		}


		return (mal_uint32)mal_decoder_read(pDecoder, frameCount, pSamples);
	}
	void PlayFile(asio::streambuf& buf)
	{

		mal_device device;
		mal_decoder_config config;
		mal_device_config dconfig;
		mal_decoder decoder;
		mal_result result = mal_decoder_init_memory_wav(buf.data().data(), buf.size(), NULL, &decoder);
		if (result != MAL_SUCCESS) {
			return;
		}
		mal_decoder_seek_to_frame(&decoder, 12880);

		dconfig = mal_device_config_init_playback(decoder.outputFormat, decoder.outputChannels, decoder.outputSampleRate, std::bind(&_ConsoleDebug::on_send_frames_to_device_wav, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

		if (mal_device_init(NULL, mal_device_type_playback, NULL, &dconfig, &decoder, &device) != MAL_SUCCESS) {
			printf("Failed to open playback device.\n");
			mal_decoder_uninit(&decoder);
			return;
		}

		if (mal_device_start(&device) != MAL_SUCCESS) {
			printf("Failed to start playback device.\n");
			mal_device_uninit(&device);
			mal_decoder_uninit(&decoder);
			return;
		}

		getchar();
	}
	void PlayFile(std::string fileName)
	{
		mal_device device;
		mal_device_config config;
		mal_decoder decoder;
		mal_result result = mal_decoder_init_file(fileName.c_str(), NULL, &decoder);
		if (result != MAL_SUCCESS) {
			return;
		}

		config = mal_device_config_init_playback(decoder.outputFormat, decoder.outputChannels, decoder.outputSampleRate, std::bind(&_ConsoleDebug::on_send_frames_to_device_wav, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

		if (mal_device_init(NULL, mal_device_type_playback, NULL, &config, &decoder, &device) != MAL_SUCCESS) {
			printf("Failed to open playback device.\n");
			mal_decoder_uninit(&decoder);
			return;
		}

		if (mal_device_start(&device) != MAL_SUCCESS) {
			printf("Failed to start playback device.\n");
			mal_device_uninit(&device);
			mal_decoder_uninit(&decoder);
			return;
		}

		getchar();
	}


	void NotifyAlarm()
	{
		printf_s("\nSend Alarm");
		auto spSession = GlobalManager::Instance()->GetTcpClient()->GetConnectedSession();
		if (spSession)
		{

			auto spreq = std::make_shared<notifyBaseStatus>();
			spreq->set_batt(0.1f);
			spreq->set_currpath("12");
			spSession->Send(spreq);
			spreq.reset();
		}
		else
		{
			printf_s("\nClient Not Connection");
		}
	}



	DDRFramework::Timer m_Timer;
	DDRFramework::timer_id m_HeartBeatTimerID;
	void StartSend()
	{
		m_HeartBeatTimerID = m_Timer.add(std::chrono::milliseconds(50), std::bind(&_ConsoleDebug::SendOnce, this, std::placeholders::_1), std::chrono::milliseconds(1));
	}
	void SendOnce(timer_id id)
	{
		printf_s("\nSend Alarm");
		auto spSession = GlobalManager::Instance()->GetTcpClient()->GetConnectedSession();
		if (spSession)
		{

			auto spreq = std::make_shared<notifyBaseStatus>();
			//spreq->set_type(eCltType::eLSMStreamRelay);
			spreq->set_batt(0.1f);
			//spreq->set_whatever("0");
			spSession->Send(spreq);
			spreq.reset();
		}
		else
		{
			printf_s("\nClient Not Connection");
		}
	}
	void StopSend()
	{

		m_Timer.remove(m_HeartBeatTimerID);
	}

	void CheckFiles() {

		std::string fmt = "OneRoute*";
		auto matchedFiles = FileManager::Instance()->MatchDir("",fmt);
		std::vector<std::string> v;
		for (auto dir : matchedFiles)
		{
			auto finalFiles = FileManager::Instance()->MatchDir(dir, "Path*");
			for (auto f : finalFiles)
			{
				v.push_back(f);
			}
		}

		auto listfiles = FileManager::Instance()->CheckFiles();

		//std::string fmt = "roo*\\*\\*.txt";
		//std::string fmt = "x*/Debug/*.db";
		//std::string fmt = "fs/*.txt";
		//auto vec = FileManager::Instance()->Match(fmt);

		//for (auto s : vec)
		//{
		//	DebugLog("%s", s.c_str());
		//}

		auto sprsp = std::make_shared<rspFileAddress>();


		auto files = FileManager::Instance()->Match(fmt);

		for (auto file : files)
		{


			std::string httpaddr = HttpFileServer::Instance()->GetHttpFullPath(file);





			sprsp->add_fileaddrlist(httpaddr);



			auto spHttpSession = std::make_shared<HttpSession>();


			cppfs::FilePath fpath(httpaddr);
			auto filename = fpath.fileName();
			spHttpSession->DoGet(httpaddr, filename);



		}



		GlobalManager::Instance()->GetTcpClient()->Send(sprsp);

	}


};



int main(int argc, char **argv)
{
	DisableMouseSelectConsole();
	InitMinDump();
	DDVoiceInteraction::Instance()->Init();
	//HttpFileServer::Instance()->Init();
	GlobalManager::Instance()->Init();
	   

	_ConsoleDebug::Instance()->ConsoleDebugLoop();
	return 0;
}

