#ifndef __STREAM_MAIN_SERVICE_H_INCLUDED
#define __STREAM_MAIN_SERVICE_H_INCLUDED
#pragma once

//#include "TalkWithAndroid.h"
#include <string>
#include <vector>
#include "AudioDeviceInquiry.h"
#include "DDRAudioTranscode.h"
#include "DDRStreamPlay.h"
//#include "zWalleAudio.h"
//#include "LocalNetworkNode\LNNFactory.h"
#include "DDRLocalAudioPull.h"
#include "ProcSharedMem.h"
#include "LSM/LSM.h"
#include "RobotChat\DDRRobotChatThread.h"



using namespace DDRCommLib;

class StreamMainService
{
public:
	StreamMainService(const char * configFileName);
	~StreamMainService();
	void Start();
	void StartTestOnlyInitPlay();
	void TestRelayAndPlay();
	void TestMultiThread();
private:
	const char * mConfigFileName;
	std::vector<std::string> inNames, outNames;
	std::vector<std::string> mAudioInNames, mAudioOutNames;
	// LSM
	MARS::LSMComm* mpLSM;
	std::vector<MARS::_oneAVChannel2Push> mAVPushInfo;
	//std::string mAudioPlayUrl;
	bool bAudioDeviceAvail;

	bool Init();
	// false - invalid information; true - valid information
	// SourceIP SourcePort SourceChannel(-1 - IPC, 1/2/3/4 - DVR) SourceBitStreamType(1 - main, 0 - sub)
	// ServerIP ServerPort(now it's 1935) ID
	__declspec(deprecated)
	bool GetInfoFromString(std::string line,
		                   char* inName,
						   char* outName);//,
						   //char* recordName); // for recording 2018/10/9
	__declspec(deprecated)
	bool ReadFileForConfig();
	// LSM
	bool IsChangedRelayInfo();

	void AudioDeviceCheck(AudioDeviceInquiry* adi,
		                  char* audioInName,
						  char* audioOutName,
						  std::vector<std::string>& audioDeivceIDs);
	bool AudioPushCheck(AudioTranscode* at, 
						char* audioInName,
						char* audioOutName);
	bool AudioPlayInitCheck(StreamPlay* sp,
		                LocalAudioPull* laPull,
						std::string& audioPlayUrl,
						bool& bRemoteAudioPlay,
						bool& bLocalAudioPlay);
	bool AudioPlayInstantCheck(int state,
		                       StreamPlay* sp,
		                       LocalAudioPull* laPull,
							   std::string& audioPlayUrl,
							   bool& bRemoteAudioPlay,
							   bool& bLocalAudioPlay);

public:
	bool bInitSuccess;
	// 接收所有来自LSM的指令
	int GetAllLSMCmd();
	// 解析通用指令
	int respond_GeneralOrderCmd(std::vector<char> &recvData);
	// 处理通用指令中的播放指令
	int respond_ClientPlayVoice(std::vector<char> &recvData);

	bool GetTalkIsMute();

	bool GetRRPushInfo();
	bool RecvShareMemoryQuit();

	DDRGadgets::SharedMemClient m_smClient;
	bool m_bIsLocal;

	// robot chat
	bool RobotChatInit(bool nStart, bool bIsOnline = false, int nCom = 0);
	static void  RobotChatThreadRun(void *param);
	DDRVoice::DDVoiceInteraction *m_pRobotChatThread;
	int respond_RobotChatCmd(std::vector<char> &recvData);
	bool m_bQuitChat;
	std::mutex m_mutexChat;

	bool m_bPlayDisturbWorkTips; // 是否播放“请不要打扰我工作”。主要是为了在展会时悠然干扰机器人不播放这句。
	bool m_bRobotChatEnable;
};

#endif // __STREAM_MAIN_SERVICE_H_INCLUDED