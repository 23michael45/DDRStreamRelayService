#include "TalkHelper.h"

#include <random>

namespace DDRCommLib { namespace MARS {

static std::random_device s_rd;
static std::mt19937 s_gen(s_rd());
static std::uniform_int_distribution<> s_dis(0, 0xFFFF);
_u16 _genRandomU16() {
	return (_u16)(s_dis(s_gen));
}

TalkHelper::TalkHelper(bool bAlwaysAllowAnswering)
	: m_stat(0), m_bAlwaysAllowAnswering(bAlwaysAllowAnswering),
	m_bOkay2Answer(bAlwaysAllowAnswering),
	m_myID(0), m_peerID(0), m_bSndPending(false),
	m_sndID1(0), m_sndID2(0) {
	m_REQACK_PERIOD = 200;
	m_CONVERSATION_HB_PERIOD = 1000;
	m_MAX_RINGING_TIME = 30000;
}

void TalkHelper::Configure(int maxRingingTime, int reqAckPeriod, int hbPeriod,
	                       bool bAlwaysAllowAnswering) {
	if (maxRingingTime < 5000) {
		maxRingingTime = 5000;
	}
	if (reqAckPeriod < 100) {
		reqAckPeriod = 100;
	}
	if (hbPeriod < reqAckPeriod * 2) {
		hbPeriod = reqAckPeriod * 2;
	}
	m_MAX_RINGING_TIME = maxRingingTime;
	m_REQACK_PERIOD = reqAckPeriod;
	m_CONVERSATION_HB_PERIOD = hbPeriod;
	m_bAlwaysAllowAnswering = bAlwaysAllowAnswering;
	m_bOkay2Answer = bAlwaysAllowAnswering;
}

int TalkHelper::GetState() const {
	return m_stat;
}

bool TalkHelper::Call() {
	if (0 != m_stat) { return false; }
	m_lock.lock();
	bool bRet = false;
	if (0 == m_stat) {
		m_stat = 1; // calling now
		m_myID = _genRandomU16();
		m_lastTicRcv = std::chrono::system_clock::now();
		m_nextTicSnd = std::chrono::system_clock::now();
		m_sndID1 = (((long long)m_myID << 48) | (int)1);
		m_bSndPending = true;
		bRet = true;
	}
	m_lock.unlock();
	return bRet;
}

bool TalkHelper::PickUp() {
	if (!m_bAlwaysAllowAnswering && 3 == m_stat) {
		m_lock.lock();
		m_bOkay2Answer = true;
		m_lock.unlock();
		return true;
	} else { return false; }
}

bool TalkHelper::HangUp() {
	bool bRet = false;
	m_lock.lock();
	if (0 != m_stat) {
		m_sndID1 = 6;
		m_nextTicSnd = std::chrono::system_clock::now();
		m_bSndPending = true;
		bRet = true;
		m_stat = 0;
	}
	m_lock.unlock();
	return bRet;
}

void TalkHelper::Receive(bool bResp, long long ID1, long long ID2) {
	_u16 myID = (_u16)((ID1 >> 32) & 0xFFFF);
	_u16 peerID = (_u16)((ID1 >> 48) & 0xFFFF);
	int peerStat = (int)(ID1 & 0xFFFFFFFF);

	m_lock.lock();
	if (6 == peerStat) { // peer hang up
		m_stat = 0;
		m_bSndPending = false;
		if (!m_bAlwaysAllowAnswering) {
			m_bOkay2Answer = false;
		}
	} else {
		int nSndOption = 0; // 0 - not sending; 1 - immediate sending; 2 - send in next interval
		switch (m_stat) {
		case 0:
			if (!bResp && 1 == peerStat) {
				// peer calling
				m_myID = _genRandomU16();
				m_stat = 3;
				m_lastTicRcv = std::chrono::system_clock::now();
			}
			break;
		case 1:
			if (bResp && myID == m_myID && 4 == peerStat) {
				// peer picked up and its answer pending
				m_stat = 2;
				nSndOption = 1; // generate and send immediately
			}
			break;
		case 2:
			if (bResp && myID == m_myID && 5 == peerStat) {
				// peer answering, conversation maintained
				nSndOption = 2; // generate and send later
			}
			break;
		case 3:
			if (!bResp && m_bOkay2Answer && 1 == peerStat) {
				// peer calling, accept this call
				m_stat = 4;
				nSndOption = 1;
			}
			break;
		case 4:
		case 5:
			//std::cout << "myID=" << myID << ", peerStat=" << peerStat << std::endl;
			if (!bResp && myID == m_myID && 2 == peerStat) {
				// peer acknowledged with right ID, or maintained conversation
				m_stat = 5;
				nSndOption = 2;
			}
			break;
		}
		if (nSndOption > 0) {
			m_lastTicRcv = std::chrono::system_clock::now();
			m_nextTicSnd = m_lastTicRcv;
			if (nSndOption > 1) {
				m_nextTicSnd += std::chrono::milliseconds(m_REQACK_PERIOD);
			}
			m_peerID = peerID;
			m_sndID1 = (((long long)m_myID << 48) | ((long long)m_peerID << 32) | (int)m_stat);
			m_bSndPending = true;
		}
	}

	m_lock.unlock();
}

bool TalkHelper::IsOkay2Send(bool *pbResp, long long *pID1, long long *pID2) {
	bool bRet = false;
	m_lock.lock();
	if (m_bSndPending) {
		auto _tic = std::chrono::system_clock::now();
		if (_tic >= m_nextTicSnd) {
			*pbResp = (1 != m_stat && 2 != m_stat);
			*pID1 = m_sndID1;
			*pID2 = m_sndID2;
			if (1 == m_stat) {
				m_nextTicSnd = _tic + std::chrono::milliseconds(m_REQACK_PERIOD);
			} else {
				m_bSndPending = false;
			}
			bRet = true;
		}
	}
	m_lock.unlock();
	return bRet;
}

void TalkHelper::Update() {
	if (0 == m_stat) { return; }
	m_lock.lock();
	int tdMax = (1 == m_stat || 3 == m_stat) ? m_MAX_RINGING_TIME : m_CONVERSATION_HB_PERIOD;
	auto _tic = std::chrono::system_clock::now();
	I64 timediff = std::chrono::duration_cast
		<std::chrono::milliseconds>(_tic - m_lastTicRcv).count();
	if (timediff > tdMax) {
		// stat 1/3, peer/me not picking up
		// stat 2/4/5, conversation not maintained
		m_stat = 0;
		if (!m_bAlwaysAllowAnswering) {
			m_bOkay2Answer = false;
		}
		m_sndID1 = 6;
		m_nextTicSnd = std::chrono::system_clock::now();
		m_bSndPending = true;
	}
	m_lock.unlock();
}

}}