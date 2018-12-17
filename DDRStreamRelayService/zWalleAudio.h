#pragma once
#include <Mmsystem.h>

#define	WOUT_HDR_NUM	6
#define	WIN_HDR_NUM		8
#define	PACKET_SIZE		6400 // 200msµÄÊý¾Ý
#define	SAMPLE_SIZE		2
#define	IN_BUF_SIZE		(PACKET_SIZE*WIN_HDR_NUM)
#define	OUT_BUF_SIZE	(PACKET_SIZE*WOUT_HDR_NUM)

class zWalleAudio
{
	HWAVEOUT ghWaveOut;
	WAVEHDR ghWaveOutHdrs[WOUT_HDR_NUM];
	HWAVEIN  ghWaveIn;
	WAVEHDR ghWaveInHdrs[WIN_HDR_NUM];
	short pcm_in[IN_BUF_SIZE/SAMPLE_SIZE];
	short pcm_out[OUT_BUF_SIZE/SAMPLE_SIZE];

	static void CALLBACK waveOutProc(
		HWAVEOUT  hwo,
		UINT      uMsg,
		DWORD_PTR dwInstance,
		DWORD_PTR dwParam1,
		DWORD_PTR dwParam2
		);

	static void CALLBACK waveInProc(
		HWAVEIN   hwi,
		UINT      uMsg,
		DWORD_PTR dwInstance,
		DWORD_PTR dwParam1,
		DWORD_PTR dwParam2
		);


public:
	zWalleAudio();
	virtual ~zWalleAudio();

	int InOpen();
	int OutOpen();
	int InStart();
	int In(BYTE* data, int len = PACKET_SIZE);
	void InStop();
	int Out(BYTE* data, int len);
	void OutStop();
	bool IsOutOpen()
	{
		return ghWaveOut != NULL;
	}
	bool IsInOpen()
	{
		return ghWaveIn != NULL;
	}
	bool IsOutEmpty();
};
