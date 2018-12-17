#include "DualChTCPClientBase.h"

//#include <iostream>
namespace DDRCommLib { namespace MARS {

DualChTCPClientBase::DualChTCPClientBase()
	: m_bStarted(false), m_UID(0), m_procCnt(0) {}

void DualChTCPClientBase::SetBufferSz(int mainSndBufSz, int mainRcvBufSz,
	                                  int subSndBufSz, int subRcvBufSz) {
	if (!m_bStarted) {
		m_main.SetSocketIOBuffer(mainSndBufSz, mainSndBufSz);
		m_sub.SetSocketIOBuffer(subSndBufSz, subRcvBufSz);
	}
}

void DualChTCPClientBase::SetConnIOPeriod(int nMaxSilentTimeSND, int nMaxSilentTimeRcv) {
	if (nMaxSilentTimeSND < 1000) {
		nMaxSilentTimeSND = 1000;
	}
	if (nMaxSilentTimeRcv < 1000) {
		nMaxSilentTimeRcv = 1000;
	}
	m_sndPeriod = nMaxSilentTimeSND;
	m_rcvPeriod = nMaxSilentTimeRcv;
}

void DualChTCPClientBase::SetServerAddr(U32 tarIP_BE, U16 tarPort_BE) {
	m_sub.SetSerAddr(tarIP_BE, tarPort_BE);
	if (!m_main.SetSerAddr(tarIP_BE, tarPort_BE)) {
		m_sub.SetState_Disconnected();
	}
}

void DualChTCPClientBase::GetConnStat(int *pMainStat, int *pSubStat) {
	if (pMainStat) {
		*pMainStat = m_main.GetConnStat();
	}
	if (pSubStat) {
		*pSubStat = m_sub.GetConnStat();
	}
}

bool DualChTCPClientBase::ProcessOnce(int ioTimeOutMs) {
	m_bStarted = true;
	if (++m_procCnt % 10000 == 0) {
		m_tmpVec.clear();
	}

	int nMainStat0 = m_main.GetConnStat();
	int nMainStat1 = m_main.Process();
	if (-1 == nMainStat1) {
		if (-1 != nMainStat0) {
			m_sub.SetState_Disconnected();
		}
		return false;
	}
	if (nMainStat0 != nMainStat1 &&
		4 == nMainStat1) {
		std::vector<char> vec;
		if (genMainLoginStr(vec) && vec.size() > 0) {
			m_main.SetLoginStrPtr(&vec[0], (int)vec.size());
		} else {
			m_main.SetState_Disconnected();
			m_sub.SetState_Disconnected();
		}
		return true;
	}
	if (0 != nMainStat1 && 5 != nMainStat1) {
		return true;
	}

	if (ioTimeOutMs < 0) {
		ioTimeOutMs = 0;
	}

	SocSet set;
	set.Add2Set(m_main.Ptr_Soc());
	if (SocSet::Wait(&set, nullptr, nullptr, ioTimeOutMs) > 0) {
		if (!m_main.ReceiveData()) {
			m_main.SetState_Disconnected();
			m_sub.SetState_Disconnected();
			return true;
		}
	}

	if (5 == m_main.GetConnStat()) {
		if (m_main.GetLoginResp(m_tmpVec) && m_tmpVec.size() > 0) {
 			int nRet = parseMainLoginResp(&m_tmpVec[0], (int)m_tmpVec.size(), m_UID);
			if (0 == nRet) {
				m_main.SetState_LoggedIn();
			} else if (1 == nRet) {
				m_UID = 0;
				m_main.SetState_Disconnected();
				m_sub.SetState_Disconnected();
			} else {
				m_main.SetState_Exception();
				m_sub.SetState_Exception();
				return false;
			}
		}
		return true;
	}

	while (1) {
		m_tmpVec.resize(0);
		int nRet = m_main.GetNextRcvMsg(m_tmpVec);
		if (-1 == nRet) {
			m_main.SetState_Disconnected();
			m_sub.SetState_Disconnected();
			return true;
		} else if (1 == nRet) {
			break;
		}
		nRet = processRcv(m_tmpVec, true);
		if (-1 == nRet) {
			m_main.SetState_Exception();
			m_sub.SetState_Exception();
			return false;
		} else if (1 == nRet) {
			m_main.SetState_Disconnected();
			m_sub.SetState_Disconnected();
			return true;
		}
	}
	int tSnd = 0, tRcv = 0;
	m_main.GetElapsedTime(&tRcv, &tSnd);
	if (tRcv > m_rcvPeriod) {
		m_main.SetState_Disconnected();
		m_sub.SetState_Disconnected();
		return true;
	}
	if (tSnd > m_sndPeriod) {
		char emptyStr[8] = { (char)0x00, (char)0x00, (char)0x00, 0x00, (char)0xFF, (char)0xFF, (char)0xFF, (char)0xFF };
		m_main.SendData(emptyStr, 8);
	}
	
	int nSubStat0 = m_sub.GetConnStat();
	int nSubStat1 = m_sub.Process();
	if (nSubStat0 != nSubStat1 && 4 == nSubStat1) {
		std::vector<char> vec;
		if (genSubLoginStr(m_UID, vec) && vec.size() > 0) {
			m_sub.SetLoginStrPtr(&vec[0], (int)vec.size());
		} else {
			m_sub.SetState_Disconnected();
		}
		return true;
	}
	if (0 != nSubStat1 && 5 != nSubStat1) {
		return true;
	}
	if (!m_sub.ReceiveData()) {
		m_sub.SetState_Disconnected();
		return true;
	}
	
	if (5 == m_sub.GetConnStat()) {
		m_sub.SetState_LoggedIn();
		return true;
	}

	while (1) {
		int nRet = m_sub.GetNextRcvMsg(m_tmpVec);
		if (-1 == nRet) {
			m_sub.SetState_Disconnected();
			return true;
		} else if (1 == nRet) {
			break;
		}
		nRet = processRcv(m_tmpVec, false);
		if (-1 == nRet) {
			m_sub.SetState_Exception();
			return false;
		}
		else if (1 == nRet) {
			m_sub.SetState_Disconnected();
			return true;
		}
	}
	tSnd = 0;
	tRcv = 0;
	m_sub.GetElapsedTime(&tRcv, &tSnd);
	if (tRcv > m_rcvPeriod) {
		m_sub.SetState_Disconnected();
		return true;
	}
	if (tSnd > m_sndPeriod) {
		char emptyStr[8] = { (char)0x00, (char)0x00, (char)0x00, 0x00, (char)0xFF, (char)0xFF, (char)0xFF, (char)0xFF };
		m_sub.SendData(emptyStr, 8);
	}

	return true;
}

bool DualChTCPClientBase::Send(const char *pStrHead, int nStrLen, bool bMainChannel) {
	if (!pStrHead || nStrLen <= 0) {
		return false;
	}
	if (bMainChannel) {
		if (0 == m_main.GetConnStat()) {
			m_main.SendData(pStrHead, nStrLen);
			return true;
		}
	} else {
		if (0 == m_sub.GetConnStat()) {
			m_sub.SendData(pStrHead, nStrLen);
			return true;
		}
	}
	return false;
}


}}