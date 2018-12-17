#include "LSM.h"

#include <string.h>
#include <iostream>
#include <thread>
#include "CommLib/CommonFunc.h"
#include "CommLib/FileCache.h"
#include "LSMComp.h"

namespace DDRCommLib { namespace MARS {

struct LSMComm::__IMPL {
	bool m_bValid;
	SharedInfo m_shared;
	LSMComp *m_pCore;
	bool m_bSubTh2Quit;
	std::thread m_subTh;
	std::vector<char> tVec, tVec2, tVec3;

	__IMPL(const char *pModuleName, const char *pLSName)
		: m_shared(pModuleName, pLSName)
	{
		m_bValid = false;
		if (!m_shared.m_bValid) {
			return;
		}
		m_pCore = new LSMComp(&m_shared);
		if (!m_pCore) {
			return;
		}
		if (!m_pCore->IsValid()) {
			delete m_pCore;
			return;
		}
		m_bValid = true;
	}

	bool Start()
	{
		if (!m_bValid) {
			return false;
		}
		if (m_subTh.joinable()) {
			return true;
		}
		m_bSubTh2Quit = false;
		m_subTh = std::thread(_threadFunc, (void*)this);
		return true;
	}

	bool EnableCmdRcv()
	{
		if (!m_subTh.joinable()) {
			m_shared.m_cmdRcv = std::shared_ptr<SafeMsgQueue>(new SafeMsgQueue);
			return true;
		}
		return false;
	}
	bool EnableCmdSnd()
	{
		if (!m_subTh.joinable()) {
			m_shared.m_cmdSnd = std::shared_ptr<SafeMsgQueue>(new SafeMsgQueue);
			return true;
		}
		return false;
	}	
	bool EnableCmdRespSnd()
	{
		if (!m_subTh.joinable()) {
			m_shared.m_cmdRespSnd = std::shared_ptr<SafeMsgQueue>(new SafeMsgQueue);
			return true;
		}
		return false;
	}
	bool EnableCmdRespRcv()
	{
		if (!m_subTh.joinable()) {
			m_shared.m_cmdRespRcv = std::shared_ptr<SafeMsgQueue>(new SafeMsgQueue);
			return true;
		}
		return false;
	}
	bool EnableAlarmRcv()
	{
		if (!m_subTh.joinable()) {
			m_shared.m_alarmRcv = std::shared_ptr<SafeMsgQueue>(new SafeMsgQueue);
			return true;
		}
		return false;
	}
	bool EnableAlarmSnd()
	{
		if (!m_subTh.joinable()) {
			m_shared.m_alarmSnd = std::shared_ptr<SafeMsgQueue>(new SafeMsgQueue);
			return true;
		}
		return false;
	}
	bool EnableStatusRcv()
	{
		if (!m_subTh.joinable()) {
			m_shared.m_statusRcv = std::shared_ptr<SafeMsgQueue>(new SafeMsgQueue);
			return true;
		}
		return false;
	}
	bool EnableStatusSnd()
	{
		if (!m_subTh.joinable()) {
			m_shared.m_statusSnd = std::shared_ptr<SafeMsgQueue>(new SafeMsgQueue);
			return true;
		}
		return false;
	}
	bool EnableFileInq()
	{
		if (!m_subTh.joinable()) {
			m_shared.m_fileRespSnd = std::shared_ptr<SafeMsgQueue>(new SafeMsgQueue);
			return true;
		}
		return false;
	}
	bool EnableAVStreamHandling(bool bAlwaysAnswer)
	{
		if (!m_subTh.joinable()) {
			return m_shared.EnableAVStreamHandling(bAlwaysAnswer);
		}
		return false;
	}

	~__IMPL() {
		if (m_subTh.joinable()) {
			m_bSubTh2Quit = true;
			m_subTh.join();
		}
		if (m_pCore) {
			delete m_pCore;
		}
	}

	void _runThis() {
		while (!m_bSubTh2Quit) {
			m_pCore->Process();
			Sleep(1);
		}
	}

	static void _threadFunc(void *pArg) {
		((__IMPL*)pArg)->_runThis();
	}
};

bool LSMComm::EnableCmdRcv()
{
	if (m_pImp && m_pImp->m_bValid) {
		return m_pImp->EnableCmdRcv();
	}
	return false;
}
bool LSMComm::EnableCmdSnd()
{
	if (m_pImp && m_pImp->m_bValid) {
		return m_pImp->EnableCmdSnd();
	}
	return false;
}

bool LSMComm::EnableCmdRespSnd()
{
	if (m_pImp && m_pImp->m_bValid) {
		return m_pImp->EnableCmdRespSnd();
	}
	return false;
}

bool LSMComm::EnableCmdRespRcv()
{
	if (m_pImp && m_pImp->m_bValid) {
		return m_pImp->EnableCmdRespRcv();
	}
	return false;
}

bool LSMComm::EnableAlarmRcv()
{
	if (m_pImp && m_pImp->m_bValid) {
		return m_pImp->EnableAlarmRcv();
	}
	return false;
}

bool LSMComm::EnableAlarmSnd()
{
	if (m_pImp && m_pImp->m_bValid) {
		return m_pImp->EnableAlarmSnd();
	}
	return false;
}

bool LSMComm::EnableStatusRcv()
{
	if (m_pImp && m_pImp->m_bValid) {
		return m_pImp->EnableStatusRcv();
	}
	return false;
}

bool LSMComm::EnableStatusSnd()
{
	if (m_pImp && m_pImp->m_bValid) {
		return m_pImp->EnableStatusSnd();
	}
	return false;
}

bool LSMComm::EnableFileInq()
{
	if (m_pImp && m_pImp->m_bValid) {
		return m_pImp->EnableFileInq();
	}
	return false;
}

bool LSMComm::EnableAVStreamHandling(bool bAlwaysAnswerTalk)
{
	if (m_pImp && m_pImp->m_bValid) {
		return m_pImp->EnableAVStreamHandling(bAlwaysAnswerTalk);
	}
	return false;
}

LSMComm::LSMComm(const char *pModuleName, const char *pLocalServerName)
{
	m_pImp = new __IMPL(pModuleName, pLocalServerName);
}

bool LSMComm::Start()
{
	if (m_pImp && m_pImp->m_bValid) {
		return m_pImp->Start();
	}
	return false;
}


LSMComm::~LSMComm() {
	if (m_pImp) {
		delete m_pImp;
	}
}

int LSMComm::GetOpMode() const {
	if (m_pImp && m_pImp->m_bValid && 0 == m_pImp->m_shared.m_connStat) {
		return m_pImp->m_shared.m_nOpMode;
	}
	return -1;
}

int LSMComm::GetNumOfMonitors() const {
	if (m_pImp && m_pImp->m_bValid && 0 == m_pImp->m_shared.m_connStat) {
		return m_pImp->m_shared.m_nMonitors;
	}
	return -1;
}

int LSMComm::GetNumOfClients() const {
	if (m_pImp && m_pImp->m_bValid && 0 == m_pImp->m_shared.m_connStat) {
		return m_pImp->m_shared.m_nClients;
	}
	return -1;
}

bool LSMComm::GetPushInfo(_oneAVChannel2Push *pStreamPushingInfo) {
	if (m_pImp && m_pImp->m_bValid) {
		m_pImp->m_shared.m_chLock.lock();
		for (int i = 0; i < MAX_AUDIOVIDEO_CHANNELS; ++i) {
			pStreamPushingInfo[i] = m_pImp->m_shared.m_PushChs[i];
		}
		m_pImp->m_shared.m_chLock.unlock();
		return true;
	}
	return false;
}

bool LSMComm::GetPullInfo(int &avType, std::string &str) {
	if (m_pImp && m_pImp->m_bValid) {
		m_pImp->m_shared.m_chLock.lock();
		avType = m_pImp->m_shared.m_pPullCh->avType;
		str = m_pImp->m_shared.m_pPullCh->accStr;
		m_pImp->m_shared.m_chLock.unlock();
		return true;
	}
	return false;
}

int LSMComm::GetConnStat() const {
	if (m_pImp && m_pImp->m_bValid) {
		return m_pImp->m_shared.m_connStat;
	}
	return -255;
}

int LSMComm::Talk_GetState() const {
	if (m_pImp && m_pImp->m_bValid) {
		if (m_pImp->m_shared.m_pTalkHelper) {
			return m_pImp->m_shared.m_pTalkHelper->GetState();
		}
	}
	return -1;
}

bool LSMComm::Talk_Call() {
	if (m_pImp && m_pImp->m_bValid) {
		if (m_pImp->m_shared.m_pTalkHelper) {
			return m_pImp->m_shared.m_pTalkHelper->Call();
		}
	}
	return false;
}

bool LSMComm::Talk_PickUp() {
	if (m_pImp && m_pImp->m_bValid) {
		if (m_pImp->m_shared.m_pTalkHelper) {
			return m_pImp->m_shared.m_pTalkHelper->PickUp();
		}
	}
	return false;
}

bool LSMComm::Talk_HangUp() {
	if (m_pImp && m_pImp->m_bValid) {
		if (m_pImp->m_shared.m_pTalkHelper) {
			return m_pImp->m_shared.m_pTalkHelper->HangUp();
		}
	}
	return false;
}

bool LSMComm::ProcessCommand(CALLBACK_CmdProcessor pCBCmdProcFunc, void *pArg) {
	if (!m_pImp || !m_pImp->m_bValid || !m_pImp->m_shared.m_cmdRcv.get()) {
		return false;
	}

	std::vector<char> &vec = m_pImp->tVec;
	std::vector<char> &vec2 = m_pImp->tVec2;
	std::vector<char> &vec3 = m_pImp->tVec3;
	bool bRespQOkay = (m_pImp->m_shared.m_cmdRespSnd.get() != nullptr);

	bool bRet = false;
	while (1) {
		if (!m_pImp->m_shared.m_cmdRcv->PopMsg(vec)) {
			break;
		}
		const char *pKeyword, *pCmdArg;
		int nCmdArgLen;
		if (!Parse_Command(&vec[0], (int)vec.size(), &pKeyword, &pCmdArg, &nCmdArgLen)) {
			continue;
		}
		bool bResp;
		if (pCBCmdProcFunc(pKeyword, pCmdArg + nCmdArgLen - pKeyword, pArg,
			               bResp, vec2)) {
			bRet = true;
			if (bResp && bRespQOkay) {
				if (vec2.size() > 0) {
					Gen_CmdResp(&vec[0], (int)vec.size(), pKeyword, &vec2[0], (int)vec2.size(), vec3);
				} else {
					Gen_CmdResp(&vec[0], (int)vec.size(), pKeyword, nullptr, 0, vec3);
				}
				m_pImp->m_shared.m_cmdRespSnd->AddMsg(&vec3[0], (int)vec3.size());
			}
		}
	}

	return bRet;
}


bool LSMComm::SendStatusUpdate(const char *pStatusBytes, int nStatusLen) {
	if (!pStatusBytes || nStatusLen <= 0) { return false; }
	if (m_pImp && m_pImp->m_bValid && m_pImp->m_shared.m_statusSnd.get()) {
		m_pImp->m_shared.m_statusSnd->AddMsg2("status\0", 7, pStatusBytes, nStatusLen);
		return true;
	}
	return false;
}

bool LSMComm::SendAlarmUpdate(const char *pAlarmBytes, int nAlarmLen) {
	if (!pAlarmBytes || nAlarmLen <= 0) { return false; }
	if (m_pImp && m_pImp->m_bValid && m_pImp->m_shared.m_alarmSnd.get()) {
		m_pImp->m_shared.m_alarmSnd->AddMsg2("alarm\0", 6, pAlarmBytes, nAlarmLen);
		return true;
	}
	return false;
}

bool LSMComm::SendCommand(const char *pCmdKeyword,
	                      const char *pCmdArgs, int nCmdArgLen,
						  long long cmdID) {
	if (m_pImp && m_pImp->m_bValid && m_pImp->m_shared.m_cmdSnd.get() &&
		Gen_Command(pCmdKeyword, pCmdArgs, nCmdArgLen, cmdID, m_pImp->tVec)) {
		m_pImp->m_shared.m_cmdSnd->AddMsg(&(m_pImp->tVec)[0], (int)m_pImp->tVec.size());
		return true;
	}
	return false;
}

bool LSMComm::UpdateFileDir(int type, const char *pFileDirNames) {
	if (m_pImp && m_pImp->m_bValid && m_pImp->m_shared.m_fileRespSnd.get()) {
		return m_pImp->m_pCore->UpdateFileDir(type, pFileDirNames);
	}
	return false;
}

bool LSMComm::GetNextCmdResp(std::vector<char> &resp, long long *pCmdID) {
	if (m_pImp && m_pImp->m_bValid && m_pImp->m_shared.m_cmdRespRcv.get()) {
		m_pImp->m_shared.m_cmdRespRcv->PopMsg(resp);
		if ((int)resp.size() > (int)sizeof(long long)) {
			*pCmdID = *((long long*)(&resp[0]));
			memmove(&resp[0], &resp[sizeof(long long)], (int)resp.size() - sizeof(long long));
			resp.resize((int)resp.size() - sizeof(long long));
		} else if ((int)resp.size() == sizeof(long long)) {
			*pCmdID = *((long long*)(&resp[0]));
			resp.resize(0);
		} else {
			return false; // unexpected
		}
		return true;
	}
	return false;
}

int LSMComm::GetNumMsg_CmdResp() {
	if (m_pImp && m_pImp->m_bValid && m_pImp->m_shared.m_cmdRespRcv.get()) {
		return m_pImp->m_shared.m_cmdRespRcv->GetNumOfMsg();
	}
	return 0;
}

bool LSMComm::GetNextStatus(std::vector<char> &statusMsg) {
	if (m_pImp && m_pImp->m_bValid && m_pImp->m_shared.m_statusRcv.get()) {
		return m_pImp->m_shared.m_statusRcv->PopMsg(statusMsg);
	}
	return false;
}

int LSMComm::GetNumMsg_Status() {
	if (m_pImp && m_pImp->m_bValid && m_pImp->m_shared.m_statusRcv.get()) {
		return m_pImp->m_shared.m_statusRcv->GetNumOfMsg();
	}
	return 0;
}

bool LSMComm::GetNextAlarm(std::vector<char> &statusMsg) {
	if (m_pImp && m_pImp->m_bValid && m_pImp->m_shared.m_alarmRcv.get()) {
		return m_pImp->m_shared.m_alarmRcv->PopMsg(statusMsg);
	}
	return false;
}

int LSMComm::GetNumMsg_Alarm() {
	if (m_pImp && m_pImp->m_bValid && m_pImp->m_shared.m_alarmRcv.get()) {
		return m_pImp->m_shared.m_alarmRcv->GetNumOfMsg();
	}
	return 0;
}

}}