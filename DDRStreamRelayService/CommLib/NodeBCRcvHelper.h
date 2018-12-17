#ifndef __DDRCOMMLIB_NODEINFO_BROADCAST_RCV_HELPER_H_INCLUDED__
#define __DDRCOMMLIB_NODEINFO_BROADCAST_RCV_HELPER_H_INCLUDED__

#include <vector>
#include <string>
#include <chrono>
#include "SocketWrapper.h"

#ifndef U32
typedef unsigned int U32;
#endif
#ifndef U16
typedef unsigned short U16;
#endif
#ifndef I64
typedef long long I64;
#endif

namespace DDRCommLib {

class NodeBCRcvHelper {
public:
	NodeBCRcvHelper(const char *pNodeName,
		            char type,
					char transProto,
					U32 bcIP_BE,
					U16 bcPort_BE);
	bool Try2FindNode(U32 *pNodeIP_BE,
		              U16 *pNodePort_BE);
protected:
	MIMCClient m_bcRcv;
	std::string m_nodeName;
	char m_typeChar;
	char m_transChar;
	std::vector<bool> m_bNewBytes;
	std::vector<U32> m_srcIP;
	std::vector<U16> m_srcPort;
	std::vector<std::vector<char>> m_srcString;
	std::vector<std::chrono::system_clock::time_point> m_srcTic;
	static const int s_MaxInactivePeriod;
	static const int s_MaxInfoLength;
};

}

#endif // __DDRCOMMLIB_NODEINFO_BROADCAST_RCV_HELPER_H_INCLUDED__
