#include "OneChConnClient.h"

#include <string.h>
//#include <iostream>

namespace DDRCommLib { namespace MARS {

#ifndef I64
typedef long long I64;
#endif

const int OneChConn_Client::RECONNECTION_PERIOD = 5000;
const int OneChConn_Client::CONNECTION_TIMEOUT = 5000;
const int OneChConn_Client::LOGINGIN_TIMEOUT = 5000;

OneChConn_Client::OneChConn_Client()
	: m_rcvQ(new ByteQ), m_sndQ(new ByteQ)
{
	m_sndBufSz = 1024;
	m_rcvBufSz = 1024;
	m_connStat = 1;

	m_serIP_BE = 0;
	m_serPort_BE = 0;

	m_decoder = nullptr;
	m_pDecoderArg = nullptr;

	m_rcvTmp.resize(m_rcvBufSz >> 4);

	m_lastTicConnAttempt = std::chrono::system_clock::now()
		- std::chrono::milliseconds(RECONNECTION_PERIOD);
}

void OneChConn_Client::SetSocketIOBuffer(int sndBufSz, int rcvBufSz)
{
	if (1 == m_connStat || 2 == m_connStat) {
		m_sndBufSz = (sndBufSz < 128 ? 128 : sndBufSz);
		m_rcvBufSz = (rcvBufSz < 128 ? 128 : rcvBufSz);
		m_rcvTmp.resize(m_rcvBufSz >> 4);
	}
}

SocketWrapper* OneChConn_Client::Ptr_Soc()
{
	if (0 == m_connStat || 5 == m_connStat) {
		return &m_soc;
	}
	return nullptr;
}

int OneChConn_Client::GetConnStat() const
{
	return m_connStat;
}

void OneChConn_Client::SetLoginStrPtr(const char *pEncTxtHead, int nEncTxtLen)
{
	if (pEncTxtHead && nEncTxtLen > 0) {
		m_loginStr.resize(nEncTxtLen);
		memcpy(&m_loginStr[0], pEncTxtHead, nEncTxtLen);
	}
}

bool OneChConn_Client::SetSerAddr(U32 tarIP_BE, U16 tarPort_BE)
{
	if (m_connStat > 2) {
		if (tarIP_BE != m_serIP_BE || tarPort_BE != m_serPort_BE) {
			m_soc.Close();
			m_rcvQ->Clear();
			m_sndQ->Clear();
			m_connStat = 2;
			m_serIP_BE = tarIP_BE;
			m_serPort_BE = tarPort_BE;
			return false;
		}
	}
	m_serIP_BE = tarIP_BE;
	m_serPort_BE = tarPort_BE;
	m_connStat = 2;
	return true;
}

void OneChConn_Client::SetState_Exception()
{
	m_soc.Close();
	m_rcvQ->Clear();
	m_sndQ->Clear();
	m_connStat = -1;
}

void OneChConn_Client::SetState_LoggedIn()
{
	if (5 == m_connStat) {
		m_connStat = 0;
		m_sndQ->Clear();
		m_lastTicRcv = std::chrono::system_clock::now();
		m_lastTicSnd = m_lastTicRcv;
	}
}

void OneChConn_Client::SetState_Disconnected()
{
	if (1 != m_connStat) {
		m_soc.Close();
		m_rcvQ->Clear();
		m_sndQ->Clear();
		m_connStat = 2;
	}
}

void OneChConn_Client::SendData(const void *pData, int nDataLen)
{
	if (0 == m_connStat) {
		m_sndLock.lock();
		m_sndQ->Push(pData, nDataLen);
		_send();
		m_sndLock.unlock();
	}
}

bool OneChConn_Client::ReceiveData()
{
	if (0 == m_connStat || 5 == m_connStat) {
		const int nBytesPerAttempt = m_rcvBufSz >> 4;
		SocSet set;
		int ss = 0;
		while (1) {
			set.Clear();
			set.Add2Set(m_soc);
			if (SocSet::Wait(&set, nullptr, nullptr, 0) > 0) {
				int n = m_soc.TCP_Recv(&m_rcvTmp[0], nBytesPerAttempt);
				if (n <= 0) {
					return false;
				}
				m_rcvQ->Push(&m_rcvTmp[0], n);
				ss += n;
				if (n < nBytesPerAttempt) {
					break;
				}
			} else {
				break;
			}
		}
		if (ss > 0) {
			m_lastTicRcv = std::chrono::system_clock::now();
		}
	}
	if (5 == m_connStat) {
		if (0 == GetNextRcvMsg(m_vec2)) {
			m_vec.swap(m_vec2);
		} else {
			m_vec.resize(0);
		}
	}
	return true;
}

bool OneChConn_Client::GetLoginResp(std::vector<char> &outVec)
{
	if (5 == m_connStat && m_vec.size() > 0) {
		outVec.swap(m_vec);
		m_vec.resize(0);
		return true;
	}
	outVec.resize(0);
	return false;
}

void OneChConn_Client::SetDecoder(MsgDecoder pDecoderFunc)
{
	m_decoder = pDecoderFunc;
}

void OneChConn_Client::SetDecoderExtraArg(void *pDecoderExtraArg)
{
	m_pDecoderArg = pDecoderExtraArg;
}

int OneChConn_Client::GetNextRcvMsg(std::vector<char> &vec)
{
	if (!m_decoder) {
		return -1;
	}
	if (0 == m_connStat || 5 == m_connStat) {
		int nRet = m_decoder(m_rcvQ.get(), m_vec, vec, m_pDecoderArg);
		if (0 == nRet) {
			m_lastTicRcv = std::chrono::system_clock::now();
		}
		return nRet;
	}
	return 1;
}

bool OneChConn_Client::GetElapsedTime(int *pTimeSinceLastRcv,
	                                  int *pTimeSinceLastSnd) const
{
	if (0 == m_connStat) {
		auto _tic = std::chrono::system_clock::now();
		*pTimeSinceLastRcv = (int)std::chrono::duration_cast
			<std::chrono::milliseconds>(_tic - m_lastTicRcv).count();
		*pTimeSinceLastSnd = (int)std::chrono::duration_cast
			<std::chrono::milliseconds>(_tic - m_lastTicSnd).count();
		return true;
	}
	return false;
}

bool OneChConn_Client::_createSoc()
{
	if (!m_soc.TCP_Init()) {
		return false;
	}
	if (!m_soc.SetBlocking(false)) {
		return false;
	}
	if (!m_soc.SetBuffer(m_sndBufSz, m_rcvBufSz)) {
		return false;
	}
	return true;
}

bool OneChConn_Client::_connectServer()
{
	auto _tic = std::chrono::system_clock::now();
	I64 timediff = std::chrono::duration_cast
		<std::chrono::milliseconds>(_tic - m_lastTicConnAttempt).count();
	if (timediff < RECONNECTION_PERIOD) {
		return false;
	}
	m_soc.TCP_Connect(m_serIP_BE, m_serPort_BE);
	m_lastTicConnAttempt = _tic;
	m_lastTicRcv = _tic;
	return true;
}

// 0 - successfully connected;
// 1 - still pending;
// -1 - timed-out or disconnected
int OneChConn_Client::_isConnected2Server()
{
	SocSet set;
	set.Add2Set(m_soc);
	if (SocSet::Wait(nullptr, &set, nullptr, 0) == 0) {
		auto _tic = std::chrono::system_clock::now();
		I64 timediff = std::chrono::duration_cast
			<std::chrono::milliseconds>(_tic - m_lastTicRcv).count();
		if (timediff > CONNECTION_TIMEOUT) {
			return -1;
		}
	} else if (m_loginStr.size() > 0) {
		m_soc.TCP_Send(&m_loginStr[0], (int)m_loginStr.size());
		m_lastTicRcv = std::chrono::system_clock::now();
		return 0;
	}
	return 1;
}

int OneChConn_Client::_login2Server()
{
	auto _tic = std::chrono::system_clock::now();
	I64 timediff = std::chrono::duration_cast
		<std::chrono::milliseconds>(_tic - m_lastTicRcv).count();
	if (timediff > LOGINGIN_TIMEOUT) {
		return -1;
	}

	if (!m_decoder) {
		return -1;
	}
	return m_decoder(m_rcvQ.get(), m_vec2, m_vec, m_pDecoderArg);
}

static int __send__(const void *pHead, int nLen, void *pArg)
{
	return ((SocketWrapper*)pArg)->TCP_Send((const char*)pHead, nLen);
}

bool OneChConn_Client::_send()
{
	int nret = m_sndQ->ProcQ(__send__, (void*)(&m_soc));
	if (nret < 0) {
		return false;
	}
	if (nret > 0) {
		m_lastTicSnd = std::chrono::system_clock::now();
	}
	return true;
}

void OneChConn_Client::AllowImmediateConn()
{
	m_lastTicConnAttempt = std::chrono::system_clock::now()
		- std::chrono::milliseconds(RECONNECTION_PERIOD);
}

int OneChConn_Client::Process()
{
	int nRet;

	switch (m_connStat) {
	case 0:
		m_sndLock.lock();
		_send();
		m_sndLock.unlock();
		break;

	case 2:
		if (_createSoc()) {
			m_connStat = 3;
		}
		break;

	case 3:
		if (_connectServer()) {
			m_connStat = 4;
		} else {
			SetState_Disconnected();
		}
		break;

	case 4:
		nRet = _isConnected2Server();
		if (0 == nRet) {
			m_connStat = 5;
		} else if (-1 == nRet) {
			SetState_Disconnected();
		}
		break;

	case 5:
		if (-1 == _login2Server()) {
			SetState_Disconnected();
		}
		break;
	}

	return m_connStat;
}

}}