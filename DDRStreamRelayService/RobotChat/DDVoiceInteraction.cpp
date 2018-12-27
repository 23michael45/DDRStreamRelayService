#include "DDVoiceInteraction.h"
#include "TTSQueuePlayer.h"
#include <Windows.h>
#include "qtts.h"
#include "qisr.h"
#include "msp_cmn.h"
#include "msp_errors.h"
#include "cJSON.h"
#include "tinyxml2.h"
#pragma comment (lib, "RobotChat/libs/libiconv.lib")
//#include <iconv.h>
#include "iconv.h"
#include <string>
#include <iostream>
#include <fstream>
#include <playsoundapi.h>

using namespace DDRVoice;

static char *gResult = NULL;
static unsigned int gBuffersize = BUFFER_SIZE;

const char * ASR_RES_PATH = "fo|res/asr/common.jet";  //离线语法识别资源路径
#ifdef _WIN64
const char * GRM_BUILD_PATH = "res/asr/GrmBuilld_x64";  //构建离线语法识别网络生成数据保存路径
#else
const char * GRM_BUILD_PATH = "res/asr/GrmBuilld";  //构建离线语法识别网络生成数据保存路径
#endif
const char * GRM_FILE = "config/grammar.bnf"; //构建离线识别语法网络所用的语法文件
const char * ANSWER_FILE = "config/answers.txt";
const char * SUBCONFIG_FILE = "config/DDRSubConfig.txt";

void OnResult(const char *result, char isLast)
{
	if (result) {
		size_t left = gBuffersize - 1 - strlen(gResult);
		size_t size = strlen(result);
		if (left < size) {
			gResult = (char*)realloc(gResult, gBuffersize + BUFFER_SIZE);
			if (gResult)
				gBuffersize += BUFFER_SIZE;
			else {
				printf("mem alloc failed\n");
				return;
			}
		}
		strncat(gResult, result, size);
		//ShowResult(gResult, isLast);
	}
}

void OnSpeechBegin()
{
	if (gResult)
	{
		free(gResult);
	}
	gResult = (char*)malloc(BUFFER_SIZE);
	gBuffersize = BUFFER_SIZE;
	memset(gResult, 0, gBuffersize);
	//printf("Start Listening...\n");
}

void OnSpeechEnd(int reason)
{
	if (reason == END_REASON_VAD_DETECT)
		/*printf("\nSpeaking done \n")*/;
	else
		printf("\nRecognizer error %d\n", reason);
}

int BuildGrmCallback(int ecode, const char *info, void *udata)
{
	UserData *grmData = (UserData *)udata;

	if (NULL != grmData) {
		grmData->buildFini = 1;
		grmData->errCode = ecode;
	}

	if (MSP_SUCCESS == ecode && NULL != info) {
		printf("构建语法成功！ 语法ID:%s\n", info);
		if (NULL != grmData)
			_snprintf(grmData->grammarID, MAX_GRAMMARID_LEN - 1, info);
	}
	else
		printf("构建语法失败！%d\n", ecode);

	return 0;
}

std::string GetSubConfigValue(std::string strTargetType)
{
	char buffer[256];
	std::ifstream subConfigfile(SUBCONFIG_FILE);
	if (!subConfigfile.is_open())
	{
		std::cout << "Error opening file" << std::endl;
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
				//std::cout << "Type:" << strType.c_str() << " Value:" << strValue.c_str() << std::endl;
				if (strTargetType == strType)
				{
					break;
				}
				else
				{
					strValue = "";
				}
			}
		}
	}
	subConfigfile.close();
	return strValue;
}


DDVoiceInteraction::DDVoiceInteraction()
	/*: MainSubsModel(1, 0, 100)*/
	: MainSubsModel(1, 10, 100)
{
	m_pSerialRobotChat = nullptr;
	m_bInitSuccess = false;
	m_bLastTimeIsRun = false;
	m_bEnterThread = false;
	/*if (!Init())
	{
		std::cout << "@DDVoiceInteraction::DDVoiceInteraction():语音模块初始化失败，请检查相关硬件连接！" << std::endl;
		return;
	}*/

	//ShowKeyHints();
	m_nConfidence = 13;
}

DDVoiceInteraction* DDVoiceInteraction::GetInstance()
{
	DDVoiceInteraction * pChat = nullptr;
	if(!pChat)
	{
		pChat = new DDVoiceInteraction();
	}
	return pChat;

}


DDVoiceInteraction::~DDVoiceInteraction()
{
	sr_uninit(&mAsr);
	MSPLogout();
}

const char* DDVoiceInteraction::GetResult()
{
	return gResult;
}

int DDVoiceInteraction::CodeConvert(char *from_charset,
	                                char *to_charset,
									const char *inbuf,
									size_t inlen,
									char *outbuf,
									size_t outlen)
{
	iconv_t cd;
	//int rc;
	const char **pin = &inbuf;
	char **pout = &outbuf;

	cd = iconv_open(to_charset, from_charset);
	if (cd == 0) return -1;
	memset(outbuf, 0, outlen);
	if (iconv(cd, pin, &inlen, pout, &outlen) == -1) return -1;
	iconv_close(cd);
	return 0;
}

int DDVoiceInteraction::U2G(const char *inbuf,
	                        unsigned int inlen,
							char *outbuf,
							unsigned int outlen)
{
	return CodeConvert("utf-8", "gb2312", inbuf, inlen, outbuf, outlen);
}

int DDVoiceInteraction::G2U(char *inbuf,
	                        size_t inlen,
							char *outbuf,
							size_t outlen)
{
	return CodeConvert("gb2312", "utf-8", inbuf, inlen, outbuf, outlen);
}

bool DDVoiceInteraction::Init(bool bOnline, int nPort)
{
	if(m_bInitSuccess)
	{
		printf("DDVoiceInteraction::Init() +++ \n");
		return true;
	}
	
	if(!m_pSerialRobotChat)
	{
		m_pSerialRobotChat = new SerialPortIOHandler;
	}
	m_pSerialRobotChat->SetSerialPort(nPort, 115200);
	m_pSerialRobotChat->Open();
	if(!m_pSerialRobotChat->IsOpened())
	{
		//printf("DDVoiceInteraction::Init() Open Serial error. no liumai\n");
		//return false;
	}

	mbOnline = bOnline;
	int			ret = MSP_SUCCESS;
	const char* loginParams = "appid = 57f5ff4e, work_dir = ."; // 登录参数，appid与msc库绑定,请勿随意改动
	ret = MSPLogin(NULL, NULL, loginParams); //第一个参数是用户名，第二个参数是密码，均传NULL即可，第三个参数是登录参数	
	if (MSP_SUCCESS != ret)	{
		printf("MSPLogin failed , Error code %d.\n", ret);
		MSPLogout();
		return false;
	}

	char sessionBeginParams[MAX_PARAMS_LEN] = { NULL };
	if (mbOnline)
	{
		_snprintf(sessionBeginParams, MAX_PARAMS_LEN - 1,
			      "sub = iat, domain = iat, \
				   language = zh_cn, \
				   accent = mandarin, \
				   sample_rate = 16000, \
				   result_type = plain, \
				   result_encoding = gb2312");
	}
	else
	{
		std::ifstream fin(ANSWER_FILE);
		if (!fin.is_open())
		{
			printf("无法打开问答文件，请检查文件是否存在！");
			return false;
		}
		while (!fin.eof())
		{
			long long int id;
			std::string answer;
			fin >> id >> answer;
			mAnswers.push_back(std::make_pair(id, answer));
		}
		UserData asrData;
		memset(&asrData, 0, sizeof(UserData));
		printf("构建离线识别语法网络...\n");
		ret = 0;
		ret = BuildGrammar(&asrData);  //第一次使用某语法进行识别，需要先构建语法网络，获取语法ID，之后使用此语法进行识别，无需再次构建
		if (MSP_SUCCESS != ret) {
			printf("构建语法调用失败！\n");
			return false;
		}
		while (1 != asrData.buildFini)
			Sleep(300);
		if (MSP_SUCCESS != asrData.errCode)
			return false;
		_snprintf(sessionBeginParams, MAX_PARAMS_LEN - 1,
			      "engine_type = local, \
				   asr_res_path = %s, sample_rate = %d, \
				   grm_build_path = %s, local_grammar = %s, \
				   result_type = xml, result_encoding = GB2312, ",
				  ASR_RES_PATH,
				  SAMPLE_RATE_16K,
				  GRM_BUILD_PATH,
				  asrData.grammarID);
	}

	//const char* sessionBeginParams = "sub = iat, domain = iat, language = zh_cn, accent = mandarin, sample_rate = 16000, result_type = plain, result_encoding = gb2312";
	int errCode;
	//struct speech_rec asr;
	struct speech_rec_notifier recnotifier = {
		OnResult,
		OnSpeechBegin,
		OnSpeechEnd
	};
	errCode = sr_init(&mAsr, sessionBeginParams, SR_MIC, DEFAULT_INPUT_DEVID, &recnotifier);
	if (errCode) {
		printf("speech recognizer init failed errCode = %d\n", errCode);
		return false;
	}

	// xcy add
	Launch();
	m_bInitSuccess = true;
	return true;
}

int DDVoiceInteraction::BuildGrammar(UserData *udata)
{
	FILE *grmFile = NULL;
	char *grmContent = NULL;
	unsigned int grmCntLen = 0;
	char grmBuildParams[MAX_PARAMS_LEN] = { NULL };
	int ret = 0;

	grmFile = fopen(GRM_FILE, "rb");
	if (NULL == grmFile) {
		printf("打开\"%s\"文件失败！[%s]\n", GRM_FILE, strerror(errno));
		return -1;
	}

	fseek(grmFile, 0, SEEK_END);
	grmCntLen = ftell(grmFile);
	fseek(grmFile, 0, SEEK_SET);

	grmContent = (char *)malloc(grmCntLen + 1);
	if (NULL == grmContent)
	{
		printf("内存分配失败!\n");
		fclose(grmFile);
		grmFile = NULL;
		return -1;
	}
	fread((void*)grmContent, 1, grmCntLen, grmFile);
	grmContent[grmCntLen] = '\0';
	fclose(grmFile);
	grmFile = NULL;

	_snprintf(grmBuildParams, MAX_PARAMS_LEN - 1,
		"engine_type = local, \
						asr_res_path = %s, sample_rate = %d, \
												grm_build_path = %s, ",
												ASR_RES_PATH,
												SAMPLE_RATE_16K,
												GRM_BUILD_PATH
												);
	ret = QISRBuildGrammar("bnf", grmContent, grmCntLen, grmBuildParams, BuildGrmCallback, udata);

	free(grmContent);
	grmContent = NULL;

	return ret;
}

void DDVoiceInteraction::ShowKeyHints()
{
	printf("\n\
		   	------------------------------------\n\
			请说“灵犀灵犀”唤醒大白\n\
			唤醒小智后如果长时间不交流请再次唤醒\n\
			------------------------------------\n");
}

int DDVoiceInteraction::TextToSpeech(const char* src_text, const char* des_path, const char* params)
{
	int          ret = -1;
	FILE*        fp = NULL;
	const char*  sessionID = NULL;
	unsigned int audio_len = 0;
	WavePcmHdr wav_hdr = {
		{ 'R', 'I', 'F', 'F' },
		0,
		{ 'W', 'A', 'V', 'E' },
		{ 'f', 'm', 't', ' ' },
		16,
		1,
		1,
		16000,
		32000,
		2,
		16,
		{ 'd', 'a', 't', 'a' },
		0
	};
	int          synth_status = MSP_TTS_FLAG_STILL_HAVE_DATA;

	if (NULL == src_text || NULL == des_path)
	{
		printf("params is error!\n");
		return ret;
	}
	fp = fopen(des_path, "wb");
	if (NULL == fp)
	{
		printf("open %s error.\n", des_path);
		return ret;
	}
	/* 开始合成 */
	sessionID = QTTSSessionBegin(params, &ret);
	if (MSP_SUCCESS != ret)
	{
		printf("QTTSSessionBegin failed, error code: %d.\n", ret);
		fclose(fp);
		return ret;
	}
	ret = QTTSTextPut(sessionID, src_text, (unsigned int)strlen(src_text), NULL);
	if (MSP_SUCCESS != ret)
	{
		printf("QTTSTextPut failed, error code: %d.\n", ret);
		QTTSSessionEnd(sessionID, "TextPutError");
		fclose(fp);
		return ret;
	}
	//printf("正在合成 ...\n");
	fwrite(&wav_hdr, sizeof(wav_hdr), 1, fp); //添加wav音频头，使用采样率为16000
	while (1)
	{
		/* 获取合成音频 */
		const void* data = QTTSAudioGet(sessionID, &audio_len, &synth_status, &ret);
		if (MSP_SUCCESS != ret)
			break;
		if (NULL != data)
		{
			fwrite(data, audio_len, 1, fp);
			wav_hdr.data_size += audio_len; //计算data_size大小
		}
		if (MSP_TTS_FLAG_DATA_END == synth_status)
			break;
	}
	//printf("\n");
	if (MSP_SUCCESS != ret)
	{
		printf("QTTSAudioGet failed, error code: %d.\n", ret);
		QTTSSessionEnd(sessionID, "AudioGetError");
		fclose(fp);
		return ret;
	}
	/* 修正wav文件头数据的大小 */
	wav_hdr.size_8 += wav_hdr.data_size + (sizeof(wav_hdr) - 8);

	/* 将修正过的数据写回文件头部,音频文件为wav格式 */
	fseek(fp, 4, 0);
	fwrite(&wav_hdr.size_8, sizeof(wav_hdr.size_8), 1, fp); //写入size_8的值
	fseek(fp, 40, 0); //将文件指针偏移到存储data_size值的位置
	fwrite(&wav_hdr.data_size, sizeof(wav_hdr.data_size), 1, fp); //写入data_size的值
	fclose(fp);
	fp = NULL;
	/* 合成完毕 */
	ret = QTTSSessionEnd(sessionID, "Normal");
	if (MSP_SUCCESS != ret)
	{
		printf("QTTSSessionEnd failed, error code: %d.\n", ret);
	}

	return ret;
}

void DDVoiceInteraction::RunTTS(const char * text, int fileIndex)
{
	int         ret = MSP_SUCCESS;
	//const char* login_params = "appid = 57f5ff4e, work_dir = .";//登录参数,appid与msc库绑定,请勿随意改动
	/*
	* rdn:           合成音频数字发音方式
	* volume:        合成音频的音量
	* pitch:         合成音频的音调
	* speed:         合成音频对应的语速
	* voice_name:    合成发音人
	* sample_rate:   合成音频采样率
	* text_encoding: 合成文本编码格式
	*
	*/
	const char* session_begin_params = "engine_type = local, voice_name = xiaoyan, text_encoding = GB2312, tts_res_path = fo|res\\tts\\xiaoyan.jet;fo|res\\tts\\common.jet, sample_rate = 16000, speed = 50, volume = 50, pitch = 50, rdn = 2";
	const char* filename;// = "DDtts.wav"; //合成的语音文件名称
	char tmp[256] = { NULL };
	sprintf(tmp, ".\\%d.wav", fileIndex);
	filename = tmp;

	/* 文本合成 */
	//printf("开始合成 ...\n");
	ret = TextToSpeech(text, filename, session_begin_params);
	if (MSP_SUCCESS != ret)
	{
		printf("text_to_speech failed, error code: %d.\n", ret);
	}
	//PlaySoundA(NULL, NULL, SND_FILENAME | SND_ASYNC);
	//PlaySoundA((LPCSTR)filename, NULL, SND_FILENAME | SND_ASYNC); //用PlaySound函数播放a.wav文件
	//PlaySoundA((LPCSTR)filename, NULL, SND_FILENAME);
}

void DDVoiceInteraction::PlayVoice(char* text, int fileIndex)
{
	RunTTS(text, 0);
	char filename[256] = { NULL };
	sprintf(filename, ".\\%d.wav", 0);
	PlaySoundA((LPCSTR)filename, NULL, SND_FILENAME);

	/*
	char filename[256] = { NULL };
	sprintf(filename, ".\\%d.wav", fileIndex);
	PlaySoundA((LPCSTR)filename, NULL, SND_FILENAME);
	*/
}

void DDVoiceInteraction::SetPlayCallBackFun(std::function<void(char*)> PlayCBFun)
{
	PlaySoundFun = std::bind(PlayCBFun, std::placeholders::_1);
}

// 读取串口的内容，看看有没有唤醒
bool DDVoiceInteraction::CheckSerialDataIsWakeup()
{
	if(!m_pSerialRobotChat)
	{
		return false;
	}
	
	if(!m_pSerialRobotChat->IsOpened())
	{
		//std::cout << "DDVoiceInteraction::CheckSerialDataIsWakeup() Error serial not open --- " << std::endl;
		return false;
	}
	bool bret = false;
	char pBuf[64] = {};
	int pBufLen = sizeof(pBuf)/sizeof(char);
	int actualLenRead = pBufLen;
	m_pSerialRobotChat->Read(pBuf, pBufLen - 1, actualLenRead);
	pBuf[actualLenRead] = '\0';
	std::string strData(pBuf);
	if(-1 != strData.find("wakeup"))
	{
		//std::cout << "DDVoiceInteraction::CheckSerialDataIsWakeup() wakeup data = " << strData.c_str() << std::endl;
		bret = true;
	}
	return bret;
}

// 让串口进入睡眠状态
bool  DDVoiceInteraction::SetSerialSleep(bool bIsWakeUp)
{
	//std::cout << "DDVoiceInteraction::SetSerialSleep() +++ bIsWakeUp = " << bIsWakeUp << std::endl;
	if(!m_pSerialRobotChat)
	{
		return false;
	}

	if(!m_pSerialRobotChat->IsOpened())
	{
		//std::cout << "DDVoiceInteraction::SetSerialSleep() Error serial not open --- " << std::endl;
		return false;
	}

	if(bIsWakeUp) // 唤醒
	{
		if(!m_pSerialRobotChat->Write("set 0", 5))
		{
			//std::cout << "DDVoiceInteraction::SetSerialSleep() Write Error1 --- " << std::endl;
			return false;
		}

		//if(!m_pSerialRobotChat->Write("set 1", 5))
		{
			//std::cout << "DDVoiceInteraction::SetSerialSleep() Write Error1 --- " << std::endl;
			//return false;
		}
	}
	else // 关闭
	{
		if(!m_pSerialRobotChat->Write("set s", 5))
		{
			//std::cout << "DDVoiceInteraction::SetSerialSleep() Write Error2 --- " << std::endl;
			return false;
		}
	}
	return true;
}

/*
	等待当前语音播放完了
*/
void DDVoiceInteraction::WaitNoVoiceRun()
{
	while(1)
	{
		unsigned int nCurrTTSChannel = DDRGadgets::GetCurrentTTSChannel();
		if(0xffffffff == nCurrTTSChannel)
		{
			//std::cout << "WaitNoVoiceRun() curr no playing" << std::endl;
			return;
		}
		//std::cout << "WaitNoVoiceRun() curr playing ..." << std::endl;
		Sleep(100);
	}
}


bool DDVoiceInteraction::_onRunOnce_Sub(int)
{
	std::cout << "聊天线程已开启" << (mbOnline ? "(在线聊天)": "(离线聊天)") << " +++++++++++++++ " << std::endl;

	char* text = "害，我来了\0";
	PlaySoundFun(text);	
	
	m_bLastTimeIsRun = true;
	
	int countTime = 5;
	int count = 0;
	int errCode;
	int ret = 0;
	char rcdText[BUFFER_SIZE] = { NULL };
	bool bQuitChatting = false;
	while (1)
	{
		WaitNoVoiceRun();
		if (errCode = sr_start_listening(&mAsr)) {
			printf("start listen failed %d\n", errCode);
		}
		Sleep(3000);
		if (errCode = sr_stop_listening(&mAsr)) {
			//printf("stop listening failed %d\n", errCode);
		}

		if (errCode == 10114)
		{
			char* text = "网络信号不好，请稍后再次尝试\0";
			PlaySoundFun(text);
		}
		if (gResult != NULL && strcmp(gResult, "") != 0)
		{
			count = 0;
			if (mbOnline)
			{
				sprintf(rcdText, GetResult());			
				unsigned int strLen = 0;
				const char*  recText = NULL;
				char recTextu[OUTLEN] = { NULL };
				char out[OUTLEN] = { NULL };
				char u2gOut[OUTLEN] = { NULL };
				char recTextFinal[OUTLEN] = { NULL };

				G2U(rcdText, strlen(rcdText), out, OUTLEN);
				strLen = strlen(out);
				//printf("out: %s", out);
				recText = MSPSearch("nlp_version=2.0", out, &strLen, &ret);
				if (MSP_SUCCESS != ret)
				{
					printf("MSPSearch failed ,error code is:%d\n", ret);
					char* text = "不好意思，云端大脑有点走神请靠近点再说一遍\0";
					PlaySoundFun(text);
					std::cout << "聊天线程已退出" << (mbOnline ? "(在线聊天)" : "(离线聊天)") << " --------11-------- " << std::endl;
					return true;
					//goto exit;
				}
				U2G(out, strlen(out), u2gOut, OUTLEN);
				if (strcmp(u2gOut, "再见") == 0 || strcmp(u2gOut, "再见。") == 0
					|| strcmp(u2gOut, "拜拜") == 0 || strcmp(u2gOut, "拜拜。") == 0)
				{
					bQuitChatting = true;
				}
				U2G(recText, strlen(recText), recTextu, OUTLEN);
				strcpy(recTextFinal, recTextu);
				cJSON * root = cJSON_Parse(recTextFinal);
				int rc = cJSON_GetObjectItem(root, "rc")->valueint;
				//printf("rc=%d\n",rc);
				if (rc == 1 || rc == 2 || rc == 3 || rc == 4)
				{
					char* text = "不好意思，云端大脑有点走神请再说一遍\0";
					PlaySoundFun(text);
					cJSON_Delete(root);
					std::cout << "聊天线程已退出" << (mbOnline ? "(在线聊天)" : "(离线聊天)") << " --------22-------- " << std::endl;
					return true;
				}
				cJSON * answer = cJSON_GetObjectItem(root, "answer");
				char* answerText = cJSON_GetObjectItem(answer, "text")->valuestring;
				//printf("\n answer is %s\n", answerText);
				PlaySoundFun(answerText);
			}
			else
			{
				int confidence;
				tinyxml2::XMLDocument doc;
				doc.Parse(gResult);
				tinyxml2::XMLElement* confiElement = doc.FirstChildElement("nlp");
				confiElement->FirstChildElement("confidence")->QueryIntText(&confidence);
				//printf("%s\n", gResult);
				//printf("%d\n", confidence);
				if (confidence > m_nConfidence)
				{
					tinyxml2::XMLElement* contentElement = doc.FirstChildElement("nlp")->
						FirstChildElement("result")->FirstChildElement("object")->FirstChildElement("contents");
					const char * contents = contentElement->GetText();
					int contentID;
					long long int id;
					contentElement->QueryIntAttribute("id", &contentID);
					if (contentID == 20006 || contentID == 20007)
					{
						bQuitChatting = true;
					}
					id = contentID;
					char answer[BUFFER_SIZE];
					memset(answer, 0, sizeof(answer));
					for (unsigned int i = 0; i < mAnswers.size(); i++)
					{
						if (mAnswers[i].first == id)
						{
							strcpy(answer, mAnswers[i].second.c_str());
							break;
						}
					}
					//printf("ID: %lld\n", id);
					if (strcmp(answer, "") != 0)// && quit == 0)
					{
						PlaySoundFun(answer);
					}
						
				}
				else
				{
					//char * pardon = "不好意思，大白没听懂，请再说一遍";
					char * pardon = "请靠近小智一点，这样我可以听的更清楚";
					PlaySoundFun(pardon);
				}
			}
		}
		else
			count++;

		if (bQuitChatting)
		{
			count = 0;
			//mbStartChatting = false;
			bQuitChatting = false;
			sr_stop_listening(&mAsr);
			break;
		}
		else if (count >= countTime)
		{
			char * quitLogan = "祝您每天好心情，再见\0";
			count = 0;
			//mbStartChatting = false;
			bQuitChatting = false;
			sr_stop_listening(&mAsr);
			PlaySoundFun(quitLogan);
			break;
		}
	}
	std::cout << "聊天线程已退出" << (mbOnline ? "(在线聊天)" : "(离线聊天)") << " --------00-------- " << std::endl;
	return true;
}

bool  DDVoiceInteraction::processFinishedTask_Main()
{
	if(m_bLastTimeIsRun)
	{
		m_bLastTimeIsRun = false;
		m_bEnterThread = false;
		//std::cout << "DDVoiceInteraction() Quit chat ..." << std::endl;
		SetSerialSleep();
	}
	
	return true;
}

bool DDVoiceInteraction::isTaskAllowed_Main()
{
#if 0
	bool bret = CheckSerialDataIsWakeup();
	if(bret)
	{
		//m_bLastTimeIsRun = true;
		std::cout << "DDVoiceInteraction::isTaskAllowed_Main() is true" << std::endl;
	}
	return bret;
#else
	if(m_bEnterThread)
	{
		//std::cout << "DDVoiceInteraction() Enter chat ..." << std::endl;
		SetSerialSleep(true);
	}
	
	return m_bEnterThread;
#endif
}

void DDVoiceInteraction::SetSubThreadEnter(bool bIsEnter)
{
	m_bEnterThread = bIsEnter;
}

bool DDVoiceInteraction::GetSubThreadIsRun()
{
	return m_bLastTimeIsRun;
}


