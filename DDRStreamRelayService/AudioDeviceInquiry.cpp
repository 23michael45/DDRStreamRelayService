#include "AudioDeviceInquiry.h"
#include <comutil.h>
#pragma comment(lib, "comsuppw.lib")




AudioDeviceInquiry::AudioDeviceInquiry()
	: MainSubsModel(1, 0, 100)
{

}

AudioDeviceInquiry::~AudioDeviceInquiry()
{

}

std::vector<std::string> AudioDeviceInquiry::GetAudioDeviceID()
{
	std::unique_lock<std::mutex> lck(mMutexDeviceID);
	//while (mcvbDeviceID.wait_for(lck, std::chrono::milliseconds(10000)) == std::cv_status::timeout);
	mcvbDeviceID.wait(lck);
	/*gMutexDeviceID.lock();
	std::vector<std::string> tmp = audioDeviceIDs;
	gMutexDeviceID.unlock();*/
	//printf("end notify\n");
	return audioDeviceIDs;
}

HRESULT AudioDeviceInquiry::EnumerateDevices(REFGUID category, IEnumMoniker **ppEnum)
{
	// Create the System Device Enumerator.
	ICreateDevEnum *pDevEnum;
	HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL,
		CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDevEnum));

	if (SUCCEEDED(hr))
	{
		// Create an enumerator for the category.
		hr = pDevEnum->CreateClassEnumerator(category, ppEnum, 0);
		if (hr == S_FALSE)
		{
			hr = VFW_E_NOT_FOUND;  // The category is empty. Treat as an error.
		}
		pDevEnum->Release();
	}
	return hr;
}

void AudioDeviceInquiry::DisplayDeviceInformation(IEnumMoniker *pEnum)
{
	IMoniker *pMoniker = NULL;
	while (pEnum->Next(1, &pMoniker, NULL) == S_OK)
	{
		IPropertyBag *pPropBag;
		HRESULT hr = pMoniker->BindToStorage(0, 0, IID_PPV_ARGS(&pPropBag));
		if (FAILED(hr))
		{
			pMoniker->Release();
			continue;
		}

		VARIANT var;
		VariantInit(&var);

		// Get description or friendly name.
		hr = pPropBag->Read(L"Description", &var, 0);
		if (FAILED(hr))
		{
			hr = pPropBag->Read(L"FriendlyName", &var, 0);
		}
		if (SUCCEEDED(hr))
		{
			//printf("%S\n", var.bstrVal);
			char deviceID[256],deviceIDU[256];
			char *p = _com_util::ConvertBSTRToString(var.bstrVal);
			ANSIToUTF8(p, deviceIDU);
			strcpy(deviceID, "audio=");
			strcat(deviceID, deviceIDU);
			//memcpy(deviceID, p, strlen(p) + 1);
			delete[] p;
			//gMutexDeviceID.lock();
			audioDeviceIDs.push_back(deviceID);
			//gMutexDeviceID.unlock();
			VariantClear(&var);
		}

		//hr = pPropBag->Write(L"FriendlyName", &var);

		//// WaveInID applies only to audio capture devices.
		//hr = pPropBag->Read(L"WaveInID", &var, 0);
		//if (SUCCEEDED(hr))
		//{
		//	printf("WaveIn ID: %d\n", var.lVal);
		//	VariantClear(&var);
		//}

		//hr = pPropBag->Read(L"DevicePath", &var, 0);
		//if (SUCCEEDED(hr))
		//{
		//	// The device path is not intended for display.
		//	printf("Device path: %S\n", var.bstrVal);
		//	VariantClear(&var);
		//}

		pPropBag->Release();
		pMoniker->Release();
	}
}

bool AudioDeviceInquiry::_onRunOnce_Sub(int)
{
	while (1)
	{
		HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
		
		if (SUCCEEDED(hr))
		{
			IEnumMoniker *pEnum;
			/*hr = EnumerateDevices(CLSID_VideoInputDeviceCategory, &pEnum);
			if (SUCCEEDED(hr))
			{
			DisplayDeviceInformation(pEnum);
			pEnum->Release();
			}*/
			/*gMutexDeviceID.lock();
			audioDeviceIDs.resize(0);
			gMutexDeviceID.unlock();*/
			hr = EnumerateDevices(CLSID_AudioInputDeviceCategory, &pEnum);
			audioDeviceIDs.resize(0);
			if (SUCCEEDED(hr))
			{
				DisplayDeviceInformation(pEnum);
				pEnum->Release();
			}	
			//printf("start notify\n");
			std::unique_lock<std::mutex> lck(mMutexDeviceID);
			mcvbDeviceID.notify_one();
			CoUninitialize();			
		}	
		Sleep(1000);
	}
	return true;
}

int AudioDeviceInquiry::ANSIToUTF8(char* pszCode, char* UTF8code)
{
	WCHAR unicode[100] = { 0, };
	char utf8[100] = { 0, };
	//read char length
	int nUnicodeSize = MultiByteToWideChar(CP_ACP, 0, pszCode, 
		                                   strlen(pszCode), unicode, 
										   sizeof(unicode));
	memset(UTF8code, 0, nUnicodeSize + 1);
	//read UTF-8 length
	int nUTF8codeSize = WideCharToMultiByte(CP_UTF8, 0, unicode, 
		                                    nUnicodeSize, UTF8code, 
											sizeof(unicode), NULL, NULL);
	//convert to UTF-8
	MultiByteToWideChar(CP_UTF8, 0, utf8, nUTF8codeSize, unicode, sizeof(unicode));
	UTF8code[nUTF8codeSize] = '\0';
	return nUTF8codeSize;
}