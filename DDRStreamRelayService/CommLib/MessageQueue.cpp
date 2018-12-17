#include "MessageQueue.h"

#include "MsgStruct.h"

namespace DDRCommLib {

static const int g_SZAdjustPeriod = 50;

ByteQ::ByteQ()
{
	m_p = Create_PlainQ();
	m_nSZCnt = m_nSZSum = 0;
}

ByteQ::~ByteQ()
{
	Destroy_PlainQ(m_p);
}

void ByteQ::Clear()
{
	Clear_PlainQ(m_p);
}

int ByteQ::Push(const void *pData, int nDataLen)
{
	int ret = Add_PlainQ(m_p, pData, nDataLen);
	if (ret > 0) {
		_adapt();
	}
	return ret;
}

int ByteQ::Push2(const void *pData1, int nDataLen1,
	             const void *pData2, int nDataLen2)
{
	int ret = Combine_PlainQ(m_p, pData1, nDataLen1, pData2, nDataLen2);
	if (ret > 0) {
		_adapt();
	}
	return ret;
}

int ByteQ::Pop(void *pBuf, int nLen2pop)
{
	int ret = PopBytes_PlainQ(m_p, pBuf, nLen2pop);
	if (ret > 0) {
		_adapt();
	}
	return ret;
}

int ByteQ::GetNumBytes() const
{
	return GetCurSz_PlainQ(m_p);
}

int ByteQ::GetContMem(const void **ppMemHead, int *pLen,
	                  int startingInd, int len2req) const
{
	return GetContMem_PlainQ(m_p, ppMemHead, pLen, startingInd, len2req);
}

int ByteQ::Copy(void *pTar, int nTarLen, int ind) const
{
	return Copy_PlainQ(m_p, pTar, nTarLen, ind);
}

int ByteQ::ProcQ(CB_PROCFUNC procFunc, void *pArg)
{
	int ret = ProcQ_PlainQ(m_p, procFunc, pArg);
	if (ret > 0) {
		_adapt();
	}
	return ret;
}

void ByteQ::_adapt()
{
	int sz = GetCurSz_PlainQ(m_p);
	if (sz >= 0) {
		++m_nSZCnt;
		m_nSZSum += sz;
	}
	if (m_nSZCnt >= g_SZAdjustPeriod) {
		int newCap = (int)((m_nSZSum + 0.0f) / m_nSZCnt + 0.5f);
		AdjustCap_PlainQ(m_p, (newCap << 1));
		m_nSZCnt = m_nSZSum = 0;
	}
}


AdaptiveMsgQ::AdaptiveMsgQ(int capType, int capVal)
{
	if (capType < 0 || capType > 2) {
		m_capType = -1;
		return;
	}
	m_p = Create_MsgQ();
	if (!m_p) {
		m_capType = -1;
		return;
	}
	m_capType = capType;
	m_capVal = capVal;
	if (m_capVal <= 64) {
		m_capVal = 64;
	}
	m_nSZCnt = m_nSZSum = 0;
}

AdaptiveMsgQ::~AdaptiveMsgQ()
{
	Destroy_MsgQ(m_p);
}

void AdaptiveMsgQ::Clear()
{
	Clear_MsgQ(m_p);
}

int AdaptiveMsgQ::Push(const void *pMsg, int nMsgLen)
{
	if (-1 == m_capType) {
		return -1;
	}
	if (1 == m_capType) { // #ofMsg
		while (GetNumMsg_MsgQ(m_p) >= m_capVal) {
			PopMsg_MsgQ(m_p, nullptr, 0);
		}
	} else if (2 == m_capType) { // #ofBytes
		while (GetCurSz_MsgQ(m_p) >= m_capVal) {
			PopMsg_MsgQ(m_p, nullptr, 0);
		}
	}
	int ret = Add_MsgQ(m_p, pMsg, nMsgLen);
	if (ret >= 0) {
		_adapt();
	}
	_adapt();
	return ret;
}

int AdaptiveMsgQ::Push2(const void *pMsg1, int nMsgLen1,
	                    const void *pMsg2, int nMsgLen2)
{
	if (-1 == m_capType) {
		return -1;
	}
	if (1 == m_capType) { // #ofMsg
		while (GetNumMsg_MsgQ(m_p) >= m_capVal) {
			PopMsg_MsgQ(m_p, nullptr, 0);
		}
	}
	else if (2 == m_capType) { // #ofBytes
		while (GetCurSz_MsgQ(m_p) >= m_capVal) {
			PopMsg_MsgQ(m_p, nullptr, 0);
		}
	}
	int ret = Combine_MsgQ(m_p, pMsg1, nMsgLen1, pMsg2, nMsgLen2);
	if (ret >= 0) {
		_adapt();
	}
	_adapt();
	return ret;
}

int AdaptiveMsgQ::Pop(void *pBuf, int nBufLen)
{
	if (-1 == m_capType) {
		return -1;
	}
	int ret = PopMsg_MsgQ(m_p, pBuf, nBufLen);
	_adapt();
	return ret;
}

void AdaptiveMsgQ::_adapt()
{
	int sz = GetCurSz_MsgQ(m_p);
	if (sz >= 0) {
		++m_nSZCnt;
		m_nSZSum += sz;
	}
	if (m_nSZCnt >= g_SZAdjustPeriod) {
		int newCap = (int)((m_nSZSum + 0.0f) / m_nSZCnt + 0.5f);
		AdjustCap_MsgQ(m_p, (newCap << 1));
		m_nSZCnt = m_nSZSum = 0;
	}
}

int AdaptiveMsgQ::GetNumMsg() const
{
	if (-1 == m_capType) {
		return -1;
	}
	return GetNumMsg_MsgQ(m_p);
}

int AdaptiveMsgQ::GetNumBytes() const
{
	if (-1 == m_capType) {
		return -1;
	}
	return GetCurSz_MsgQ(m_p);
}

int AdaptiveMsgQ::GetNextMsgLen() const
{
	if (-1 == m_capType) {
		return -1;
	}
	return GetNextMsgLen_MsgQ(m_p);
}


}