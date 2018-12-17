#include "LSMComp.h"

#include <string.h>
#include <iostream>
#include "CommLib/CommonFunc.h"

namespace DDRCommLib { namespace MARS {
	
void SafeMsgQueue::Send(OneChConn_Client *pOneCh, std::vector<char> &vec1, std::vector<char> &vec2) {
	locker.lock();
	while (1) {
		int nlen = queue->GetNextMsgLen();
		if (nlen <= 0) { break; }
		vec1.resize(nlen);
		if (queue->Pop(&vec1[0], nlen) <= 0) {
			break; // unexpected
		}
		PackVecs(vec1, vec2);
		pOneCh->SendData(&vec2[0], (int)vec2.size());
	}
	locker.unlock();
}

void SafeMsgQueue::Send(DualChTCPClientBase *pDualCh, std::vector<char> &vec1, std::vector<char> &vec2) {
	locker.lock();
	while (1) {
		int nlen = queue->GetNextMsgLen();
		if (nlen <= 0) { break; }
		vec1.resize(nlen);
		if (queue->Pop(&vec1[0], nlen) <= 0) {
			break; // unexpected
		}
		PackVecs(vec1, vec2);
		pDualCh->Send(&vec2[0], (int)vec2.size(), (vec2.size() <= 4096));
	}
	locker.unlock();
}

void SafeMsgQueue::AddMsg(const char *pHead, int nLen) {
	locker.lock();
	queue->Push(pHead, nLen);
	locker.unlock();
}

void SafeMsgQueue::AddMsg2(const char *pHead1, int nLen1, const char *pHead2, int nLen2) {
	locker.lock();
	queue->Push2(pHead1, nLen1, pHead2, nLen2);
	locker.unlock();
}

bool SafeMsgQueue::PopMsg(std::vector<char> &vec) {
	locker.lock();
	int nlen = queue->GetNextMsgLen();
	if (nlen > 0) {
		vec.resize(nlen);
		nlen = queue->Pop(&vec[0], nlen);
	}
	locker.unlock();
	return (nlen > 0);
}

int SafeMsgQueue::GetNumOfMsg() {
	locker.lock();
	int nret = queue->GetNumMsg();
	locker.unlock();
	return nret;
}

int SafeMsgQueue::GetNumBytes() {
	locker.lock();
	int nret = queue->GetNumBytes();
	locker.unlock();
	return nret;
}

SharedInfo::SharedInfo(const char *pModuleName, const char *pLSName)
{
	m_bValid = false;
	m_PushChs = nullptr;
	m_pPullCh = nullptr;
	if ((!pModuleName || '\0' == *pModuleName) ||
		(!pLSName || '\0' == *pLSName)) {
		return;
	}
	m_moduleName = pModuleName;
	m_localServerName = pLSName;
	m_PushChs = nullptr;
	m_pPullCh = nullptr;

	m_peerTalkerType = 0;
	m_connStat = 1;

	m_bValid = true;
}

bool SharedInfo::EnableAVStreamHandling(bool bAlwaysAnswerCalls)
{
	if (!m_bValid) {
		return false;
	}
	if (m_PushChs) {
		delete[] m_PushChs;
	}
	m_PushChs = new _oneAVChannel2Push[MAX_AUDIOVIDEO_CHANNELS];
	if (!m_PushChs) {
		return false;
	}
	for (int i = 0; i < MAX_AUDIOVIDEO_CHANNELS; ++i) {
		m_PushChs[i].avType = 0;
		m_PushChs[i].devType = -1;
		m_PushChs[i].localAccStr = std::string();
		m_PushChs[i].remoteAddress = std::string();
	}

	if (m_pPullCh) {
		delete m_pPullCh;
	}
	m_pPullCh = new _oneAVChannel2Pull;
	if (!m_pPullCh) {
		return false;
	}
	m_pPullCh->avType = 0;
	m_pPullCh->accStr = std::string();

	m_pTalkHelper = std::shared_ptr<TalkHelper>(new TalkHelper(bAlwaysAnswerCalls));
	m_pTalkHelper->Configure(CALLING_MAX_RINGING_TIME,
		                     CALLING_REQ_ACK_PERIOD,
							 CALLING_MAX_INACTIVE_TIME,
							 bAlwaysAnswerCalls);
	return true;
}

SharedInfo::~SharedInfo() {
	if (m_PushChs) {
		delete[] m_PushChs;
	}
	if (m_pPullCh) {
		delete m_pPullCh;
	}
}

bool SharedInfo::IsValid() const { return m_bValid; }

LSMComp::LSMComp(SharedInfo *pShared) {
	m_bValid = false;
	if (!pShared || !pShared->IsValid()) { return; }
	U32 ip;
	ConvertStr2UIP_IPv4(DDRLNB_NODEINFO_BROADCASTING_ADDRESS_IP, &ip);
	unsigned short port = ConvertHTONS(DDRLNB_NODEINFO_BROADCASTING_ADDRESS_PORT);
	m_pNodeHelper = std::shared_ptr<NodeBCRcvHelper>(new NodeBCRcvHelper(pShared->m_localServerName.c_str(),
		(char)0x88, (char)0x3B, ip, port));
	m_pFileSrc = std::shared_ptr<FileSource>(new FileSource("."));
	m_pShared = pShared;
	m_serIP = 0;
	m_serPort = 0;
	m_UID = 0;
	m_prCnt = 0;
	SetBufferSz(4096, 4096, 65536, 65536);
	SetConnIOPeriod(HB_SENDING_PERIOD, MAX_TIME_FOR_INACTIVE_CONNECTION);
	SetDecoder(DecodeNextMessage);
	m_bValid = true;
}

bool LSMComp::IsValid() const {
	return m_bValid;
}

void LSMComp::Process() {
	if (!m_bValid) { return; }

	if (_isServerAddrChanged()) {
		m_main.SetSerAddr(m_serIP, m_serPort);
		m_sub.SetSerAddr(m_serIP, m_serPort);
	}

	ProcessOnce(10);
	if (0 == (m_pShared->m_connStat = m_main.GetConnStat())) {
		_forwardSndQueue();
		_sendTalks();
	}

	if (++m_prCnt % 10000 == 0) {
		m_tVec.clear();
		m_tVec2.clear();
	}
}

bool LSMComp::UpdateFileDir(int type, const char *pFileDirNames) {
	if (!m_bValid || !m_pFileSrc.get()) { return false; }
	if (type < 0 || type > 2) { return false; }

	std::vector<char> tmpVec, vec2;
	if (!FileCache::FormOneSentence(tmpVec, FC_INQ, (FCFunction)type,
		                            pFileDirNames, strlen(pFileDirNames))) {
		return false;
	}
	int nRet = m_pFileSrc->Respond2Sentence(&tmpVec[0], (int)tmpVec.size(), vec2);
	if (0 == nRet) {
		tmpVec.resize(5 + sizeof(U32) + sizeof(U16));
		memcpy(&tmpVec[0], "file\0", 5);
		*((U32*)(&tmpVec[5])) = 0;
		*((U16*)(&tmpVec[5 + sizeof(U32)])) = 0;
		m_pShared->m_fileRespSnd->AddMsg2(&tmpVec[0], (int)tmpVec.size(),
			                              &vec2[0], (int)vec2.size());
		return true;
	}
	return false;
}

void LSMComp::_sendTalks() {
	if (!m_pShared->m_pPullCh ||
		!m_pShared->m_PushChs ||
		!m_pShared->m_pTalkHelper) {
		return;
	}
	m_pShared->m_pTalkHelper->Update();
	if (0 == m_pShared->m_pTalkHelper->GetState()) {
		m_pShared->m_peerTalkerType = 0;
	}
	bool bResp;
	long long id1, id2;
	bool bSth2Send = false;
	if (m_pShared->m_pTalkHelper->IsOkay2Send(&bResp, &id1, &id2)) {
		if (bResp) {
			if (1 == m_pShared->m_peerTalkerType) {
				if (Gen_MTalkResp(id1, id2, m_tVec)) {
					bSth2Send = true;
				}
			}
			else if (2 == m_pShared->m_peerTalkerType) {
				if (Gen_CTalkResp(id1, id2, m_tVec)) {
					bSth2Send = true;
				}
			}
		} else {
			bSth2Send = Gen_RTalk(id1, id2, m_tVec);
		}
	}
	if (bSth2Send) {
		PackVecs(m_tVec, m_tVec2);
		m_main.SendData(&m_tVec2[0], (int)m_tVec2.size());
	}
}

void LSMComp::_groupChsInfoAlloc() {
	if (!m_pShared->m_pPullCh || !m_pShared->m_PushChs) {
		return;
	}

	m_pShared->m_chLock.lock();

	std::string &_str = m_pShared->m_pPullCh->accStr;
	if (1 == m_pShared->m_peerTalkerType && m_resAlloc.bAllocated) {
		m_pShared->m_pPullCh->avType = m_resInfo.avType;
		_str = "rtmp://";
		char tmpStr[20];
		ConvertUIP2Str_IPv4(m_resAlloc.uip_BE, tmpStr);
		_str += tmpStr;
		_str += ":";
		ConvertInt2Str(ConvertHTONS(m_resAlloc.port_BE), tmpStr);
		_str += tmpStr;
		_str += "/live/";
		_str += m_resAlloc.avpass;
	} else if (2 == m_pShared->m_peerTalkerType) { // local client calling
		m_pShared->m_pPullCh->avType = m_resInfo.avType;
		_str = m_resInfo.accessStr;
	} else {
		_str = std::string();
	}

	for (int i = 0; i < MAX_AUDIOVIDEO_CHANNELS; ++i) {
		if (m_avInfo[i].avType >= 1 &&
			m_avInfo[i].avType <= 3) {
			m_pShared->m_PushChs[i].avType = m_avInfo[i].avType;
			m_pShared->m_PushChs[i].devType = m_avInfo[i].devType;
			m_pShared->m_PushChs[i].localAccStr = m_avInfo[i].accessStr;
			std::string &str = m_pShared->m_PushChs[i].remoteAddress;
			if (m_avAlloc[i].bAllocated) {
				str = "rtmp://";
				char tmpStr[20];
				ConvertUIP2Str_IPv4(m_avAlloc[i].uip_BE, tmpStr);
				str += tmpStr;
				str += ":";
				ConvertInt2Str(ConvertHTONS(m_avAlloc[i].port_BE), tmpStr);
				str += tmpStr;
				str += "/live/";
				str += m_avAlloc[i].avpass;
			} else {
				str = std::string();
			}
		} else {
			m_pShared->m_PushChs[i].avType = 0;
			m_pShared->m_PushChs[i].devType = -1;
			m_pShared->m_PushChs[i].localAccStr = std::string();
			m_pShared->m_PushChs[i].remoteAddress = std::string();
		}
	}

	m_pShared->m_chLock.unlock();
}

bool LSMComp::genMainLoginStr(std::vector<char> &vec) {
	if (Gen_LSMLogin(m_pShared->m_moduleName.c_str(), m_UID, m_tVec)) {
		PackVecs(m_tVec, vec);
		return true;
	} else { return false; }
}

bool LSMComp::genSubLoginStr(unsigned int uid, std::vector<char> &vec) {
	const char *pC = m_pShared->m_localServerName.c_str();
	for (; '\0' != *pC && '_' != *pC; ++pC);
	std::string rname = std::string(m_pShared->m_localServerName.c_str(), pC);
	if (Gen_SubChLogin(rname.c_str(), "WHATEVER", m_UID, m_tVec)) {
		PackVecs(m_tVec, vec);
		return true;
	} else { return false; }
}

// 0 - login successful; -1 - login exception (INCORRECT PASSWORD, etc);
// 1 - other login error
int LSMComp::parseMainLoginResp(const char *pRespHead, int nRespLen, unsigned int &newUID) {
	int retCode, role, nRIDLen;
	const char *pRID;
	if (0 != strcmp(pRespHead, "login")) {
		return 1;
	}
	if (!Parse_LoginResp(pRespHead + 6, nRespLen - 6, &retCode, &role, &pRID, &nRIDLen, &newUID)) {
		return 1;
	}
	if (0 == retCode && 3 == role) {
		return 0;
	}
	else if (5 != retCode) { return 1; }
	else { return -1; }
}

// 0 - okay; 1 - disconnection due to illegal strings;
// -1 - server side notification of exception (INCORRECT PASSWORD, etc)
int LSMComp::processRcv(const std::vector<char> &strVec, bool bFromMain) {
	int nStrOffset;
	auto type = AnalyzeInfoStr(strVec, &nStrOffset);
	switch (type) {
	case ILLEGAL:
		return 1;
		break;
		
	case EMPTY:
		break;

	case LOGIN: // login-resp ignored
		break;

	case NOTICE:
		if (1) {
			bool bSROnline;
			int nOpMode, nMons, nClts, talkAVType, talkPeer;
			long long id1, id2;
			if (Parse_LS2LSM_ACCPU(&strVec[nStrOffset],
				                   (int)strVec.size() - nStrOffset,
								   &bSROnline, &nOpMode,
								   &nMons, &nClts)) {
				m_pShared->m_bRSConnected = bSROnline;
				m_pShared->m_nOpMode = nOpMode;
				m_pShared->m_nMonitors = nMons;
				m_pShared->m_nClients = nClts;
			}
			else if (m_pShared->m_pTalkHelper &&
				     Parse_LS2LSM_AVPU(&strVec[nStrOffset],
					                   (int)strVec.size() - nStrOffset,
									   &m_resAlloc, &m_resInfo,
									   m_avInfo, m_avAlloc)) {
				_groupChsInfoAlloc();
			}
			else if (m_pShared->m_pTalkHelper &&
				     Parse_MTalk(&strVec[nStrOffset],
				                 (int)strVec.size() - nStrOffset,
								 &id1, &id2, &talkAVType)) {
				m_pShared->m_peerTalkerType = 1; // monitor
				m_resInfo.avType = talkAVType;
				m_pShared->m_pTalkHelper->Receive(false, id1, id2);
			}
			else if (m_pShared->m_pTalkHelper &&
				     Parse_CTalk(&strVec[nStrOffset],
					             (int)strVec.size() - nStrOffset,
								 &id1, &id2, &talkAVType)) {
				m_pShared->m_peerTalkerType = 2; // client
				m_resInfo.avType = talkAVType;
				m_pShared->m_pTalkHelper->Receive(false, id1, id2);
			}
			else if (m_pShared->m_pTalkHelper &&
				     Parse_RTalkResp(&strVec[nStrOffset],
					                 (int)strVec.size() - nStrOffset,
									 &id1, &id2, &talkAVType, &talkPeer)) {
				m_pShared->m_peerTalkerType = talkPeer;
				m_resInfo.avType = talkAVType;
				m_pShared->m_pTalkHelper->Receive(true, id1, id2);
			}
		}
		break;

	case STATUS: // "status..." into statusRcv
		if (m_pShared->m_statusRcv.get()) {
			m_pShared->m_statusRcv->AddMsg(&strVec[nStrOffset],
				                           (int)strVec.size() - nStrOffset);
		}
		break;

	case FILE: // "file..." into fileInqRcv
		if (m_pShared->m_fileRespSnd.get() &&
			(int)strVec.size() - nStrOffset > (int)sizeof(U32) +
			(int)sizeof(U16)) {
			if (m_pFileSrc->Respond2Sentence(&strVec[nStrOffset + sizeof(U32) + sizeof(U16)],
				                             (int)strVec.size() - nStrOffset - sizeof(U32) - sizeof(U16),
											 m_tVec2) == 0) {
				char X[5 + sizeof(U32) + sizeof(U16)];
				memcpy(X, "file\0", 5);
				memcpy(X + 5, &strVec[nStrOffset], sizeof(U32) + sizeof(U16));
				m_pShared->m_fileRespSnd->AddMsg2(X, sizeof(X), &m_tVec2[0], (int)m_tVec2.size());
			}
		}
		break;

	case ALARM: // "alarm..." into alarmRcv
		if (m_pShared->m_alarmRcv.get()) {
			m_pShared->m_alarmRcv->AddMsg(&strVec[nStrOffset],
				                          (int)strVec.size() - nStrOffset);
		}
		break;

	case CHATTING: // "chat..." ignored
		break;

	case COMMAND: // "cmdO..." into cmdRcv
		if (m_pShared->m_cmdRcv.get()) {
			m_pShared->m_cmdRcv->AddMsg(&strVec[0], (int)strVec.size());
		}
		break;

	case CMD_RESP: // "cmdR..." into cmdRespRcv
		if (m_pShared->m_cmdRespRcv.get()) {
			long long cmdID;
			const char *pRespContent;
			int nRespLen;
			if (Parse_CmdResp(&strVec[0], (int)strVec.size(),
				              &cmdID, &pRespContent, &nRespLen)) {
				if (nRespLen > 0) {
					m_pShared->m_cmdRespRcv->AddMsg2((const char*)(&cmdID), sizeof(long long),
						                             pRespContent, nRespLen);
				} else {
					m_pShared->m_cmdRespRcv->AddMsg((const char*)(&cmdID), sizeof(long long));
				}
			}
		}
		break;

	} // end switch(type)

	return 0;
}

void LSMComp::_forwardSndQueue() {
	// cmdSnd queue
	if (m_pShared->m_cmdSnd.get()) {
		m_pShared->m_cmdSnd->Send(this, m_tVec, m_tVec2);
	}
	// cmdRespSnd queue
	if (m_pShared->m_cmdRespSnd.get()) {
		m_pShared->m_cmdRespSnd->Send(this, m_tVec, m_tVec2);
	}
	// alarmSnd queue
	if (m_pShared->m_alarmSnd.get()) {
		m_pShared->m_alarmSnd->Send(this, m_tVec, m_tVec2);
	}
	// statusSnd queue
	if (m_pShared->m_statusSnd.get()) {
		m_pShared->m_statusSnd->Send(this, m_tVec, m_tVec2);
	}
	// fileRespSnd queue
	if (m_pShared->m_fileRespSnd.get()) {
		m_pShared->m_fileRespSnd->Send(&m_sub, m_tVec, m_tVec2);
	}
}

bool LSMComp::_isServerAddrChanged() {
	if (!m_pNodeHelper) { return false; }
	if (0 == m_main.GetConnStat()) {
		return false;
	}
	U32 sip;
	U16 sPort;
	if (m_pNodeHelper->Try2FindNode(&sip, &sPort)) {
		if (m_serIP != sip || m_serPort != sPort) {
			m_serIP = sip;
			m_serPort = sPort;
			return true;
		}
	}
	return false;
}

}}