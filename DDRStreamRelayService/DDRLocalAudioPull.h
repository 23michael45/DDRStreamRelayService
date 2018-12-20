//#ifndef __LOCAL_AUDIO_PULL_H_INCLUDED__
//#define __LOCAL_AUDIO_PULL_H_INCLUDED__
//#pragma once
//
//#include <string>
//#include "MyMTModules.h"
//using namespace DDRMTLib;
//
//class LocalAudioPull : public MainSubsModel
//{
//public:
//	LocalAudioPull();
//	~LocalAudioPull();
//	bool Respond();
//
//	// true for start, false for stop.
//	void SetAudioPull(bool bStart); 
//	void SetAudioPullUrl(std::string url);
//	bool IsQuit();
//	bool SetMute(bool bIsMute);
//protected:
//	bool _onRunOnce_Sub(int) override;
//	//bool isTaskAllowed_Main() override;
//private:
//	//bool mbAudioPullExt;
//	bool mbAudioPull;
//	std::string mAudioPullUrl;
//	bool bQuit;
//	
//};
//#endif //__LOCAL_AUDIO_PULL_H_INCLUDED__