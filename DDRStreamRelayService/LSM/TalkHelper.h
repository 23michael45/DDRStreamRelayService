#ifndef __DDRCOMMLIB_MARS_TALKING_HELPER_H_INCLUDED__
#define __DDRCOMMLIB_MARS_TALKING_HELPER_H_INCLUDED__

#include <mutex>
#include <chrono>
#include <atomic>

namespace DDRCommLib {
	
#ifndef _u16
typedef unsigned int _u16;
#endif
#ifndef I64
typedef long long I64;
#endif

namespace MARS {

class TalkHelper {
public:
	TalkHelper(bool bAlwaysAllowAnswering = false);
	// all in milliseconds
	void Configure(int maxRingingTime, int reqAckPeriod, int hbPeriod,
		           bool bAlwaysAllowAnswering);

	// 0 - silent;
	// 1 - calling
	// 2 - talking
	// 3 - being called to answer
	// 4 - pending answer
	// 5 - answering
	int GetState() const;
	
	// *************************** USER INTERFACE *************************** //
	bool Call();
	bool PickUp();
	bool HangUp();
	// *************************** USER INTERFACE *************************** //

	// ******************** INTERNAL WORKING THREAD API ********************* //
	void Receive(bool bResp, long long ID1, long long ID2);
	bool IsOkay2Send(bool *pbResp, long long *pID1, long long *pID2);
	void Update();
	// ******************** INTERNAL WORKING THREAD API ********************* //

protected:
	int m_REQACK_PERIOD;
	int m_MAX_RINGING_TIME;
	int m_CONVERSATION_HB_PERIOD;

	std::atomic<int> m_stat;
	std::mutex m_lock;
	bool m_bAlwaysAllowAnswering;
	bool m_bOkay2Answer;
	_u16 m_myID;
	_u16 m_peerID;
	bool m_bSndPending;
	long long m_sndID1;
	long long m_sndID2;
	std::chrono::system_clock::time_point m_lastTicRcv;
	std::chrono::system_clock::time_point m_nextTicSnd;
};

}}

#endif // __DDRCOMMLIB_MARS_TALKING_HELPER_H_INCLUDED__