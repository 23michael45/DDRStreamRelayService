#include <Windows.h>
#include "DDRLocalAudioPull.h"
#include "zWalleAudio.h"
#include "LocalNetworkNode\LNNFactory.h"
#include <mutex>
std::mutex gMutexAudioPull;
static std::mutex g_mutexGetMute;
static bool g_bLocalIsMute = false;

unsigned int ClientCB(BYTE *pBuf, unsigned int nBufLen, void *pArg)
{
	// xcy add +++ 20181105
	bool bmute = false;
	if (g_mutexGetMute.try_lock())
	{
		bmute = g_bLocalIsMute;
		g_mutexGetMute.unlock();
		if (!bmute)
		{
			zWalleAudio *audio = (zWalleAudio*)pArg;
			audio->Out(pBuf, nBufLen);
#if 0
			for (int i = 0; i < 50; i++)
				printf("%d_", (int)pBuf[i]);
			printf("\n接收%d字节\n", nBufLen);
#endif
			return 0;
		}
	}
	// xcy add --- 20181105
	//zWalleAudio *audio = (zWalleAudio*)pArg;
	//audio->Out(pBuf, nBufLen);
	printf("222接收%d字节\n", nBufLen);
	return 0;
}

LocalAudioPull::LocalAudioPull()
	: MainSubsModel(1, 0, 100)
{
	mbAudioPull = false;
	bQuit = false;
}

LocalAudioPull::~LocalAudioPull()
{
	EndSubThreads();
}

bool LocalAudioPull::Respond()
{
	return MainSubsModel::Process() == 0 ? false : true;
}

bool LocalAudioPull::_onRunOnce_Sub(int)
{
	if (!mbAudioPull)
	{
		gMutexAudioPull.lock();
		bQuit = true;
		gMutexAudioPull.unlock();
		Sleep(100);
		return true;
	}
	gMutexAudioPull.lock();
	bQuit = false;
	gMutexAudioPull.unlock();
	std::shared_ptr<DDRLNN::LNNNodeGroupInterface> LNGPull;
	LNGPull = DDRLNN::CreateNodeGroup();
	zWalleAudio c_audio;
	int posStart = 0;
	int pos = mAudioPullUrl.find(":", posStart);
	int secPos = mAudioPullUrl.find(":", pos + 1);
	std::string socketType = mAudioPullUrl.substr(pos + 1, secPos - pos - 1);
	if (-1 == c_audio.OutOpen()){
		printf("AudioPlay@Init: 局域网语音播放模块初始化失败，请检查！\n");
		gMutexAudioPull.lock();
		bQuit = true;
		gMutexAudioPull.unlock();
		return false;
	}
	if (strcmp("UDP", socketType.c_str()) == 0) // 2018.10.24 UDP may have problems???
		LNGPull->AddUDPClient(mAudioPullUrl.c_str());
		//LNGPull->AddTCPClient(mAudioPullUrl.c_str(), 0);
	else if (strcmp("TCP", socketType.c_str()) == 0)
		LNGPull->AddTCPClient(mAudioPullUrl.c_str(), 0);
	else
	{
		printf("本地音频推流未指定TCP/UDP，请检查！\n");
		gMutexAudioPull.lock();
		bQuit = true;
		gMutexAudioPull.unlock();
		return false;
	}
	LNGPull->SetClientOption(mAudioPullUrl.c_str(), 1024 * 256, 1024 * 1024 * 8);
	LNGPull->_ccb_SetArg_onRecvData(mAudioPullUrl.c_str(), &c_audio);
	LNGPull->_ccb_SetFunc_onRecvData(mAudioPullUrl.c_str(), ClientCB);
	LNGPull->StartRunning();
	while (mbAudioPull)
	{
		//printf("LocalAudioPull LNN Name：[%s]\n", mAudioPullUrl.c_str());
		Sleep(100);
	}
	gMutexAudioPull.lock();
	bQuit = true;
	gMutexAudioPull.unlock();
	c_audio.OutStop();
	LNGPull->StopRunning();
	return true;
}

//bool LocalAudioPull::isTaskAllowed_Main()
//{
//	mbAudioPull = mbAudioPullExt;
//	mAudioPullUrl = mAudioPullUrlExt;
//	return true;
//}

void LocalAudioPull::SetAudioPull(bool bStart)
{
	mbAudioPull= bStart;
}

void LocalAudioPull::SetAudioPullUrl(std::string url)
{
	mAudioPullUrl = url;
}

/*
	xcy add +++ 20181105
	设置是否静音。主要是在收到其他播放指令时会用到
*/
bool LocalAudioPull::SetMute(bool bIsMute)
{
	if (g_mutexGetMute.try_lock())
	{
		g_bLocalIsMute = bIsMute;
		g_mutexGetMute.unlock();
		return true;
	}
	return false;
}

bool LocalAudioPull::IsQuit()
{
	gMutexAudioPull.lock();
	bool tmp = bQuit;
	gMutexAudioPull.unlock();
	return tmp;
}