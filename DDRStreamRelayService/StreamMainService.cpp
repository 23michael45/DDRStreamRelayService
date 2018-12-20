#pragma warning(disable :4996)
#pragma once
#include <Windows.h>
#include "StreamMainService.h"
#include "StreamRelayService.h"
//#include "DDRLocalAudioPull.h"
//#include "DDRAudioTranscode.h"
//#include "AudioDeviceInquiry.h"
//#include "DDRAudioPlay.h"
//#include "DDRStreamPlay.h"
#include <fstream>
#include <chrono>
#include "ParamReader.h"

//#include "zWalleAudio.h"
//#include "LocalNetworkNode\LNNFactory.h"

#define USE_TTS 1  // 需要时TTS转语音模块，如果只关注ffmpeg可以将这个宏置0
#if USE_TTS
#include "TTSQueuePlayer.h"
#endif

#define MODULE_NAME "AVManager_Module"
#define CHANNEL_NUM 15
#define CHANNEL_NUM_TEST 15

#if 0 // 2018.10.25 use subThread instead.
unsigned int ClientCB(BYTE *pBuf, unsigned int nBufLen, void *pArg) {
	zWalleAudio *audio = (zWalleAudio*)pArg;
	audio->Out(pBuf, nBufLen);
	printf("接收%d字节\n", nBufLen);
	return 0;
}


bool ServerCB(void *pVecBytes, void *pArg){
	zWalleAudio *audio = (zWalleAudio*)pArg;
	DDRLNN::ResizeVecBytes(pVecBytes, 6400);
	if (true == audio->IsInOpen()){
		BYTE DATA[6400];
		if (6400 == audio->In(DATA)){
			printf("发送6400字节\n");
			memcpy(DDRLNN::GetVecBytesHead(pVecBytes), DATA, 6400);
			return true;
		}
	}
	return false;
}
#endif

int KeyTest()
{
	HANDLE keyboard = GetStdHandle(STD_INPUT_HANDLE);
	DWORD dw, num;
	if (!::GetNumberOfConsoleInputEvents(keyboard, &num) || num == 0) {
		return 0;
	}
	for (int i = 0; i < (int)num; ++i) {
		INPUT_RECORD input;
		::ReadConsoleInputA(keyboard, &input, 1, (LPDWORD)(&dw));
		if (input.EventType == KEY_EVENT && !input.Event.KeyEvent.bKeyDown) {
			return (int)(input.Event.KeyEvent.wVirtualKeyCode);
		}
	}
	return 0;
}

void RobotChatPlayVoice(const char* text)
{
	DDRGadgets::PlayVoice(text, 5);
}

#define SERVER_CONFIG_TXT "DDRAVConfig.txt"
/*
	读取配置文件
*/
std::string GetDDRSeetafaceConfigValue(std::string strTargetType)
{
//	std::cout << "GetDDRSeetafaceConfigValue strTargetType:" << strTargetType.c_str() << std::endl;
	char buffer[256];
	std::ifstream subConfigfile(SERVER_CONFIG_TXT);
	if (!subConfigfile.is_open())
	{
		std::cout << "GetDDRSeetafaceConfigValue() Error opening file" << std::endl;
		subConfigfile.close();
		return "";
	}
	std::string strType = "";
	std::string strValue = "";
	while (!subConfigfile.eof())
	{
		subConfigfile.getline(buffer, 512);
		std::string strData(buffer);

		int index2 = strData.find('=');
		if (-1 == index2)
		{
			continue;
		}

		strType = strData.substr(0, index2);
		if ("Version" == strType)
		{
			continue;
		}

		if (strData.size() > 1)
		{
			strData = strData.substr(index2 + 1);
			index2 = strData.find(';');
			if (index2 != -1)
			{
				strValue = strData.substr(0, index2);
//				std::cout << "Type:" << strType.c_str() << " Value:" << strValue.c_str() << std::endl;
				if (strTargetType == strType)
				{
					break;
				}
			}
			strValue = "";
		}
	}
	subConfigfile.close();
	return strValue;
}


bool RecvLocalServerCmd(const void *pCmdHead,
	int nCmdLen,
	void *pArg,
	bool &bResp,
	std::vector<char> &respVec)
{
	respVec.resize(0);
	StreamMainService *pObj = (StreamMainService*)pArg;
	if (!pObj)
	{
		return true;
	}

	if (nCmdLen == 0)
	{
		bResp = false;
		return true;	
	}
//	std::cout << "RecvLocalServerCmd CmdLen = " << nCmdLen << std::endl;
	char * pHeadTemp = (char*)pCmdHead;
	int nHeadEndIndex = 0;
	for (int i = 0; i < nCmdLen; i++)
	{
		if ('\0' == pHeadTemp[i])
		{
			nHeadEndIndex = i;
			break;
		}
	}

	if (nHeadEndIndex == 0)
	{
		return false;
	}

	int len = *((int*)(pHeadTemp + nHeadEndIndex + 1)); // 获取长度
	if (len > 10000 * 50)
	{
		return false;
	}
	std::vector<char> vecCmdData;
	vecCmdData.resize(len);
	// 去除指令头，和四个字节长度。保存在 vecCmdData 中
	vecCmdData.assign(pHeadTemp + nHeadEndIndex + 4 + 1, pHeadTemp + nHeadEndIndex + 4 + len + 1);

	std::string strHead((const char*)pCmdHead);
	if ("GeneralOrder" == strHead)
	{
		pObj->respond_GeneralOrderCmd(vecCmdData);
		bResp = true;
		return true;
	}
	else if ("VoiceIntercom" == strHead)
	{
		bResp = true;
		return true;
	}
	else if ("RobotChat" == strHead)
	{
		pObj->respond_RobotChatCmd(vecCmdData);
		bResp = false;
		return true;
	}
	else
	{

	}
	bResp = false;
	return true;
}

StreamMainService::StreamMainService(const char * configFileName)
	:mConfigFileName(configFileName),
	m_smClient("SERVICE_CMD", 1)
{
	std::cout << "大道智创音视频管理服务，版本：V1.01 日期：2018-11-22" << std::endl;
	av_log_set_level(AV_LOG_QUIET); // 去除ffpeng的log信息
	m_pRobotChatThread = nullptr;
	m_bPlayDisturbWorkTips = true;
	m_bRobotChatEnable = true;
	bInitSuccess = true;
	if (!Init())
	{
		bInitSuccess = false;
		printf("StreamMainService@Init(): 主服务初始化失败，请检查设备或文件命名是否符合规则！\n");
#if USE_TTS
		DDRGadgets::PlayVoice("音视频管理服务启动失败，请检查", 2);
#endif
	}
}

StreamMainService::~StreamMainService()
{
	delete mpLSM;
}

bool StreamMainService::Init()
{
#if 0
	std::ifstream configFile;
	while (1)
	{
		configFile.open(mConfigFileName, _SH_DENYWR);//_SH_DENYRW /* deny read/write mode */
		if (configFile.is_open())
			break;
	}
	std::string line;
	while (std::getline(configFile, line))
	{
		char inName[256] = { NULL }, outName[256] = { NULL };
		if (!GetInfoFromString(line, inName, outName))
			continue;
		inNames.push_back(std::string(inName));
		outNames.push_back(std::string(outName));
	}
	/*inNames.push_back("audio=@device_cm_{33D9A762-90C8-11D0-BD43-00A0C911CE86}\\wave_{0E78F83D-75C9-4C68-8CC8-CF0CC2E8429A}");
	outNames.push_back("rtmp://134.175.196.42:1935/live/testaudio1");*/
	configFile.close();
	if (inNames.size() < 1)
		return false;
	bAudioDeviceAvail = false;
#else // merge with LSM
	mpLSM = nullptr;
	//ParameterReader pd("LocalServerIDConfig.txt");
	//char localServerID[256] = { NULL };
	//strcpy(localServerID, pd.getData("LOCAL_SERVER_ID").c_str());

	std::string strRobotID = GetDDRSeetafaceConfigValue("RobotID");
	if ("" == strRobotID)
	{
		strRobotID = "DDR028";
	}
	std::string localServerID = strRobotID + "_LocalServer";

	//mpLSM = new MARS::LSMComm(MODULE_NAME, localServerID, false, false, false, false, false, false, false, false, false, true, true);
	mpLSM = new MARS::LSMComm(MODULE_NAME, localServerID.c_str());
		//true, false, false, false, 
		//true, false, false, false, 
		//false, true, true);

	/*mpLSM->EnableCmdRcv();
	mpLSM->EnableCmdSnd();
	mpLSM->EnableCmdRespSnd();
	mpLSM->EnableAVStreamHandling(true);*/
	//mpLSM->EnableFileInq();
	//m_LSMComm->EnableCmdRespRcv();
	//m_LSMComm->EnableAlarmRcv();
	//m_LSMComm->EnableAlarmSnd();
	//m_LSMComm->EnableStatusRcv();
	//m_LSMComm->EnableStatusSnd();
	mpLSM->Start();

	std::cout << "局域网服务器名字为：" << localServerID.c_str() << std::endl;

	std::string strTemp = GetDDRSeetafaceConfigValue("PlayDisturbWorkTips");
	//std::cout << "strTemp = " << strTemp.c_str() << std::endl;
	if ("0" == strTemp)
	{
		m_bPlayDisturbWorkTips = false;
	}
	std::cout << (m_bPlayDisturbWorkTips ? "会播放打扰工作提示音" : "不播放打扰工作提示音") << std::endl;

	strTemp = GetDDRSeetafaceConfigValue("RobotChatEnable");
	if ("0" == strTemp)
	{
		m_bRobotChatEnable = false;
	}
	std::cout << (m_bRobotChatEnable ? "有语音聊天" : "没有语音聊天") << std::endl;
	

#if 1
	GetRRPushInfo();
#else
	MARS::_oneAVChannel2Push avChnPush[CHANNEL_NUM];
	if (!mpLSM->GetPushInfo(avChnPush))
	{
		printf("StreamMainService@Init()：无法获取推流信息，请检查!\n");
		return false;
	}
	int count = 0;
	bool bEmpty = true;
	while (bEmpty)
	{
#if 1
		if (!mpLSM->GetPushInfo(avChnPush))
			return false;
		for (int i = 0; i < CHANNEL_NUM; i++)
		{
			if (avChnPush[i].avType == 1
				|| avChnPush[i].avType == 2
				|| avChnPush[i].avType == 3)
			{
				bEmpty = false;
				break;
			}
		}
#endif
		Sleep(100);
		std::cout << "count = " << count << std::endl;
		count++;
	}
	printf("%d\n", count);
	mAVPushInfo.resize(CHANNEL_NUM);
	std::copy(avChnPush, avChnPush + CHANNEL_NUM, mAVPushInfo.begin());
	inNames.resize(0);
	outNames.resize(0);
	mAudioInNames.resize(0);
	mAudioOutNames.resize(0);
	for (int i = 0; i < CHANNEL_NUM_TEST; i++)
	{
		if (mAVPushInfo[i].avType == 2 || mAVPushInfo[i].avType == 3)
		{
			inNames.push_back(mAVPushInfo[i].localAccStr);
			outNames.push_back(mAVPushInfo[i].remoteAddress);
		}
		else if (mAVPushInfo[i].avType == 1)
		{
			mAudioInNames.push_back(mAVPushInfo[i].localAccStr);
			mAudioOutNames.push_back(mAVPushInfo[i].remoteAddress);
		}
	}
	if (inNames.size() < 1 && mAudioInNames.size() < 1)
	{
		printf("StreamMainService@Init()：无推流信息，请检查!\n");
		return false;
	}

#endif
	bAudioDeviceAvail = false;
#endif
	//std::cout << "StreamMainService::Init() ---\n";
	return true;
}

void StreamMainService::TestMultiThread()
{
#if 1 // for audio
	AudioDeviceInquiry adi;
	adi.Launch();
	adi.Process();
	Sleep(1000);
	char in_filename[256] = { NULL, }, out_filename[256] = { NULL, };
#endif
	while (1)
	{
#if 1 // for audio
		adi.Process();
		//Sleep(1000);
		std::vector<std::string> audioDeivceIDs = adi.GetAudioDeviceID();
		if (audioDeivceIDs.size() < 1)
		{
			printf("无音频推流设备，请检查！\n");
			bAudioDeviceAvail = false;
			//continue;
		}
#if 0
		else
#else // LSM
		else if (mAudioInNames.size() >= 1)
#endif
		{
			bAudioDeviceAvail = true;
			std::vector<std::string> deviceIDs = adi.GetAudioDeviceID();
			strcpy(in_filename, deviceIDs[0].c_str());
			strcpy(out_filename, mAudioOutNames[0].c_str());
			//strcpy(out_filename, "rtmp://134.175.196.42:1935/live/testaudio2");
		}
#endif
#if 0
		if (ReadFileForConfig())
#else // LSM
		if (IsChangedRelayInfo())
#endif
		{
			printf("配置文件有改动，请稍后！\n");
		}
		StreamRelayService srs(inNames.size());

		int ret = srs.Init(inNames, outNames);
		if (ret == STREAM_INIT_ERROR_INPUT_OUTPUT_SIZE
			|| ret == STREAM_INIT_ERROR_OUTPUT_NAME_DUPLICATED)
		{
			printf("初始化失败，输入输出名称有误，请检查！\n");
			Sleep(5000);
			continue;
		}
		else if (ret == STREAM_INIT_ERROR_WEB_QUALITY)
		{
			printf("初始化失败，网络不好，或输出与服务器已有推流重复，请检查！\n");
			continue;
		}
		srs.Start();
#if 1 // for audio
		AudioTranscode at;
		if (bAudioDeviceAvail)
		{
			if (!at.Init(in_filename, out_filename))
			{
				printf("音频设备初始化失败，请检查！\n");
				continue;
			}
			at.Launch();
		}
#endif
		std::chrono::high_resolution_clock::time_point beginTime = std::chrono::high_resolution_clock::now();
		while (1)
		{
			std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
			int durTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - beginTime).count();
			if (std::chrono::duration_cast<std::chrono::milliseconds>(endTime - beginTime).count() >= 5000)
			{
				beginTime = endTime;
#if 0
				if (ReadFileForConfig())
#else // LSM
				if (IsChangedRelayInfo())
#endif
				{
					printf("配置文件有改动，需重新初始化，请稍后！\n");
					srs.QuitSubThreads();
					if (bAudioDeviceAvail)
					{
						at.QuitSubThread();
					}
					break;
				}
			}
#if 1 // for audio
			adi.Process();
			//Sleep(1000);
			if (bAudioDeviceAvail)
			{
				audioDeivceIDs = adi.GetAudioDeviceID();
				if (audioDeivceIDs.size() < 1)
				{
					printf("音频推流设备有问题，需要重新初始化，请检查！\n");
					//at.Quit();
					HANDLE hself = GetCurrentProcess();
					TerminateProcess(hself, 0);
				}
				if (!at.Respond())
				{
					printf("网络连接异常，正在重新初始化！\n");
					at.QuitSubThread();
					srs.QuitSubThreads();
					break;
				}
			}
			else
			{
				if (adi.GetAudioDeviceID().size() >= 1)
				{
					printf("音频流服务加入，正在重新初始化推流服务，请稍等！\n");
					srs.QuitSubThreads();
					break;
				}
			}
#endif
		}
	}
}

void StreamMainService::TestRelayAndPlay()
{
#if 1 // for audio
	AudioDeviceInquiry adi;
	adi.Launch();
	adi.Process();
	Sleep(1000);
	char in_filename[256] = { NULL, }, out_filename[256] = { NULL, };
#endif
	//in_filename = "audio=@device_cm_{33D9A762-90C8-11D0-BD43-00A0C911CE86}\\wave_{0E78F83D-75C9-4C68-8CC8-CF0CC2E8429A}";//argv[1];
	//out_filename = "rtmp://134.175.196.42:1935/live/testaudio1";
	std::string audioPlayUrl;

	while (1)
	{
		bool bRemoteAudioPlay = false;

#if 1 // for audio
		adi.Process();
		//Sleep(1000);
		std::vector<std::string> audioDeivceIDs = adi.GetAudioDeviceID();
		if (audioDeivceIDs.size() < 1)
		{
			printf("无音频推流设备，请检查！\n");
			bAudioDeviceAvail = false;
			//continue;
		}
#if 1
		else
#else // LSM
		else if (mAudioInNames.size() >= 1)
#endif
		{
			bAudioDeviceAvail = true;
			std::vector<std::string> deviceIDs = adi.GetAudioDeviceID();
			strcpy(in_filename, deviceIDs[0].c_str());
			//strcpy(out_filename, mAudioOutNames[0].c_str());
			strcpy(out_filename, "rtmp://134.175.196.42:1935/live/testaudio2");
		}
#endif

#if 1
		if (ReadFileForConfig())
#else // LSM
		if (IsChangedRelayInfo())
#endif
		{
			printf("配置文件有改动，请稍后！\n");
		}
		StreamRelayService srs(inNames.size());

		int ret = srs.Init(inNames, outNames);
		if (ret == STREAM_INIT_ERROR_INPUT_OUTPUT_SIZE
			|| ret == STREAM_INIT_ERROR_OUTPUT_NAME_DUPLICATED)
		{
			printf("初始化失败，输入输出名称有误，请检查！\n");
			Sleep(5000);
			continue;
		}
		else if (ret == STREAM_INIT_ERROR_WEB_QUALITY)
		{
			printf("初始化失败，网络不好，或输出与服务器已有推流重复，请检查！\n");
			continue;
		}
		srs.Launch();
		srs.Respond();
#if 1 // for audio
		AudioTranscode at;
		if (bAudioDeviceAvail)
		{
			if (!at.Init(in_filename, out_filename))
			{
				printf("音频设备初始化失败，请检查！\n");
				continue;
			}
			at.Launch();
			at.Respond();
		}
#endif

#if 0 // for audio play LSM
		StreamPlay sp;
		int avType = 0;
		int state = mpLSM->Talk_GetState();
		if (state == 5)
		{
			//int avType = 0;
			if (mpLSM->GetPullInfo(avType, audioPlayUrl))
			{
				if (avType == 1)
				{
					if (!audioPlayUrl.empty()
						&& strncmp(audioPlayUrl.c_str(), "rtmp", 4) == 0)
					{
						if (!sp.Init(audioPlayUrl.c_str()))
						{
							printf("AudioPlay@Init: 语音播放模块初始化失败，请检查！\n");
							continue;
						}
						sp.Launch();
						bRemoteAudioPlay = true;
					}
				}
			}
		}
#else
		/*StreamPlay sp;
		if (!sp.Init("rtmp://134.175.196.42:1935/live/testaudio2"))
		{
			printf("AudioPlay@Init: 语音播放模块初始化失败，请检查！\n");
			continue;
		}
		sp.Launch();
		bRemoteAudioPlay = true;*/
#endif
		/*if (bAudioDeviceAvail)
		{
			at.Launch();
		}
		srs.Launch();*/
		std::chrono::high_resolution_clock::time_point beginTime = std::chrono::high_resolution_clock::now();
		std::chrono::high_resolution_clock::time_point audioPlayBeginTime = std::chrono::high_resolution_clock::now();
		std::chrono::high_resolution_clock::time_point audioDeviceBeginTime = std::chrono::high_resolution_clock::now();
		while (1)
		{
			std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
			int durTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - beginTime).count();
			int audioPlayDurTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - audioPlayBeginTime).count();
			int audioDeviceDurTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - audioDeviceBeginTime).count();
			if (durTime >= 5000)
			{
				beginTime = endTime;
#if 1
				if (ReadFileForConfig())
#else // LSM
				if (IsChangedRelayInfo())
#endif
				{
					printf("配置文件有改动，需重新初始化，请稍后！\n");
					srs.QuitSubThreads();
					if (bAudioDeviceAvail)
					{
						at.QuitSubThread();
					}
					/*if (bRemoteAudioPlay)
					ap.Stop();*/
					break;
				}
			}
#if 0 // for audio play
			int state = mpLSM->Talk_GetState();
			if (audioPlayDurTime >= 1000)
			{
				audioPlayBeginTime = endTime;
				std::string url;

				printf("state=%d\n", state);
				if (state == 5)
				{
					if (mpLSM->GetPullInfo(avType, url))
					{
						printf("avType=%d, url=%s\n", avType, url.c_str());
						if (url.empty())
						{
							audioPlayUrl = "";
							if (bRemoteAudioPlay)
							{
								bRemoteAudioPlay = false;
								sp.Stop();
							}
						}
						if (strcmp(url.c_str(), audioPlayUrl.c_str()) != 0)
						{
							printf("音频播放地址有变动，请稍后！\n");
							srs.QuitSubThreads();
							if (bRemoteAudioPlay)
								sp.Stop();
							if (bAudioDeviceAvail)
							{
								at.QuitSubThread();
							}
							break;
						}
					}
				}
				else if (bRemoteAudioPlay)
				{
					audioPlayUrl = "";
					bRemoteAudioPlay = false;
					sp.Stop();
				}
			}

#endif
			if (!srs.Respond())
			{
				printf("网络连接异常，正在重新初始化！\n");
				//srs.Reset();
				//Sleep(2000);
				//srs.EndSubThreads();
				srs.QuitSubThreads();
				if (bAudioDeviceAvail)
				{
					at.QuitSubThread();
				}
				/*if (bRemoteAudioPlay)
					sp.Stop();*/
				break;
			}

#if 1 // for audio
			adi.Process();
			//Sleep(1000);
			if (audioDeviceDurTime >= 5000)
			{
				audioDeviceBeginTime = endTime;
				if (bAudioDeviceAvail)
				{
					audioDeivceIDs = adi.GetAudioDeviceID();
					if (audioDeivceIDs.size() < 1)
					{
						printf("音频推流设备有问题，需要重新初始化，请检查！\n");
						//at.Quit();
						HANDLE hself = GetCurrentProcess();
						TerminateProcess(hself, 0);
					}
					if (!at.Respond())
					{
						printf("网络连接异常，正在重新初始化！\n");
						at.QuitSubThread();
						srs.QuitSubThreads();
						/*if (bRemoteAudioPlay)
							sp.Stop();*/
						break;
					}
				}
				else
				{
					if (adi.GetAudioDeviceID().size() >= 1)
					{
						printf("音频流服务加入，正在重新初始化推流服务，请稍等！\n");
						srs.QuitSubThreads();
						/*if (bRemoteAudioPlay)
							sp.Stop();*/
						break;
					}
				}
			}
#endif

#if 1 // for audio play
			if (bRemoteAudioPlay)
			{
				/*int type = sp.GrabFrame();
				if (type == 1)
				{
				if (sp.RetrieveAudioFrame())
				{
				sp.PlayAudio();
				}
				}*/
				/*if (!sp.Respond())
				{
					printf("audioplay: 网络连接异常，正在重新初始化！\n");
					if (bRemoteAudioPlay)
						sp.Stop();
					srs.QuitSubThreads();
					if (bAudioDeviceAvail)
					{
						at.QuitSubThread();
					}
					break;
				}*/
				/*if (!ap.Respond())
				{
				printf("audioplay: 网络连接异常，正在重新初始化！\n");
				if (bRemoteAudioPlay)
				ap.Stop();
				srs.QuitSubThreads();
				break;
				}*/
			}
#endif
			std::chrono::high_resolution_clock::time_point endTimeDur = std::chrono::high_resolution_clock::now();
			durTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTimeDur - endTime).count();
			//printf("durTime=%dms\n", durTime);
		}
	}
}

void StreamMainService::AudioDeviceCheck(AudioDeviceInquiry* adi,
	char* audioInName,
	char* audioOutName,
	std::vector<std::string>& audioDeivceIDs)
{
#if 1 // for audio
	adi->Process();
	//Sleep(1000);
	audioDeivceIDs = adi->GetAudioDeviceID();
	if (audioDeivceIDs.size() < 1)
	{
		printf("无音频推流设备，请检查！\n");
		bAudioDeviceAvail = false;
		//continue;
	}
#if 0
	else
#else // LSM
	else if (mAudioInNames.size() >= 1)
#endif
	{
		bAudioDeviceAvail = true;
		std::vector<std::string> deviceIDs = adi->GetAudioDeviceID();
		strcpy(audioInName, deviceIDs[0].c_str());
		strcpy(audioOutName, mAudioOutNames[0].c_str());
		//strcpy(out_filename, "rtmp://134.175.196.42:1935/live/testaudio2");
	}
#endif
}

bool StreamMainService::AudioPushCheck(AudioTranscode* at,
	char* audioInName,
	char* audioOutName)
{
	if (bAudioDeviceAvail)
	{
		if (!at->Init(audioInName, audioOutName))
		{
			printf("音频设备初始化失败，请检查！\n");
			return false;
		}
		at->Launch();
		at->Respond();
#if 0 // 2018.10.26 use subThread instead
		/*****for local audio push*****/
		if (strncmp(mAudioInNames[0].c_str(), "LNN", 3) == 0)
		{
			if (-1 == s_audio->InOpen()){
				printf("本地音频推流服务初始化失败，请检查！\n");
				return false;
			}
			s_audio->InStart();
			int posStart = 0;
			int pos = mAudioInNames[0].find(":", posStart);
			int secPos = mAudioInNames[0].find(":", pos + 1);
			std::string socketType = mAudioInNames[0].substr(pos + 1, secPos - pos - 1);
			if (strcmp("UDP", socketType.c_str()) == 0) // 2018.10.24 UDP may have problems???
				LNGPush->AddUDPServer(mAudioInNames[0].c_str());
				//LNGPush->AddTCPServer(mAudioInNames[0].c_str(), 0);
			else if (strcmp("TCP", socketType.c_str()) == 0)
				LNGPush->AddTCPServer(mAudioInNames[0].c_str(), 0);
			else
			{
				printf("本地音频推流未指定TCP/UDP，请检查！\n");
				return false;
			}
			LNGPush->SetServerOption(mAudioInNames[0].c_str(), 1024 * 1024 * 8, 1024 * 256);
			LNGPush->_scb_SetArg_NewDataPublish(mAudioInNames[0].c_str(), &s_audio);
			LNGPush->_scb_SetFunc_NewDataPublish(mAudioInNames[0].c_str(), ServerCB);
			LNGPush->StartRunning();
		}
		/*****for local audio push*****/
#endif
	}
	return true;
}

bool StreamMainService::AudioPlayInitCheck(StreamPlay* sp,
	LocalAudioPull* laPull,
	std::string& audioPlayUrl,
	bool& bRemoteAudioPlay,
	bool& bLocalAudioPlay)
{
	int avType = 0;
	int state = mpLSM->Talk_GetState();
	if (state == 5)
	{
		//int avType = 0;
//		if (mpLSM->GetPullInfo(avType, audioPlayUrl))
//		{
//			if (avType == 1)
//			{
//				if (!audioPlayUrl.empty()
//					&& strncmp(audioPlayUrl.c_str(), "rtmp", 4) == 0)
//				{
//					if (!sp->Init(audioPlayUrl.c_str()))
//					{
//						printf("AudioPlay@Init: 语音播放模块初始化失败，请检查！\n");
//						return false;
//					}
//					sp->Launch();
//					bRemoteAudioPlay = true;
//				}
//				else if (!audioPlayUrl.empty()
//					&& strncmp(audioPlayUrl.c_str(), "LNN", 3) == 0)
//				{
//					laPull->SetAudioPullUrl(audioPlayUrl);
//					laPull->SetAudioPull(true);
//#if 0 // may be useless
//					int posStart = 0;
//					int pos = audioPlayUrl.find(":", posStart);
//					int secPos = audioPlayUrl.find(":", pos + 1);
//					std::string socketType = audioPlayUrl.substr(pos + 1, secPos - pos - 1);
//					if (-1 == c_audio.OutOpen()){
//						printf("AudioPlay@Init: 局域网语音播放模块初始化失败，请检查！\n");
//						continue;
//					}
//					if (strcmp("UDP", socketType.c_str()) == 0) // 2018.10.24 UDP may have problems???
//						LNGPull->AddUDPClient(audioPlayUrl.c_str());
//					//LNGPull->AddTCPClient(audioPlayUrl.c_str(), 0);
//					else if (strcmp("TCP", socketType.c_str()) == 0)
//						LNGPull->AddTCPClient(audioPlayUrl.c_str(), 0);
//					else
//					{
//						printf("本地音频推流未指定TCP/UDP，请检查！\n");
//						continue;
//					}
//					LNGPull->SetClientOption(audioPlayUrl.c_str(), 1024 * 256, 1024 * 1024 * 8);
//					LNGPull->_ccb_SetArg_onRecvData(audioPlayUrl.c_str(), &c_audio);
//					LNGPull->_ccb_SetFunc_onRecvData(audioPlayUrl.c_str(), ClientCB);
//					LNGPull->StartRunning();
//#endif
//					bLocalAudioPlay = true;
//				}
//			}
//		}
	}
	return true;
}

bool StreamMainService::AudioPlayInstantCheck(int state, 
	StreamPlay* sp,
	LocalAudioPull* laPull,
	std::string& audioPlayUrl,
	bool& bRemoteAudioPlay,
	bool& bLocalAudioPlay)
{

	int avType = 0;
	//int state = mpLSM->Talk_GetState();
	std::string url;
	//if (state)
		printf("AudioPlayInstantCheck +++ state = %d\n", state);
	if (state == 5 || state == 2)
	{
		if (mpLSM->GetPullInfo(avType, url))
		{
			printf("avType=%d, url=%s\n", avType, url.c_str());
			if (url.empty())
			{
				//audioPlayUrl = "";
				if (bRemoteAudioPlay)
				{
					bRemoteAudioPlay = false;
					sp->Stop();
				}
				if (bLocalAudioPlay)
				{
					bLocalAudioPlay = false;
					/*c_audio.OutStop();
					LNGPull->StopRunning();*/
					laPull->SetAudioPull(false);
				}
			}
			else if (strcmp(url.c_str(), audioPlayUrl.c_str()) != 0)
			{
				audioPlayUrl = url;
				printf("音频播放地址有变动，请稍后！\n");
				if (bRemoteAudioPlay)
				{
					bRemoteAudioPlay = false;
					sp->Stop();
				}
				if (bLocalAudioPlay)
				{
					bLocalAudioPlay = false;
					/*c_audio.OutStop();
					LNGPull->StopRunning();*/
					laPull->SetAudioPull(false);
				}
				if (strncmp(audioPlayUrl.c_str(), "rtmp", 4) == 0)
				{
					sp->SetPlayUrl(audioPlayUrl);
					sp->Launch();
					bRemoteAudioPlay = true;
				}
				else if (strncmp(audioPlayUrl.c_str(), "LNN", 3) == 0)
				{
					laPull->SetAudioPull(false);
					laPull->SetAudioPullUrl(audioPlayUrl);
					while (!laPull->IsQuit());
					laPull->SetAudioPull(true);
					bLocalAudioPlay = true;
				}
			}
		}
	}
	else
	{
		if (bRemoteAudioPlay)
		{
			audioPlayUrl = "";
			bRemoteAudioPlay = false;
			sp->Stop();
		}
		if (bLocalAudioPlay)
		{
			audioPlayUrl = "";
			bLocalAudioPlay = false;
			/*c_audio.OutStop();
			LNGPull->StopRunning();*/
			laPull->SetAudioPull(false);
		}
	}
	
	return true;
}

void StreamMainService::Start()
{
	if (!bInitSuccess)
	{
		std::cout << "StreamMainService::Start() Error. Init StreamMainService failed \n";
		return;
	}

	std::cout << "StreamMainService::Start() 开始运行 ...... \n";
#if 1 // for audio
	AudioDeviceInquiry adi;
	adi.Launch();
	adi.Process();
	Sleep(1000);
	char audioInFilename[256] = { NULL, }, audioOutFilename[256] = { NULL, };
	// for local audio
#endif
	//in_filename = "audio=@device_cm_{33D9A762-90C8-11D0-BD43-00A0C911CE86}\\wave_{0E78F83D-75C9-4C68-8CC8-CF0CC2E8429A}";//argv[1];
	//out_filename = "rtmp://134.175.196.42:1935/live/testaudio1";
	std::string audioPlayUrl;
	//bool bOnlyInitAudioChat = false;
	while (1)
	{
		if (RecvShareMemoryQuit())
		{
			break;
		}
		GetAllLSMCmd();// Young add 20181101
		
		bool bRemoteAudioPlay = false;
		bool bLocalAudioPlay = false;
		std::vector<std::string> audioDeivceIDs;
#if 0 // for audio
		adi.Process();
		//Sleep(1000);
		std::vector<std::string> audioDeivceIDs = adi.GetAudioDeviceID();
		if (audioDeivceIDs.size() < 1)
		{
			printf("无音频推流设备，请检查！\n");
			bAudioDeviceAvail = false;
			//continue;
		}
#if 0
		else
#else // LSM
		else if (mAudioInNames.size() >= 1)
#endif
		{
			bAudioDeviceAvail = true;
			std::vector<std::string> deviceIDs = adi.GetAudioDeviceID();
			strcpy(audioInFilename, deviceIDs[0].c_str());
			strcpy(audioOutFilename, mAudioOutNames[0].c_str());
			//strcpy(out_filename, "rtmp://134.175.196.42:1935/live/testaudio2");
		}
#endif
		// xcy add 20181107 +++
		if (!GetRRPushInfo())
			continue;
		// xcy add 20181107 ---

		AudioDeviceCheck(&adi, audioInFilename, audioOutFilename, audioDeivceIDs);

#if 0
		if (ReadFileForConfig())
#else // LSM
		if (IsChangedRelayInfo())
#endif
		{
			printf("配置文件有改动，请稍后！\n");
		}
		StreamRelayService srs(inNames.size());
		for (int nTemp = 0; nTemp < outNames.size(); nTemp++)
		{
			if ("" == outNames[nTemp])
			{
				outNames[nTemp] = std::to_string(nTemp);
			}
		}
		int ret = srs.Init(inNames, outNames);
		if (ret == STREAM_INIT_ERROR_INPUT_OUTPUT_SIZE
			|| ret == STREAM_INIT_ERROR_OUTPUT_NAME_DUPLICATED)
		{
			printf("初始化失败，输入输出名称有误，请检查！\n");
			continue;
		}
#if 0 // 2018.10.25 there is no such error now. 
		else if (ret == STREAM_INIT_ERROR_WEB_QUALITY)
		{
			printf("初始化失败，网络不好，或输出与服务器已有推流重复，请检查！\n");
			continue;
		}
#endif
		srs.Launch();
		srs.Respond();
#if 1 // for audio
		AudioTranscode at;
		/*zWalleAudio s_audio;
		std::shared_ptr<DDRLNN::LNNNodeGroupInterface> LNGPush;
		LNGPush = DDRLNN::CreateNodeGroup();*/
		//LNGPull = DDRLNN::CreateNodeGroup();
#if 1
		if (bAudioDeviceAvail)
		{
			if (at.Init(audioInFilename, audioOutFilename, mAudioInNames[0].c_str()))
			{
				//printf("音频设备初始化失败，请检查！\n");
				//continue;
				at.Launch();
				at.Respond();
			}
			
#if 0 // 2018.10.26 use subThread instead
			/*****for local audio push*****/
			if (strncmp(mAudioInNames[0].c_str(), "LNN", 3) == 0)
			{
				if (-1 == s_audio.InOpen()){
					printf("本地音频推流服务初始化失败，请检查！\n");
					continue;
				}
				s_audio.InStart();
				int posStart = 0;
				int pos = mAudioInNames[0].find(":", posStart);
				int secPos = mAudioInNames[0].find(":", pos + 1);
				std::string socketType = mAudioInNames[0].substr(pos + 1, secPos - pos - 1);
				if (strcmp("UDP", socketType.c_str()) == 0) // 2018.10.24 UDP may have problems???
					LNGPush->AddUDPServer(mAudioInNames[0].c_str());
					//LNGPush->AddTCPServer(mAudioInNames[0].c_str(), 0);
				else if (strcmp("TCP", socketType.c_str()) == 0)
					LNGPush->AddTCPServer(mAudioInNames[0].c_str(), 0);
				else
				{
					printf("本地音频推流未指定TCP/UDP，请检查！\n");
					continue;
				}
				LNGPush->SetServerOption(mAudioInNames[0].c_str(), 1024 * 1024 * 8, 1024 * 256);
				LNGPush->_scb_SetArg_NewDataPublish(mAudioInNames[0].c_str(), &s_audio);
				LNGPush->_scb_SetFunc_NewDataPublish(mAudioInNames[0].c_str(), ServerCB);
				LNGPush->StartRunning();
			}
			/*****for local audio push*****/
#endif
		}
#else
		if(!AudioPushCheck(&at, &s_audio,LNGPush, audioInFilename, audioOutFilename))
			continue;
#endif
#endif

#if 1 // for audio play
		StreamPlay sp;	
		LocalAudioPull laPull;
		laPull.Launch();
		if(!AudioPlayInitCheck(&sp, &laPull, audioPlayUrl, bRemoteAudioPlay, bLocalAudioPlay))
			continue;
		//std::cout << "Start: audioPlayUrl = [" << audioPlayUrl.c_str() << "]" << std::endl;
#endif
		/*if (bAudioDeviceAvail)
		{
		at.Launch();
		}
		srs.Launch();		*/
		std::chrono::high_resolution_clock::time_point beginTime = std::chrono::high_resolution_clock::now();
		std::chrono::high_resolution_clock::time_point audioPlayBeginTime = std::chrono::high_resolution_clock::now();
		std::chrono::high_resolution_clock::time_point audioDeviceBeginTime = std::chrono::high_resolution_clock::now();
		while (1)
		{
			// Young add +++ 20181105
			if (RecvShareMemoryQuit())
				break;
			GetAllLSMCmd(); 
			bool bIsMute = GetTalkIsMute();
			sp.SetMute(bIsMute);
			laPull.SetMute(bIsMute);
			// Young add --- 20181105

			std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
			int durTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - beginTime).count();
			int audioPlayDurTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - audioPlayBeginTime).count();
			int audioDeviceDurTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - audioDeviceBeginTime).count();
			if (durTime >= 5000)
			{
				beginTime = endTime;
#if 0
				if (ReadFileForConfig())
#else // LSM
				if (IsChangedRelayInfo())
#endif
				{
					printf("配置文件有改动，需重新初始化，请稍后！\n");
					srs.QuitSubThreads();
					if (bAudioDeviceAvail)
					{
						at.QuitSubThread();
					}
					/*if (bRemoteAudioPlay)
					ap.Stop();*/
					//while(!srs.IsAllQuit());
					break;
				}
			}
#if 1 // for audio play
			int state = mpLSM->Talk_GetState();
			//if (state)
			//	std::cout << "Start Init state = " << state << std::endl;

			if (audioPlayDurTime >= 1000)
			{
				audioPlayBeginTime = endTime;
#if 1
				AudioPlayInstantCheck(state, &sp, &laPull, audioPlayUrl, bRemoteAudioPlay, bLocalAudioPlay);
#else
				int avType = 0;
				//int state = mpLSM->Talk_GetState();
				std::string url;

				printf("state=%d\n", state);
				if (state == 5)
				{
					if (mpLSM->GetPullInfo(avType, url))
					{
						printf("avType=%d, url=%s\n", avType, url.c_str());
						if (url.empty())
						{
							//audioPlayUrl = "";
							if (bRemoteAudioPlay)
							{
								bRemoteAudioPlay = false;
								sp.Stop();
							}
							if (bLocalAudioPlay)
							{
								bLocalAudioPlay = false;
								/*c_audio.OutStop();
								LNGPull->StopRunning();*/
								laPull.SetAudioPull(false);
							}
						}
						else if (strcmp(url.c_str(), audioPlayUrl.c_str()) != 0)
						{
							audioPlayUrl = url;
							printf("音频播放地址有变动，请稍后！\n");
							if (bRemoteAudioPlay)
							{
								bRemoteAudioPlay = false;
								sp.Stop();
							}
							if (bLocalAudioPlay)
							{
								bLocalAudioPlay = false;
								/*c_audio.OutStop();
								LNGPull->StopRunning();*/
								laPull.SetAudioPull(false);
							}
							if (strncmp(audioPlayUrl.c_str(), "rtmp", 4) == 0)
							{
								sp.SetPlayUrl(audioPlayUrl);
								sp.Launch();
								bRemoteAudioPlay = true;
							}
							else if (strncmp(audioPlayUrl.c_str(), "LNN", 3) == 0)
							{
								laPull.SetAudioPull(false);
								laPull.SetAudioPullUrl(audioPlayUrl);
								while (!laPull.IsQuit());
								laPull.SetAudioPull(true);
								bLocalAudioPlay = true;
							}
						}
					}
				}
				else if (bRemoteAudioPlay)
				{
					audioPlayUrl = "";
					bRemoteAudioPlay = false;
					sp.Stop();
				}
				else if (bLocalAudioPlay)
				{
					audioPlayUrl = "";
					bLocalAudioPlay = false;
					/*c_audio.OutStop();
					LNGPull->StopRunning();*/
					laPull.SetAudioPull(false);
				}
#endif
			}
#endif
#if 1  // for chatting call
			int key = KeyTest();
			if (state == 0)
			{
				if (key == 32)
				{
					printf("正在打电话，请等待接通......\n");
					mpLSM->Talk_Call();
				}
			}
			else if (state == 2)
			{
				if (key == 'Q')
				{
					printf("挂断中......\n");
					mpLSM->Talk_HangUp();
				}
			}

			if (key == 'Q')
			{
				printf("挂断中1......\n");
				mpLSM->Talk_HangUp();
			}

#endif
			//if ((!m_bIsLocal) && (!srs.Respond()))
			if (!bLocalAudioPlay && (!srs.Respond()))
			{
				printf("srs.Respond is false 网络连接异常，正在重新初始化！\n");
				//srs.Reset();
				//Sleep(2000);
				//srs.EndSubThreads();
				srs.QuitSubThreads();
				if (bAudioDeviceAvail)
				{
					at.QuitSubThread();
				}
				if (bRemoteAudioPlay)
					sp.Stop();
				if (bLocalAudioPlay)
				{
					bLocalAudioPlay = false;
					/*c_audio.OutStop();
					LNGPull->StopRunning();*/
					laPull.SetAudioPull(false);
				}
				//while (!srs.IsAllQuit());
				break;
			}

#if 1 // for audio
			adi.Process();
			//Sleep(1000);
			if (audioDeviceDurTime >= 5000)
			{
				audioDeviceBeginTime = endTime;
				if (bAudioDeviceAvail)
				{
					audioDeivceIDs = adi.GetAudioDeviceID();
					if (audioDeivceIDs.size() < 1)
					{
						printf("音频推流设备有问题，需要重新初始化，请检查！\n");
						//at.Quit();
						HANDLE hself = GetCurrentProcess();
						TerminateProcess(hself, 0);
					}
					if (!at.Respond())
					{
						printf("at.Respond is false 网络连接异常，正在重新初始化！\n");
						at.QuitSubThread();
						/*****for local audio push*****/
						/*s_audio.InStop();
						LNGPush->StopRunning();*/
						/*****for local audio push*****/
						srs.QuitSubThreads();
						if (bRemoteAudioPlay)
							sp.Stop();
						if (bLocalAudioPlay)
						{
							bLocalAudioPlay = false;
							/*c_audio.OutStop();
							LNGPull->StopRunning();*/
							laPull.SetAudioPull(false);
						}
						//while (!srs.IsAllQuit());
						break;
					}
				}
				else
				{
					if (adi.GetAudioDeviceID().size() >= 1)
					{
						printf("音频流服务加入，正在重新初始化推流服务，请稍等！\n");
						mAudioInNames.resize(0);
						mAudioOutNames.resize(0);
						for (int i = 0; i < CHANNEL_NUM_TEST; i++)
						{
							if (mAVPushInfo[i].avType == 1)
							{
								mAudioInNames.push_back(mAVPushInfo[i].localAccStr);
								mAudioOutNames.push_back(mAVPushInfo[i].remoteAddress);
							}
						}
						srs.QuitSubThreads();
						if (bRemoteAudioPlay)
							sp.Stop();
						//while (!srs.IsAllQuit());
						break;
					}
				}
			}
#endif

#if 1 // for audio play
			if (!laPull.Respond())
			{
				printf("audioplay: 局域网音频拉流服务异常，正在重新初始化！\n");
				if (bRemoteAudioPlay)
					sp.Stop();
				srs.QuitSubThreads();
				if (bAudioDeviceAvail)
				{
					at.QuitSubThread();
				}
				if (bLocalAudioPlay)
				{
					bLocalAudioPlay = false;
					/*c_audio.OutStop();
					LNGPull->StopRunning();*/
					laPull.SetAudioPull(false);
				}
				break;
			}
			if (bRemoteAudioPlay && state == 5)
			{
				/*int type = sp.GrabFrame();
				if (type == 1)
				{
				if (sp.RetrieveAudioFrame())
				{
				sp.PlayAudio();
				}
				}*/
				if (!sp.Respond())
				{
					printf("audioplay: 网络连接异常，正在重新初始化！\n");
					if (bRemoteAudioPlay)
						sp.Stop();
					srs.QuitSubThreads();
					if (bAudioDeviceAvail)
					{
						at.QuitSubThread();
					}
					if (bLocalAudioPlay)
					{
						bLocalAudioPlay = false;
						/*c_audio.OutStop();
						LNGPull->StopRunning();*/
						laPull.SetAudioPull(false);
					}
					//while (!srs.IsAllQuit());
					break;
				}
				/*if (!ap.Respond())
				{
				printf("audioplay: 网络连接异常，正在重新初始化！\n");
				if (bRemoteAudioPlay)
				ap.Stop();
				srs.QuitSubThreads();
				break;
				}*/
			}
#endif
			std::chrono::high_resolution_clock::time_point endTimeDur = std::chrono::high_resolution_clock::now();
			durTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTimeDur - endTime).count();
			//printf("durTime=%dms\n", durTime);
		} // small while(1)
	} // big while(1)
	std::cout << "StreamMainService::Start() 即将退出 ...... \n";
}

void StreamMainService::StartTestOnlyInitPlay() // 2018.10.24 old version, all will reinit when any subthread is broken.
{
#if 1 // for audio
	AudioDeviceInquiry adi;
	adi.Launch();
	adi.Process();
	Sleep(1000);
	char in_filename[256] = { NULL, }, out_filename[256] = { NULL, };
#endif
	//in_filename = "audio=@device_cm_{33D9A762-90C8-11D0-BD43-00A0C911CE86}\\wave_{0E78F83D-75C9-4C68-8CC8-CF0CC2E8429A}";//argv[1];
	//out_filename = "rtmp://134.175.196.42:1935/live/testaudio1";
	std::string audioPlayUrl;
	bool bOnlyInitAudioChat = false;
	while (1)
	{
		bool bRemoteAudioPlay = false;

#if 1 // for audio
		adi.Process();
		//Sleep(1000);
		std::vector<std::string> audioDeivceIDs = adi.GetAudioDeviceID();
		if (audioDeivceIDs.size() < 1)
		{
			printf("无音频推流设备，请检查！\n");
			bAudioDeviceAvail = false;
			//continue;
		}
#if 0
		else
#else // LSM
		else if (mAudioInNames.size() >= 1)
#endif
		{
			bAudioDeviceAvail = true;
			std::vector<std::string> deviceIDs = adi.GetAudioDeviceID();
			strcpy(in_filename, deviceIDs[0].c_str());
			strcpy(out_filename, mAudioOutNames[0].c_str());
			//strcpy(out_filename, "rtmp://134.175.196.42:1935/live/testaudio2");
		}
#endif

#if 0
		if (ReadFileForConfig())
#else // LSM
		if (IsChangedRelayInfo())
#endif
		{
			printf("配置文件有改动，请稍后！\n");
		}
		StreamRelayService srs(inNames.size());
		
		int ret = srs.Init(inNames, outNames);
		if (ret == STREAM_INIT_ERROR_INPUT_OUTPUT_SIZE
			|| ret == STREAM_INIT_ERROR_OUTPUT_NAME_DUPLICATED)
		{
			printf("初始化失败，输入输出名称有误，请检查！\n");
			Sleep(5000);
			continue;
		}
		else if (ret == STREAM_INIT_ERROR_WEB_QUALITY)
		{
			printf("初始化失败，网络不好，或输出与服务器已有推流重复，请检查！\n");
			continue;
		}
		srs.Launch();
		srs.Respond();
#if 1 // for audio
		AudioTranscode at;
		if (bAudioDeviceAvail)
		{
			if (!at.Init(in_filename, out_filename))
			{
				printf("音频设备初始化失败，请检查！\n");
				continue;
			}
			at.Launch();
			at.Respond();
		}
#endif

#if 1 // for audio play
		StreamPlay sp;
		int avType = 0;
		int state = mpLSM->Talk_GetState();
		if (state == 5)
		{
			//int avType = 0;
			if (mpLSM->GetPullInfo(avType, audioPlayUrl))
			{
				if (avType == 1)
				{
					if (!audioPlayUrl.empty()
						&& strncmp(audioPlayUrl.c_str(), "rtmp", 4) == 0)
					{
						if (!sp.Init(audioPlayUrl.c_str()))
						{
							printf("AudioPlay@Init: 语音播放模块初始化失败，请检查！\n");
							continue;
						}
						sp.Launch();
						bRemoteAudioPlay = true;
					}
				}
			}
		}
#endif
		/*if (bAudioDeviceAvail)
		{
			at.Launch();
		}
		srs.Launch();		*/
		std::chrono::high_resolution_clock::time_point beginTime = std::chrono::high_resolution_clock::now();
		std::chrono::high_resolution_clock::time_point audioPlayBeginTime = std::chrono::high_resolution_clock::now();
		std::chrono::high_resolution_clock::time_point audioDeviceBeginTime = std::chrono::high_resolution_clock::now();
		while (1)
		{
			std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
			int durTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - beginTime).count();
			int audioPlayDurTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - audioPlayBeginTime).count();
			int audioDeviceDurTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - audioDeviceBeginTime).count();
			if (durTime >= 5000)
			{
				beginTime = endTime;
#if 0
				if (ReadFileForConfig())
#else // LSM
				if (IsChangedRelayInfo())
#endif
				{
					printf("配置文件有改动，需重新初始化，请稍后！\n");
					srs.QuitSubThreads();
					if (bAudioDeviceAvail)
					{
						at.QuitSubThread();
					}
					/*if (bRemoteAudioPlay)
						ap.Stop();*/
					break;
				}
			}
#if 1 // for audio play
			int state = mpLSM->Talk_GetState();
			if (audioPlayDurTime >= 1000)
			{
				audioPlayBeginTime = endTime;
				std::string url;
				
				printf("state=%d\n", state);
				if (state == 5)
				{
					if (mpLSM->GetPullInfo(avType, url))
					{
						printf("avType=%d, url=%s\n", avType, url.c_str());
						if (url.empty())
						{
							audioPlayUrl = "";
							if (bRemoteAudioPlay)
							{
								bRemoteAudioPlay = false;
								sp.Stop();
							}
						}
						if (strcmp(url.c_str(), audioPlayUrl.c_str()) != 0)
						{
							printf("音频播放地址有变动，请稍后！\n");
							srs.QuitSubThreads();
							if (bRemoteAudioPlay)
								sp.Stop();
							if (bAudioDeviceAvail)
							{
								at.QuitSubThread();
							}
							break;
						}
					}
				}
				else if (bRemoteAudioPlay)
				{
					audioPlayUrl = "";
					bRemoteAudioPlay = false;
					sp.Stop();
				}
			}
			
#endif
			if (!srs.Respond())
			{
				printf("网络连接异常，正在重新初始化！\n");
				//srs.Reset();
				//Sleep(2000);
				//srs.EndSubThreads();
				srs.QuitSubThreads();
				if (bAudioDeviceAvail)
				{
					at.QuitSubThread();
				}
				if (bRemoteAudioPlay)
					sp.Stop();
				break;
			}

#if 1 // for audio
			adi.Process();
			//Sleep(1000);
			if (audioDeviceDurTime >= 5000)
			{
				audioDeviceBeginTime = endTime;
				if (bAudioDeviceAvail)
				{
					audioDeivceIDs = adi.GetAudioDeviceID();
					if (audioDeivceIDs.size() < 1)
					{
						printf("音频推流设备有问题，需要重新初始化，请检查！\n");
						//at.Quit();
						HANDLE hself = GetCurrentProcess();
						TerminateProcess(hself, 0);
					}
					if (!at.Respond())
					{
						printf("网络连接异常，正在重新初始化！\n");
						at.QuitSubThread();
						srs.QuitSubThreads();
						if (bRemoteAudioPlay)
							sp.Stop();
						break;
					}
				}
				else
				{
					if (adi.GetAudioDeviceID().size() >= 1)
					{
						printf("音频流服务加入，正在重新初始化推流服务，请稍等！\n");
						srs.QuitSubThreads();
						if (bRemoteAudioPlay)
							sp.Stop();
						break;
					}
				}
			}
#endif

#if 1 // for audio play
			if (bRemoteAudioPlay && state == 5)
			{
				/*int type = sp.GrabFrame();
				if (type == 1)
				{
					if (sp.RetrieveAudioFrame())
					{
						sp.PlayAudio();
					}
				}*/
				if (!sp.Respond())
				{
					printf("audioplay: 网络连接异常，正在重新初始化！\n");
					if (bRemoteAudioPlay)
						sp.Stop();
					srs.QuitSubThreads();
					if (bAudioDeviceAvail)
					{
						at.QuitSubThread();
					}
					break;
				}
				/*if (!ap.Respond())
				{
					printf("audioplay: 网络连接异常，正在重新初始化！\n");
					if (bRemoteAudioPlay)
						ap.Stop();
					srs.QuitSubThreads();
					break;
				}*/
			}
#endif
			std::chrono::high_resolution_clock::time_point endTimeDur = std::chrono::high_resolution_clock::now();
			durTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTimeDur - endTime).count();
			//printf("durTime=%dms\n", durTime);
		}
	}
}

bool StreamMainService::IsChangedRelayInfo()
{
	bool bConfig = false;
	MARS::_oneAVChannel2Push avChnPush[CHANNEL_NUM];
	/*if (!mpLSM->GetPushInfo(avChnPush))
	{
		return false;
	}*/

	std::vector<std::string> tmpIns, tmpOuts, tmpAudioIns, tmpAudioOUts;
	for (int i = 0; i < CHANNEL_NUM_TEST; i++)
	{
		//if (avChnPush[i].remoteAddress.size() > 0)
		//	std::cout << "Recv romote addr" << i+1 << ": " << avChnPush[i].remoteAddress << std::endl;

		if (i >= mAVPushInfo.size()) // xcy add
			continue;

		if (mAVPushInfo[i].avType != avChnPush[i].avType
			|| mAVPushInfo[i].devType != avChnPush[i].devType
			|| strcmp(mAVPushInfo[i].localAccStr.c_str(), avChnPush[i].localAccStr.c_str()) != 0
			|| strcmp(mAVPushInfo[i].remoteAddress.c_str(), avChnPush[i].remoteAddress.c_str()) != 0)
		{
			bConfig = true;
			mAVPushInfo[i].avType = avChnPush[i].avType;
			mAVPushInfo[i].devType = avChnPush[i].devType;
			mAVPushInfo[i].localAccStr = avChnPush[i].localAccStr;
			mAVPushInfo[i].remoteAddress = avChnPush[i].remoteAddress;
		}
		if (mAVPushInfo[i].avType == 2 || mAVPushInfo[i].avType == 3)
		{
			tmpIns.push_back(mAVPushInfo[i].localAccStr);
			tmpOuts.push_back(mAVPushInfo[i].remoteAddress);
		}
		else if (mAVPushInfo[i].avType == 1)
		{
			tmpAudioIns.push_back(mAVPushInfo[i].localAccStr);
			tmpAudioOUts.push_back(mAVPushInfo[i].remoteAddress);
		}
	}
	if (bConfig)
	{
		inNames = tmpIns;
		outNames = tmpOuts;
		mAudioInNames = tmpAudioIns;
		mAudioOutNames = tmpAudioOUts;
	}
	return bConfig;
}

bool StreamMainService::ReadFileForConfig()
{
	bool bConfig = false;
	std::ifstream configFile;
	configFile.open(mConfigFileName, _SH_DENYWR);//_SH_DENYRW /* deny read/write mode */
	if (!configFile.is_open())
		return false;
	std::string line;
	std::vector<std::string> tmpIns, tmpOuts;
	while (std::getline(configFile, line))
	{
		char inName[256] = { NULL }, outName[256] = { NULL };
		if (!GetInfoFromString(line, inName, outName))
			continue;
		tmpIns.push_back(std::string(inName));
		tmpOuts.push_back(std::string(outName));
		int countDiff = 0;
		for (int i = 0; i < inNames.size(); i++)
		{
			if (strcmp(inName, inNames[i].c_str()) != 0
				|| strcmp(outName, outNames[i].c_str()) != 0)
			{
				countDiff++;
			}
			else
				break;
		}
		if (countDiff == inNames.size())
			bConfig = true;
	}
	configFile.close();
	if (bConfig)
	{
		inNames = tmpIns;
		outNames = tmpOuts;
		/*inNames.push_back("audio='@device_cm_{33D9A762-90C8-11D0-BD43-00A0C911CE86}\\wave_{0E78F83D-75C9-4C68-8CC8-CF0CC2E8429A}");
		outNames.push_back("rtmp://134.175.196.42:1935/live/testaudio1'");*/
	}
	return bConfig;
}

bool StreamMainService::GetInfoFromString(std::string line, 
	                                      char* inName,
										  char* outName)
{
	int posStart = 0;
	std::vector<std::string> vals;
	while (1)
	{
		int tmp1 = line.find(" ", posStart);
		int tmp2 = line.find("\t", posStart);
		int pos = -1;// = line.find(" ", posStart);
		pos = max(tmp1, tmp2);
		if (pos == -1)
		{
			std::string val = line.substr(posStart, line.length() - posStart);
			int tmpR = val.find("\r");
			if (tmpR != -1)
			{
				val.erase(tmpR, 1);
			}
			if (val.size() > 0)
				vals.push_back(val);
			break;
		}
		if (pos - posStart <= 0)
		{
			posStart = pos + 1;
			continue;
		}
		std::string val = line.substr(posStart, pos - posStart);
		vals.push_back(val);
		posStart = pos + 1;
	}
	if (vals.size() != 7)
		return false;
	if (atoi(vals[0].c_str()) == -1)
		return false;
	char inIP[32] = { NULL }, outIP[32] = { NULL };
	int inPort, outPort, chn, bsType;
	char id[128] = { NULL };
	strcpy(inIP, vals[0].c_str());
	inPort = atoi(vals[1].c_str());
	chn = atoi(vals[2].c_str());
	bsType = atoi(vals[3].c_str());
	strcpy(outIP, vals[4].c_str());
	outPort = atoi(vals[5].c_str());
	strcpy(id, vals[6].c_str());
	//char inName[256] = { NULL };
	if (chn == -1)
	{
		if (bsType == 1)
			//sprintf(inName, "rtsp://admin:admin123@%s:%d/h264/ch1/main/av_stream", inIP, inPort);
			//sprintf(inName, "rtsp://%s:%d/av0_0&user=admin&password=admin", inIP, inPort);
			//sprintf(inName, "rtsp://%s:%d/user=admin&password=&channel=1&stream=0.sdp?", inIP, inPort);
			sprintf(inName, "rtsp://%s:%d/11", inIP, inPort);
			//sprintf(inName, "rtsp://%s:%d/snl/live/1/1", inIP, inPort);
			//sprintf(inName, "F:/movies/阳光电影www.ygdy8.com.阿尔忒弥斯酒店.BD.720p.中英双字幕.mkv");
		else
			//sprintf(inName, "rtsp://admin:admin123@%s:%d/h264/ch1/sub/av_stream", inIP, inPort);
			//sprintf(inName, "rtsp://%s:%d/av0_1&user=admin&password=admin", inIP, inPort);
			//sprintf(inName, "rtsp://%s:%d/user=admin&password=&channel=1&stream=1.sdp?", inIP, inPort);
			sprintf(inName, "rtsp://%s:%d/12", inIP, inPort);
	}
	else
	{
		bsType = (bsType == 1 ? 0 : 1);
		sprintf(inName, "rtsp://admin:dadao1203@%s:%d/cam/realmonitor?channel=%d&subtype=%d", inIP, inPort, chn, bsType);
	}
	//char outName[256] = { NULL };
	sprintf(outName, "rtmp://%s:1935/live/%s", outIP, id);
	return true;
}


int StreamMainService::GetAllLSMCmd()
{
	if (mpLSM)
	{
		int nConn = mpLSM->GetConnStat();
		if (nConn)
		{
			std::cout << "音视频管理服务未连接局域网服务器，返回码:" << nConn << std::endl;
			Sleep(1000);
			return 0;
		}

		mpLSM->ProcessCommand(RecvLocalServerCmd, (void*)(this));

		std::vector<char> vec;
		//mpLSM->GetNextAlarm(vec);
		if (vec.size() > 1)
		{
		//	std::cout << "获取到Alarm指令，大小为：" << vec.size() << std::endl;
		}
	}
	return 0;
}

/*
这个主要是针对通用指令
eg:
"PlayVoice:Operational=Start,Type=0,data=你好大道智创,Intervals=30000,"
*/
int StreamMainService::respond_GeneralOrderCmd(std::vector<char> &recvData)
{
	char chTemp = ';';
	BYTE byLast = recvData.back();
	if (byLast != chTemp)
	{
		recvData.push_back(chTemp);
	}
	std::string strTemp;
	strTemp.assign(recvData.begin(), recvData.end());
	//std::cout << "respond_GeneralOrderCmd strTemp:[" << strTemp.c_str() << "]" << std::endl;
	int nIndex1 = strTemp.find(":");
	if (nIndex1 < 0)
	{
		return -1;
	}

	std::string strCommand = "";
	strCommand.assign(strTemp.begin(), strTemp.begin() + nIndex1);

	if ("PlayVoice" == strCommand)
	{
		return respond_ClientPlayVoice(recvData);
	}

	return true;
}


/*
这个主要是针对语音聊天功能
eg:
RobotChat\0xxxxType=Start;Online=1;ComNum=2;（进入语音聊天，（如果用的是六麦还需要指定串口号））
RobotChat\0xxxxType=Stop;（停止语音聊天）
*/
int StreamMainService::respond_RobotChatCmd(std::vector<char> &recvData)
{
	if (!m_bRobotChatEnable)
	{
		return 0;
	}

	char chTemp = ';';
	BYTE byLast = recvData.back();
	if (byLast != chTemp)
	{
		recvData.push_back(chTemp);
	}
	std::string strData;
	strData.assign(recvData.begin(), recvData.end());
	//std::cout << "respond_RobotChatCmd strTemp:[" << strData.c_str() << "]" << std::endl;
	std::string strType = "", strOnline = "", strCom = "";
	while (1)
	{
		int nIndex1 = strData.find(chTemp);
		if (nIndex1 < 0)
		{
			break;
		}

		std::string strTemp = "";
		strTemp.assign(strData.begin(), strData.begin() + nIndex1);
		strData.erase(strData.begin(), strData.begin() + nIndex1 + 1);

		int nIndex = strTemp.find("=");

		int nTemp = strTemp.find("Type");
		if (nTemp >= 0)
		{
			strType.assign(strTemp.begin() + nIndex + 1, strTemp.end());
		}

		nTemp = strTemp.find("Online");
		if (nTemp >= 0)
		{
			strOnline.assign(strTemp.begin() + nIndex + 1, strTemp.end());
		}

		nTemp = strTemp.find("ComNum");
		if (nTemp >= 0)
		{
			strCom.assign(strTemp.begin() + nIndex + 1, strTemp.end());
		}
	}

	if ("Start" == strType)
	{
		bool nOnline = false;
		if ("1" == strOnline)
			nOnline = true;
		RobotChatInit(true, nOnline, atoi(strCom.c_str()));
	}
	else
	{
		RobotChatInit(false);
	}

	return true;
}
// 


int StreamMainService::respond_ClientPlayVoice(std::vector<char> &recvData)
{
	char chTempFenge = ';';
	std::string strTemp = "", strTemp2 = "";
	strTemp.assign(recvData.begin(), recvData.end());
	strTemp2 = strTemp;
	//std::cout << "Recv General:" << strTemp.c_str() << std::endl;

	int nIndex1 = strTemp.find(":");
	if (nIndex1 < 0)
	{
		return -1;
	}

	std::string strCommand = "";
	strCommand.assign(strTemp.begin(), strTemp.begin() + nIndex1);
	if (strCommand != "PlayVoice")
	{
		return -1;
	}

	std::string strData = "";
	strData.assign(strTemp.begin() + nIndex1 + 1, strTemp.end());

	std::string strPlayData = "", strLevel = "", strOperational = "";
	std::string strType = "", strIntervals = "";
	while (1)
	{
		int nIndex1 = strData.find(chTempFenge);
		if (nIndex1 < 0)
		{
			break;
		}

		std::string strTemp = "";
		strTemp.assign(strData.begin(), strData.begin() + nIndex1);
		strData.erase(strData.begin(), strData.begin() + nIndex1 + 1);

		int nIndex = strTemp.find("=");

		int nTemp = strTemp.find("Data");
		if (nTemp >= 0)
		{
			strPlayData.assign(strTemp.begin() + nIndex + 1, strTemp.end());
		}

		nTemp = strTemp.find("Level");
		if (nTemp >= 0)
		{
			strLevel.assign(strTemp.begin() + nIndex + 1, strTemp.end());
		}

		nTemp = strTemp.find("Operational");
		if (nTemp >= 0)
		{
			strOperational.assign(strTemp.begin() + nIndex + 1, strTemp.end());
		}

		nTemp = strTemp.find("Type");
		if (nTemp >= 0)
		{
			strType.assign(strTemp.begin() + nIndex + 1, strTemp.end());
		}

		nTemp = strTemp.find("Intervals");
		if (nTemp >= 0)
		{
			strIntervals.assign(strTemp.begin() + nIndex + 1, strTemp.end());
		}
	}

	if ("Start" == strOperational)
	{
		if ("" == strPlayData || "" == strLevel || "" == strIntervals || "" == strType)
		{
			std::cout << "PlayVoice error: illegal instruction data:" << strTemp2.c_str() << std::endl;
			return -1;
		}

		if (atoi(strLevel.c_str()) > 16)
			strLevel = "1";

		std::cout << "播放:[" << strPlayData.c_str() << "] 级别:" << atoi(strLevel.c_str()) << std::endl;

		if (mpLSM)
		{
			// 正在对讲，级别低于8的就不响应了。
			if((5 == mpLSM->Talk_GetState()) && (atoi(strIntervals.c_str()) < 8))
				return 0;
		}

#if USE_TTS
		if ("TTS" == strType)
		{
			if (!m_bPlayDisturbWorkTips && (-1 != strPlayData.find("请不要打扰我工作")))
			{
				std::cout << "不播放【请不要打扰我工作】" << std::endl;
			}
			else
			{
				DDRGadgets::PlayVoice(strPlayData.c_str(), atoi(strLevel.c_str()), atoi(strIntervals.c_str()));
			}
		}
		else
		{
			DDRGadgets::PlayFile(strPlayData.c_str(), atoi(strLevel.c_str()), atoi(strIntervals.c_str()));
		}
#endif
		return 0;
	}
	else
	{
		std::cout << "正在停止播放 ......\n";
#if USE_TTS
		DDRGadgets::StopCyclePlay();
#endif
		return 0;
	}
	return 0;
}

/*
	获取是否要将推拉流静音的状态
	返回值：false表示不静音  true表示要静音
	8为语音对讲的级别。
*/
bool StreamMainService::GetTalkIsMute()
{
	unsigned int nCurrTTSChannel = DDRGadgets::GetCurrentTTSChannel();
	if ((nCurrTTSChannel > 2) && (-1 != nCurrTTSChannel))
		return true;
	return false;
}

/*
	获取远程推流信息
*/
bool StreamMainService::GetRRPushInfo()
{
	m_bIsLocal = true;
	MARS::_oneAVChannel2Push avChnPush[CHANNEL_NUM];
	//if (!mpLSM->GetPushInfo(avChnPush))
	//{
	//	//printf("StreamMainService@Init()：无法获取推流信息，请检查!\n");
	//	return false;
	//}

	bool bEmpty = true;
	for (int i = 0; i < CHANNEL_NUM; i++)
	{
		if (avChnPush[i].avType == 1
			|| avChnPush[i].avType == 2
			|| avChnPush[i].avType == 3)
		{
			bEmpty = false;
			break;
		}
	}
	
	if (!bEmpty)
	{
		mAVPushInfo.resize(CHANNEL_NUM);
		std::copy(avChnPush, avChnPush + CHANNEL_NUM, mAVPushInfo.begin());
		inNames.resize(0);
		outNames.resize(0);
		mAudioInNames.resize(0);
		mAudioOutNames.resize(0);
		for (int i = 0; i < CHANNEL_NUM_TEST; i++)
		{
			if (-1 != mAVPushInfo[i].remoteAddress.find("rtmp"))
			{
				m_bIsLocal = false; // 表示当前广域网是通的
			}

			if (mAVPushInfo[i].avType == 2 || mAVPushInfo[i].avType == 3)
			{
				inNames.push_back(mAVPushInfo[i].localAccStr);
				outNames.push_back(mAVPushInfo[i].remoteAddress);
			}
			else if (mAVPushInfo[i].avType == 1)
			{
				mAudioInNames.push_back(mAVPushInfo[i].localAccStr);
				mAudioOutNames.push_back(mAVPushInfo[i].remoteAddress);
			}
		}
		if (inNames.size() < 1 && mAudioInNames.size() < 1)
		{
			//printf("StreamMainService@Init()1：无推流信息，请检查!\n");
			return false;
		}
		printf("StreamMainService@Init() 有推流信息 +++ \n");
		return true;
	}
//	printf("StreamMainService@Init()2：无推流信息，请检查!\n");

	Sleep(10);
	return false;
}

bool StreamMainService::RecvShareMemoryQuit()
{
	if (m_smClient.IsOkay() && (char)0xFF == *(m_smClient.GetBufHead()))
	{
		RobotChatInit(false);
		std::cout << "音视频管理服务收到退出指令..." << std::endl;
		return true;
	}
	else
	{
		return false;
	}
}

/*
	语音对讲的线程，循环调用 m_pRobotChatThread->Process() 就好了。
	并且等待退出指令。
*/
void StreamMainService::RobotChatThreadRun(void *param)
{
	StreamMainService *pTemp = (StreamMainService *)param;
	if (pTemp)
	{
		if (pTemp->m_pRobotChatThread)
		{
			while (1)
			{
				pTemp->m_pRobotChatThread->Process();
				Sleep(30);
				if (pTemp->m_mutexChat.try_lock())
				{
					bool bQuit = pTemp->m_bQuitChat;
					pTemp->m_mutexChat.unlock();
					if (bQuit)
					{
						break;
					}
				}
			}
		}
	}
}

/*
	管理语音聊天用的
	@bStart： false - 退出对讲， true - 进入对讲
	@bIsOnline：false - 离线， true - 在线
	@nCom：串口号（用到了6麦就会需要这个）
*/
bool StreamMainService::RobotChatInit(bool bStart, bool bIsOnline, int nCom)
{
	if (bStart)
	{
		static bool bInitOK = false;
		if (!bInitOK)
		{
			m_pRobotChatThread = DDRVoice::DDVoiceInteraction::GetInstance();
			if (m_pRobotChatThread)
			{
				bool bret = m_pRobotChatThread->Init(bIsOnline, nCom);
				if (!bret)
				{
					std::cout << "创建语音对话模块失败 " << std::endl;
					return false;
				}
				m_pRobotChatThread->SetPlayCallBackFun(RobotChatPlayVoice);
			}

			std::thread t2(&StreamMainService::RobotChatThreadRun, this);
			t2.detach();
		}
		bInitOK = true;
		m_mutexChat.lock();
		m_bQuitChat = false;
		m_mutexChat.unlock();
		if(m_pRobotChatThread)
		{
			if (-1 != DDRGadgets::GetCurrentTTSChannel())
			{
				std::cout << "正在播放语音，请稍后尝试 ...... \n";
			}
			else if (m_pRobotChatThread->GetSubThreadIsRun())
			{
				std::cout << "当前正在语音对讲，请稍后尝试 ...... \n";
			}
			else
			{
				m_pRobotChatThread->SetSubThreadEnter(true);
				std::cout << "正在进入语言聊天 ...... \n";
			}
		}
	}
	else
	{
		m_mutexChat.lock();
		m_bQuitChat = true;
		m_mutexChat.unlock();
	}
	return true;
}

