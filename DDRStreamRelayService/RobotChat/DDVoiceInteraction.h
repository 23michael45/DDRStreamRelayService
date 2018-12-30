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
	int     buildFini;  //标识语法构建是否完成
	int     updateFini; //标识更新词典是否完成
	int     errCode; //记录语法构建或更新词典回调错误码
	char    grammarID[MAX_GRAMMARID_LEN]; //保存语法构建返回的语法ID
}UserData;
	/* wav音频头部格式 */
struct WavePcmHdr
{
	char            riff[4];                // = "RIFF"
	int				size_8;                 // = FileSize - 8
	char            wave[4];                // = "WAVE"
	char            fmt[4];                 // = "fmt "
	int				fmt_size;				// = 下一个结构体的大小 : 16

	short int       format_tag;             // = PCM : 1
	short int       channels;               // = 通道数 : 1
	int				samples_per_sec;        // = 采样率 : 8000 | 6000 | 11025 | 16000
	int				avg_bytes_per_sec;      // = 每秒字节数 : samples_per_sec * bits_per_sample / 8
	short int       block_align;            // = 每采样点字节数 : wBitsPerSample / 8
	short int       bits_per_sample;        // = 量化比特数: 8 | 16

	char            data[4];                // = "data";
	int				data_size;              // = 纯数据长度 : FileSize - 44 
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
	//UNICODE码转为GB2312码
	int U2G(const char *inbuf,
		unsigned int inlen,
		char *outbuf,
		unsigned int outlen);
	//GB2312码转为UNICODE码
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