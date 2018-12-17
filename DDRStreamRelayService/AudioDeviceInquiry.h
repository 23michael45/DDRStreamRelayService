#pragma once
#include <windows.h>
#include <dshow.h>
#include <vector>
#include <string>

#include <mutex>
#include <condition_variable>

#pragma comment(lib, "strmiids")

#include "MyMTModules.h"
using namespace DDRMTLib;

class AudioDeviceInquiry : public MainSubsModel
{
public:
	AudioDeviceInquiry();
	~AudioDeviceInquiry();
	std::vector<std::string> GetAudioDeviceID();

protected:
	bool _onRunOnce_Sub(int) override;

private:
	std::vector<std::string> audioDeviceIDs;

	std::condition_variable mcvbDeviceID;
	std::mutex mMutexDeviceID;

	HRESULT EnumerateDevices(REFGUID category, IEnumMoniker **ppEnum);
	void DisplayDeviceInformation(IEnumMoniker *pEnum);

	int ANSIToUTF8(char* pszCode, char* UTF8code);
};