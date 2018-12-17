#ifndef __DDRCOMMLIB_DUAL_CHANNEL_TCP_CLIENT_BASE_H_INCLUDED__
#define __DDRCOMMLIB_DUAL_CHANNEL_TCP_CLIENT_BASE_H_INCLUDED__

#include <vector>
#include "OneChConnClient.h"

#ifndef U32
typedef unsigned int U32;
#endif
#ifndef U16
typedef unsigned short U16;
#endif

namespace DDRCommLib { namespace MARS {

class DualChTCPClientBase {
public:
	DualChTCPClientBase();
	void SetBufferSz(int mainSndBufSz, int mainRcvBufSz,
		             int subSndBufSz, int subRcvBufSz);

	// time in milliseconds
	void SetConnIOPeriod(int nMaxSilentTimeSND, int nMaxSilentTimeRcv);

	void SetServerAddr(U32 tarIP_BE, U16 tarPort_BE);

	typedef int(*MsgDecoder)(ByteQ *pCAQ,
		                     std::vector<char> &tmpVec,
							 std::vector<char> &dstVec,
							 void *pArg);
	inline void SetDecoder(MsgDecoder pDecoderFunc) {
		m_main.SetDecoder(pDecoderFunc);
		m_sub.SetDecoder(pDecoderFunc);
	}
	inline void SetDecoderExtraArg(void *pDecoderExtraArg) {
		m_main.SetDecoderExtraArg(pDecoderExtraArg);
		m_sub.SetDecoderExtraArg(pDecoderExtraArg);
	}

	// stat:
	// 0, logged in, IO ready
	// -1, exception or user define
	// 1, not configured;
	// 2, tarAddr configured, socket not created;
	// 3, socket created, not connecting;
	// 4, connecting
	// 5, connected, logging in
	void GetConnStat(int *pMainStat, int *pSubStat);

	// return FALSE only if m_main is in EXCEPTION state
	bool ProcessOnce(int ioTimeOutMs);

	bool Send(const char *pStrHead, int nStrLen, bool bMainChannel);

protected:
	bool m_bStarted;
	unsigned int m_UID;
	OneChConn_Client m_main;
	OneChConn_Client m_sub;
	int m_procCnt;

	int m_sndPeriod, m_rcvPeriod;

	std::vector<char> m_tmpVec;

	virtual bool genMainLoginStr(std::vector<char> &vec) = 0;
	virtual bool genSubLoginStr(unsigned int uid, std::vector<char> &vec) = 0;

	// 0 - login successful; -1 - login exception (INCORRECT PASSWORD, etc);
	// 1 - other login error
	virtual int parseMainLoginResp(const char *pRespHead, int nRespLen, unsigned int &newUID) = 0;

	// 0 - okay; 1 - disconnection due to illegal strings;
	// -1 - server side notification of exception (INCORRECT PASSWORD, etc)
	virtual int processRcv(const std::vector<char> &strVec, bool bFromMain) = 0;
};

}}

#endif // __DDRCOMMLIB_DUAL_CHANNEL_TCP_CLIENT_BASE_H_INCLUDED__
