#include "DDRRobotChatThread.h"
#include <iostream>
#include <Windows.h>
#include <fstream>  
#include <playsoundapi.h>

using namespace DDRRobotChat;
void DDRRobotChatThread::PlayVoice(char* text)
{
//	m_clsddv.RunTTS(text, 0);
//	char filename[256] = { NULL };
//	sprintf(filename, ".\\%d.wav", 0);
//	PlaySoundA((LPCSTR)filename, NULL, SND_FILENAME);
}

DDRRobotChatThread* DDRRobotChatThread::GetInstance()
{
	DDRRobotChatThread * pChat = nullptr;
	if(!pChat)
	{
		pChat = new DDRRobotChatThread();
	}
	return pChat;
}

DDRRobotChatThread::DDRRobotChatThread() : MainSubsModel(1, 10, 10)
{
	m_pSerialRobotChat = nullptr;
	m_clsddv = nullptr;
	m_bDDVIsOk = false;
}

DDRRobotChatThread::~DDRRobotChatThread()
{

}

bool DDRRobotChatThread::Init(int nPort /*= 0*/)
{
	//std::cout << "@DDRRobotChatThread::Init() +++ nPort = " << nPort << std::endl;

	if (!m_clsddv->Init(false))
	{
		std::cout << "DDRRobotChatThread::Init():语音模块初始化失败，请检查相关硬件连接！" << std::endl;
		return false;
	}
	m_clsddv->SetPlayCallBackFun(PlayVoice);
	m_clsddv->Launch();

	if(!m_pSerialRobotChat)
	{
		m_pSerialRobotChat = new SerialPortIOHandler;
	}
	m_pSerialRobotChat->SetSerialPort(nPort, 115200);
	if(m_pSerialRobotChat->IsOpened())
	{
		return true;
	}
	m_bDDVIsOk = true;
	return m_pSerialRobotChat->Open();
}

bool DDRRobotChatThread::Start()
{
	//std::cout << "DDRRobotChatThread::Start() +++  " << std::endl;
	if (!Launch()) 
	{
		return Resume(); 
	}
	else
	{
		Respond(); // 要这个才能运行 _onRunOnce_Sub
		return true; 
	}
}

bool DDRRobotChatThread::Respond()
{
	MainSubsModel::Process();
	return IsLastCheckON();
}

bool DDRRobotChatThread::Reset()
{
	return Pause();
}

void DDRRobotChatThread::Quit()
{
	EndSubThreads();
}

bool DDRRobotChatThread::_onRunOnce_Sub(int)
{
	//std::cout << "DDRRobotChatThread::_onRunOnce_Sub() +++	" << std::endl;
	bool bPrevDDVRun = false;
	while (1)
	{
		if(m_bDDVIsOk)
		{
			if(m_clsddv->AreSubThreadsBusy())
			{
				static int ncount = 0;
				if(30 == ncount)
				{
					std::cout << "AreSubThreadsBusy is true " << std::endl;
					ncount = 0;
				}
				ncount++;
			}
			else
			{
				if(CheckSerialDataIsWakeup())
				{
					int nProcessret = m_clsddv->Process(); 
					std::cout << "DDRRobotChatThread::_onRunOnce_Sub() ddv.Process +++	nProcessret = " << nProcessret << std::endl;	
					bPrevDDVRun = true;
				}
				else if(bPrevDDVRun)
				{
					SetSerialSleep();// 让串口进入睡眠状态
					bPrevDDVRun = false;
				}

				if(bPrevDDVRun)
				{
					int nTemp = 0;
					while(1)
					{	
						Sleep(200);
						bool bBusy = m_clsddv->AreSubThreadsBusy();
						std::cout << "wait AreSubThreadsBusy is true ...... bBusy = " << bBusy << std::endl;
						if(bBusy)
						{
							break;
						}
						nTemp++;
						if(nTemp > 10)
							break;
					}
				}
			
				static int ncount = 0;
				if(30 == ncount)
				{
					std::cout << "AreSubThreadsBusy is false " << std::endl;
					ncount = 0;
				}
				ncount++;
			}
		}
		Sleep(100);
	}
	return true;
}

// 读取串口的内容，看看有没有唤醒
bool DDRRobotChatThread::CheckSerialDataIsWakeup()
{
	if(!m_pSerialRobotChat)
	{
		return false;
	}
	
	if(!m_pSerialRobotChat->IsOpened())
	{
		//std::cout << "DDRRobotChatThread::CheckSerialDataIsWakeup() Error serial not open --- " << std::endl;
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
		//std::cout << "DDRRobotChatThread::CheckSerialDataIsWakeup() wakeup data = " << strData.c_str() << std::endl;
		bret = true;
	}
	return bret;
}

// 让串口进入睡眠状态
bool  DDRRobotChatThread::SetSerialSleep()
{
	//std::cout << "DDRRobotChatThread::SetSerialSleep() +++ " << std::endl;
	if(!m_pSerialRobotChat)
	{
		return false;
	}

	if(!m_pSerialRobotChat->IsOpened())
	{
		//std::cout << "DDRRobotChatThread::SetSerialSleep() Error serial not open --- " << std::endl;
		return false;
	}

	if(!m_pSerialRobotChat->Write("set 0", 5))
	{
		//std::cout << "DDRRobotChatThread::SetSerialSleep() Write Error --- " << std::endl;
		return false;
	}
	return true;
}
