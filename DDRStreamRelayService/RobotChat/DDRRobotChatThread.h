#ifndef __DDVOICETHREAD__H__
#define __DDVOICETHREAD__H__

#include "DDVoiceInteraction.h"
#include "MyMTModules.h"
#include "SerialPortIOHandler.h"
#include <Windows.h>

using namespace DDRMTLib;
using namespace DDRVoice;
using namespace DDRDrivers;

namespace DDRRobotChat
{
	class DDRRobotChatThread : public MainSubsModel
	{
		DDRRobotChatThread();
	public:	
		static DDRRobotChatThread* GetInstance();
		~DDRRobotChatThread();

		bool Init(int nPort = 0);
		bool Start();
		bool Respond();
		bool Reset();
		void Quit();
		
	private:
		/*********************************
		** Example of callback function
		** @text: text needs to be played.
		**********************************/
		static void PlayVoice(char* text);
		bool CheckSerialDataIsWakeup();
		bool SetSerialSleep();

	protected:
		bool _onRunOnce_Sub(int) override;

	private:
		SerialPortIOHandler *m_pSerialRobotChat;	
		bool m_bDDVIsOk;
		DDVoiceInteraction *m_clsddv;
	};
}

#endif // __DDVOICETHREAD__H__