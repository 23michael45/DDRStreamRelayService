#ifndef __DD_VOICE_INTERACTION_H_INCLUDED__
#define __DD_VOICE_INTERACTION_H_INCLUDED__
//#pragma once

#include "speech_recognizer.h"
#include "MyMTModules.h"

#include <functional>
#include <vector>
#include "../../Shared/thirdparty/asio/include/asio.hpp"

using namespace DDRMTLib;
#define	BUFFER_SIZE	4096
#define OUTLEN 4096
#define SAMPLE_RATE_16K     (16000)
#define MAX_GRAMMARID_LEN   (32)
#define MAX_PARAMS_LEN      (1024)
typedef struct _UserData {
	int     buildFini;  //��ʶ�﷨�����Ƿ����
	int     updateFini; //��ʶ���´ʵ��Ƿ����
	int     errCode; //��¼�﷨��������´ʵ�ص�������
	char    grammarID[MAX_GRAMMARID_LEN]; //�����﷨�������ص��﷨ID
}UserData;
	/* wav��Ƶͷ����ʽ */
struct WavePcmHdr
{
	char            riff[4];                // = "RIFF"
	int				size_8;                 // = FileSize - 8
	char            wave[4];                // = "WAVE"
	char            fmt[4];                 // = "fmt "
	int				fmt_size;				// = ��һ���ṹ��Ĵ�С : 16

	short int       format_tag;             // = PCM : 1
	short int       channels;               // = ͨ���� : 1
	int				samples_per_sec;        // = ������ : 8000 | 6000 | 11025 | 16000
	int				avg_bytes_per_sec;      // = ÿ���ֽ��� : samples_per_sec * bits_per_sample / 8
	short int       block_align;            // = ÿ�������ֽ��� : wBitsPerSample / 8
	short int       bits_per_sample;        // = ����������: 8 | 16

	char            data[4];                // = "data";
	int				data_size;              // = �����ݳ��� : FileSize - 44 
};

class DDVoiceInteraction : public MainSubsModel
{
	DDVoiceInteraction();
public:
	static DDVoiceInteraction* GetInstance();

	~DDVoiceInteraction();

	bool Init(bool bOnline = false); // Initialize

	/*******************************************************
	** Text to speech function.
	** @text: text that needs to transform.
	** @fileIndex: the sound file named as: 'fileIndex.wav'.
	********************************************************/
	void RunTTS(const char * text, int fileIndex);

	/*******************************************************************
	** Set the callback of playing sounds.
	** @PlayCBFun: the callback function, and
	**             the params of the callback are as follows:
	** 			   @char* text: text needs to be played.
	********************************************************************/
	void SetPlayCallBackFun(std::function<void(char*)> PlayCBFun);


	std::shared_ptr<asio::streambuf> GetVoiceBuf(std::string& content);

protected:
	bool _onRunOnce_Sub(int) override;
	bool processFinishedTask_Main() override;//{ return true; }
	bool isTaskAllowed_Main() override;//{ return true; }

private:
	struct speech_rec mAsr;
	bool m_bInitSuccess;
	bool m_bLastTimeIsRun;
	bool mbOnline;
	int m_nConfidence;
	std::vector<std::pair<long long int, std::string>> mAnswers;

	void PlayVoice(char* text, int fileIndex = 0);
	std::function<void(char*)> PlaySoundFun;
	int TextToSpeech(const char* src_text, const char* des_path, const char* params);
	int CodeConvert(char *from_charset,
		char *to_charset,
		const char *inbuf,
		size_t inlen,
		char *outbuf,
		size_t outlen);
	//UNICODE��תΪGB2312��
	int U2G(const char *inbuf,
		unsigned int inlen,
		char *outbuf,
		unsigned int outlen);
	//GB2312��תΪUNICODE��
	int G2U(char *inbuf,
		size_t inlen,
		char *outbuf,
		size_t outlen);
	const char* GetResult();
	void ShowKeyHints();

	int BuildGrammar(UserData *udata);

	void WaitNoVoiceRun();
private:
	bool m_bEnterThread;
public:
	void SetSubThreadEnter(bool bIsEnter);
	bool GetSubThreadIsRun();
};

#endif