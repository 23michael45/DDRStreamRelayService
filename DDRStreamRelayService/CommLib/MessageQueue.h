#ifndef __DDRCOMMLIB_WRAPPED_MSG_Q_H_INCLUDED__
#define __DDRCOMMLIB_WRAPPED_MSG_Q_H_INCLUDED__

namespace DDRCommLib {

class ByteQ
{
public:
	ByteQ();
	ByteQ(const ByteQ&) = delete;
	~ByteQ();

	void Clear();

	int Push(const void *pData, int nDataLen);
	int Push2(const void *pData1, int nDataLen1,
		      const void *pData2, int nDataLen2);

	// return number of bytes popped (-1 for error or insufficient buffer)
	int Pop(void *pBuf, int nLen2pop);

	int GetNumBytes() const;

	/* Try to get pointer to CONTIGUOUS memory in the queue
	pPQ            - pointer to the PlainQueue
	startingInd    - starting index of the queue
	len2req        - required length, -1 to allow all the way through the tail
	*ppHead, *pLen - filled with the pointer to queue[startingInd] and the actual
	length as required.
	Return value:
	0  - specified memory valid AND CONTIGUOUS.
	-1 - mem specified by startingInd and len INVALID
	1  - specified memory valid but not contiguous
	*/
	int GetContMem(const void **ppMemHead, int *pLen,
		           int startingInd = 0, int len2req = -1) const;
	int Copy(void *pTar, int nTarLen, int ind = 0) const;

	/* Callback function that processes [pHead, pHead + nLen) and return the
	number of bytes successfully processed. */
	typedef int(*CB_PROCFUNC)(const void *pHead, int nLen, void *pArg);
	// Process all bytes and pop the successfully processed bytes.
	int ProcQ(CB_PROCFUNC procFunc, void *pArg);

private:
	void *m_p;
	int m_nSZCnt;
	int m_nSZSum;

	void _adapt();
};

class AdaptiveMsgQ
{
public:
	// capType - 0, no limitation; 1 - limited by number of messages;
	//           2, limited by number of bytes occupied.
	AdaptiveMsgQ(int capType = 0, int capVal = 0);
	AdaptiveMsgQ(const AdaptiveMsgQ&) = delete;
	~AdaptiveMsgQ();

	void Clear();

	// return length of message pushed (-1 for error)
	int Push(const void *pMsg, int nMsgLen);

	int Push2(const void *pMsg1, int nMsgLen1,
		      const void *pMsg2, int nMsgLen2);

	// return number of bytes popped (-1 for error or insufficient buffer)
	int Pop(void *pBuf, int nBufLen);

	int GetNumMsg() const;
	int GetNumBytes() const;
	int GetNextMsgLen() const;

private:
	void *m_p;
	int m_capType;
	int m_capVal;
	int m_nSZCnt;
	int m_nSZSum;

	void _adapt();
};

}

#endif // __DDRCOMMLIB_WRAPPED_MSG_Q_H_INCLUDED__
