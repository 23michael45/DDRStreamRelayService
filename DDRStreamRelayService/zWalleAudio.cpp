//#include "stdafx.h"
//#include "zWalleAudio.h"
//
////#pragma comment(lib, "winmm.lib")
//
//typedef struct {
//	BYTE IdentifierString[4];
//	DWORD dwLength;
//
//} RIFF_CHUNK, *PRIFF_CHUNK;
//
//typedef struct {
//	WORD  wFormatTag;         // Format category
//	WORD  wChannels;          // Number of channels
//	DWORD dwSamplesPerSec;    // Sampling rate
//	DWORD dwAvgBytesPerSec;   // For buffer estimation
//	WORD  wBlockAlign;        // Data block size
//	WORD  wBitsPerSample;
//} WAVE_FILE_HEADER, *PWAVE_FILE_HEADER;
//
//BYTE gWaveFileHeader[] = {
//	0x52, 0x49, 0x46, 0x46, 0x28, 0x9C, 0x00, 0x00, 0x57, 0x41, 0x56, 0x45, 0x66, 0x6D, 0x74, 0x20,
//	0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x40, 0x1F, 0x00, 0x00, 0x40, 0x1F, 0x00, 0x00,
//	0x01, 0x00, 0x08, 0x00, 0x64, 0x61, 0x74, 0x61, 0x03, 0x9C, 0x00, 0x00,
//};
//
//// 音频格式
//WAVEFORMATEX gDefWavFmt = {
//	WAVE_FORMAT_PCM,
//	1,
//	16000,
//	32000,
//	2,
//	16,
//	sizeof(WAVEFORMATEX)
//};
//
//void* ReadWaveSample(TCHAR *pFileName, DWORD* pNumSample)
//{
//	BYTE* pWaveSample = NULL;
//	HANDLE hFile;
//	RIFF_CHUNK RiffChunk = { 0 };
//	DWORD dwBytes, dwReturnValue;
//	WAVE_FILE_HEADER WaveFileHeader;
//	DWORD dwIncrementBytes;
//
//	if (hFile = CreateFile(pFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL))
//	{
//		char szIdentifier[5] = { 0 };
//
//		SetFilePointer(hFile, 12, NULL, FILE_CURRENT);
//
//		ReadFile(hFile, &RiffChunk, sizeof(RiffChunk), &dwBytes, NULL);
//		ReadFile(hFile, &WaveFileHeader, sizeof(WaveFileHeader), &dwBytes, NULL);
//
//		dwIncrementBytes = dwBytes;
//
//		do {
//			SetFilePointer(hFile, RiffChunk.dwLength - dwIncrementBytes, NULL, FILE_CURRENT);
//
//			dwReturnValue = GetLastError();
//
//			if (dwReturnValue == 0)
//			{
//				dwBytes = ReadFile(hFile, &RiffChunk, sizeof(RiffChunk), &dwBytes, NULL);
//
//				dwIncrementBytes = 0;
//
//				memcpy(szIdentifier, RiffChunk.IdentifierString, 4);
//			}
//
//		} while (_stricmp(szIdentifier, "data") && dwReturnValue == 0);
//
//		if (dwReturnValue == 0)
//		{
//			pWaveSample = (BYTE *)LocalAlloc(LMEM_ZEROINIT, RiffChunk.dwLength);
//
//			*pNumSample = RiffChunk.dwLength;
//
//			ReadFile(hFile, pWaveSample, RiffChunk.dwLength, &dwBytes, NULL);
//
//			CloseHandle(hFile);
//		}
//	}
//
//	return pWaveSample;
//}
//
//void CALLBACK zWalleAudio::waveOutProc(
//	HWAVEOUT  hwo,
//	UINT      uMsg,
//	DWORD_PTR dwInstance,
//	DWORD_PTR dwParam1,
//	DWORD_PTR dwParam2
//	)
//{
//	zWalleAudio* pWA = (zWalleAudio*)dwInstance;
//	LPWAVEHDR phdr = (LPWAVEHDR)dwParam1;
//	if (uMsg == WOM_DONE)
//	{
//		// 填数据
//		//phdr->dwFlags &= ~WHDR_DONE; //	清DONE标志
//		//waveOutWrite(hwo, phdr, sizeof(WAVEHDR));
//	}
//}
//
//void CALLBACK zWalleAudio::waveInProc(
//	HWAVEIN   hwi,
//	UINT      uMsg,
//	DWORD_PTR dwInstance,
//	DWORD_PTR dwParam1,
//	DWORD_PTR dwParam2
//	)
//{
//	zWalleAudio* pWA = (zWalleAudio*)dwInstance;
//	LPWAVEHDR phdr = (LPWAVEHDR)dwParam1;
//	if (uMsg == WIM_DATA)
//	{
//		// 取数据
//		//phdr->dwFlags &= ~WHDR_DONE; //	清DONE标志
//		//waveInAddBuffer(hwi, phdr, sizeof(WAVEHDR));
//	}
//}
//
//zWalleAudio::zWalleAudio()
//	: ghWaveOut(NULL)
//	, ghWaveIn(NULL)
//{
//}
//
//int zWalleAudio::OutOpen()
//{
//	// 打开语音输出设备
//	if (waveOutOpen(&ghWaveOut, WAVE_MAPPER, &gDefWavFmt, (DWORD_PTR)waveOutProc, (DWORD_PTR)this, CALLBACK_FUNCTION) == MMSYSERR_NOERROR)
//	{	// 准备缓冲区
//		for (int i = 0; i<WOUT_HDR_NUM; i++)
//		{
//			ghWaveOutHdrs[i].dwUser = i;
//			ghWaveOutHdrs[i].dwFlags = 0;
//			ghWaveOutHdrs[i].lpData = (LPSTR)&pcm_out[i*PACKET_SIZE/SAMPLE_SIZE];
//			ghWaveOutHdrs[i].dwBufferLength = PACKET_SIZE;
//			waveOutPrepareHeader(ghWaveOut, &ghWaveOutHdrs[i], sizeof(WAVEHDR));
//			ghWaveOutHdrs[i].dwFlags |= WHDR_DONE;
//		}
//		return 0;
//	}
//	else
//	{
//		printf("错误：没找到合适的声音输出设备（PCM单通道,16K,16位）！\n");
//		return -1;
//	}
//	/*
//	if(waveOutSetVolume(NULL, dwVolume) != MMSYSERR_NOERROR )           音量设置是否成功
//	 {
//	  DBGMSG(ZONE_1, (TEXT("waveOutSetVolume failed, [MainLayere.cpp, SetVolume]")));
//	 }
//	*/
//	//waveOutSetVolume(ghWaveOut, 9);
//}
//
//int zWalleAudio::InOpen()
//{
//	InStop();
//	// 打开录音设备
//	if (waveInOpen(&ghWaveIn, WAVE_MAPPER, &gDefWavFmt, (DWORD_PTR)waveInProc, (DWORD_PTR)this, CALLBACK_FUNCTION) == MMSYSERR_NOERROR)
//	{	// 准备缓冲区
//		for (int i = 0; i<WIN_HDR_NUM; i++)
//		{
//			ghWaveInHdrs[i].dwUser = i;
//			ghWaveInHdrs[i].dwFlags = 0;
//			// 跳过报头，指向数据区
//			ghWaveInHdrs[i].lpData = (LPSTR)&pcm_in[i*PACKET_SIZE/SAMPLE_SIZE];
//			ghWaveInHdrs[i].dwBufferLength = PACKET_SIZE;
//			waveInPrepareHeader(ghWaveIn, &ghWaveInHdrs[i], sizeof(WAVEHDR));
//			ghWaveInHdrs[i].dwFlags |= WHDR_DONE;
//		}
//		return 0;
//	}
//	else
//	{
//		printf("错误：没找到合适的录音设备（PCM单通道,16K,16位）！\n");
//		return -1;
//	}
//}
//
//zWalleAudio::~zWalleAudio()
//{
//	OutStop();
//	InStop();
//}
//
//int zWalleAudio::InStart()
//{
//	if (ghWaveIn == NULL)
//		return -1;
//
//	for (int i = 0; i<WIN_HDR_NUM; i++)
//	{
//		ghWaveInHdrs[i].dwFlags &= ~WHDR_DONE; //	清DONE标志
//		waveInAddBuffer(ghWaveIn, &ghWaveInHdrs[i], sizeof(WAVEHDR));
//	}
//	return waveInStart(ghWaveIn);
//}
//
//int zWalleAudio::In(BYTE* data, int len)
//{
//	if (ghWaveIn == NULL)
//		return -1;
//
//	for (int i = 0; i<WIN_HDR_NUM; i++)
//	{
//		if (ghWaveInHdrs[i].dwFlags & WHDR_DONE)
//		{
//			memcpy(data, ghWaveInHdrs[i].lpData, len);
//			ghWaveInHdrs[i].dwFlags &= ~WHDR_DONE; //	清DONE标志
//			waveInAddBuffer(ghWaveIn, &ghWaveInHdrs[i], sizeof(WAVEHDR));
//			return len;
//		}
//	}
//	return 0;
//}
//
//void zWalleAudio::InStop()
//{
//	if (ghWaveIn)
//	{
//		waveInStop(ghWaveIn);
//		for (int i = 0; i<WIN_HDR_NUM; i++)
//		{
//			waveInUnprepareHeader(ghWaveIn, &ghWaveInHdrs[i], sizeof(WAVEHDR));
//		}
//		waveInClose(ghWaveIn);
//		ghWaveIn = NULL;
//	}
//}
//
//int zWalleAudio::Out(BYTE* data, int len)
//{
//	if (ghWaveOut == NULL)
//		return -1;
//
//	int i;
//	for (int i = 0; i < WOUT_HDR_NUM; i++)
//	{
//		if (ghWaveOutHdrs[i].dwFlags & WHDR_DONE)
//		{
//			ghWaveOutHdrs[i].dwFlags &= ~WHDR_DONE; // 输出前清DONE标志
//			if (len > PACKET_SIZE)
//			{
//				len = PACKET_SIZE;
//			}
//			ghWaveOutHdrs[i].dwBufferLength = len;
//			memcpy(ghWaveOutHdrs[i].lpData, data, len);
//			waveOutWrite(ghWaveOut, &ghWaveOutHdrs[i], sizeof(WAVEHDR));
//			return len;
//		}
//	}
//	return 0;
//}
//
//void zWalleAudio::OutStop()
//{
//	if (ghWaveOut)
//	{
//		waveOutReset(ghWaveOut);
//		for (int i = 0; i<WOUT_HDR_NUM; i++)
//		{
//			waveOutUnprepareHeader(ghWaveOut, &ghWaveOutHdrs[i], sizeof(WAVEHDR));
//		}
//		waveOutClose(ghWaveOut);
//		ghWaveOut = NULL;
//	}
//}
//
//bool zWalleAudio::IsOutEmpty()
//{
//	if (ghWaveOut == NULL)
//		return true;
//
//	int i;
//	for (int i = 0; i < WOUT_HDR_NUM; i++)
//	{
//		if (!(ghWaveOutHdrs[i].dwFlags & WHDR_DONE))
//		{
//			return false;
//		}
//	}
//	return true;
//}
