#ifndef __DDRCOMMLIB_CLIENT_ONECHANNEL_STATEFULL_CONNECTION_H_INCLUDED__
#define __DDRCOMMLIB_CLIENT_ONECHANNEL_STATEFULL_CONNECTION_H_INCLUDED__

#include <vector>
#include <memory>
#include <mutex>
#include <chrono>
#include "CommLib/MessageQueue.h"
#include "CommLib/SocketWrapper.h"

namespace DDRCommLib { namespace MARS {

class OneChConn_Client {
public:
	OneChConn_Client();
	void SetSocketIOBuffer(int sndBufSz, int rcvBufSz);
	SocketWrapper* Ptr_Soc();

	// stat:
	// 0, logged in, IO ready
	// -1, exception or user define
	// 1, not configured;
	// 2, tarAddr configured, socket not created;
	// 3, socket created, not connecting;
	// 4, connecting
	// 5, connected, logging in
	int GetConnStat() const;

	void SetLoginStrPtr(const char *pEncTxtHead, int nEncTxtLen);

	// return FALSE only if address changes
	bool SetSerAddr(U32 tarIP_BE, U16 tarPort_BE);

	// process status change and
	// return current conStat
	int Process();
	void AllowImmediateConn();

	void SetState_Exception();
	void SetState_LoggedIn();
	void SetState_Disconnected();

	void SendData(const void *pData, int nDataLen);
	bool ReceiveData();
	bool GetLoginResp(std::vector<char> &outVec);
	
	// decode a message from a round queue
	// possibly need tmpVec to store intermediate results
	// dstVec is the output vector (can be empty)
	// return 0 for success; 1 for bytes pending (insufficient);
	// -1 for ILLEGAL bytes
	typedef int(*MsgDecoder)(ByteQ *pCAQ,
		                     std::vector<char> &tmpVec,
							 std::vector<char> &dstVec,
							 void *pArg);
	void SetDecoder(MsgDecoder pDecoderFunc);
	void SetDecoderExtraArg(void *pDecoderExtraArg);

	// USING DECODER FUNCTION TO FETCH ONE SINGLE MESSAGE FROM THE RECEIVED QUEUES
	// Return 0 if a string (including an empty string) can be decoded
	// Return 1 if insufficient for a complete string
	// Return -1 for illegal bytes
	int GetNextRcvMessage(std::vector<char> &vec);

	// 0 - successful
	// 1 - pending
	// 2 - illegal
	int GetNextRcvMsg(std::vector<char> &vec);
	// time in milliseconds
	bool GetElapsedTime(int *pTimeSinceLastRcv, int *pTimeSinceLastSnd) const;

protected:
	int m_sndBufSz;
	int m_rcvBufSz;
	MsgDecoder m_decoder;
	void *m_pDecoderArg;
	std::vector<char> m_rcvTmp;

	std::vector<char> m_loginStr;
	U32 m_serIP_BE;
	U16 m_serPort_BE;
	std::vector<char> m_vec, m_vec2;

	int m_connStat;
	SocketWrapper m_soc;
	std::chrono::system_clock::time_point m_lastTicConnAttempt;
	std::chrono::system_clock::time_point m_lastTicRcv;
	std::chrono::system_clock::time_point m_lastTicSnd;

	std::shared_ptr<ByteQ> m_rcvQ;
	std::timed_mutex m_sndLock;
	std::shared_ptr<ByteQ> m_sndQ;

	static const int RECONNECTION_PERIOD;
	static const int CONNECTION_TIMEOUT;
	static const int EXTRA_BYTES_FOR_ENCRYPTION;
	static const int LOGINGIN_TIMEOUT;
	//static const timeval s_ztv;

	bool _createSoc();
	bool _connectServer();
	// 0 - successfully connected;
	// 1 - still pending;
	// -1 - timed-out or disconnected
	int _isConnected2Server();
	// 0 - login response alleged
	// 1 - still pending;
	// -1 - timed-out or disconnected
	int _login2Server();

	// return if the connection is still ON
	bool _send();
};

}}

#endif // __DDRCOMMLIB_CLIENT_ONECHANNEL_STATEFULL_CONNECTION_H_INCLUDED__
