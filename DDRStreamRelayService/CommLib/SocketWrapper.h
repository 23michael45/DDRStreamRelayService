#ifndef __DDRCOMMLIB_SOCKET_WRAPPER_H_INCLUDED__
#define __DDRCOMMLIB_SOCKET_WRAPPER_H_INCLUDED__

#ifndef U32
typedef unsigned int U32;
#endif
#ifndef U16
typedef unsigned short U16;
#endif

namespace DDRCommLib {

class SocSet;
class MIMCServer;
class MIMCClient;

class SocketWrapper
{
public:
	/* type:
	0 - unspecified;
	1 - UDP socket;
	2 - TCP socket;
	*/
	SocketWrapper(int type = 0);

	SocketWrapper(const SocketWrapper &oriSW);
	SocketWrapper& operator= (const SocketWrapper &oriSW);
	~SocketWrapper();
	void Release();
	void Close();
	bool IsValid() const;

	// set socket's buffer sizes
	bool SetBuffer(int sndBufSz, int rcvBufSz);
	// set socket to block/non-blocking
	bool SetBlocking(bool bBlocking);
	// set the address this socket is bound to reusable or not
	bool SetAddrReuse(bool bReusable);
	// bind socket to target address
	bool Bind(U32 tarIP_BE, U16 tarPort_BE);
	// for bound (explicitly or implicitly), get its address
	bool GetBoundAddr(U32 *pIP_BE, U16 *pPort_BE);

	//********************************** UDP functions **********************************//
	// initialize as a UDP socket
	bool UDP_Init();
	// send one datagram to target address
	int UDP_SendTo(const char *pData, int nDataLen,
		           U32 tarIP_BE, U16 tarPort_BE);
	// try to receive a datagram (with peer's address info)
	int UDP_RecvFrom(char *pBuf, int nBufLen,
				     U32 *pPeerIP_BE, U16 *pPeerPort_BE);

	//********************************** TCP functions **********************************//
	// initialized as a TCP socket
	bool TCP_Init();
	// send bytes
	int TCP_Send(const char *pData, int nDataLen);
	// receive bytes
	int TCP_Recv(char *pBuf, int nBufLen);
	// listen (as a server entrance socket)
	bool TCP_Listen();
	// accept (as a server) connection to pTarSoc,
	// and filled pointer (if non-null) with peer address
	bool TCP_Accept(SocketWrapper *pTarSoc,
		            U32 *pPeerIP_BE,
					U16 *pPeerPort_BE);
	// connect to target address
	bool TCP_Connect(U32 tarIP_BE, U16 tarPort_BE);

	friend class SocSet;
	friend class MIMCServer;
	friend class MIMCClient;

private:
	struct __refInfo;
	struct __refInfo *m_pRef;
};


class MIMCServer
{
public:
	MIMCServer();
	void Refresh();
	bool IsValid() const;
	void Close();
	~MIMCServer();
	bool SetSndBuffer(int sndBufferSz);
	bool Broadcast(U32 tarMCIP_BE, U16 tarMCPort_BE,
		           const char *pBuf, int nBufLen);
private:
	struct _IMPL;
	_IMPL *m_pImpl;
};

class MIMCClient
{
public:
	MIMCClient();
	void Close();
	bool IsValid() const;
	~MIMCClient();
	bool JoinGroup(U32 tarMCIP_BE, U16 tarMCPort_BE);
	int RecvFrom(char *pBuf, int nBufLen,
				 U32 *pPeerIP_BE, U16 *pPeerPort_BE);
private:
	struct _IMPL;
	_IMPL *m_pImpl;
};

class SocSet
{
public:
	SocSet();
	~SocSet();
	void Clear();
	bool Add2Set(SocketWrapper *pSoc);
	bool Add2Set(SocketWrapper &soc);
	static int Wait(SocSet *pRdSet, SocSet *pWrSet, SocSet *pExSet,
		            int timeMs);
	bool IsSet(SocketWrapper &soc);
	void ClrSoc(SocketWrapper &soc);
private:
	struct __IMPL;
	struct __IMPL *m_pImp;
};

}

#endif // __DDR_COMM_SOCKET_WRAPPER_H_INCLUDED__
