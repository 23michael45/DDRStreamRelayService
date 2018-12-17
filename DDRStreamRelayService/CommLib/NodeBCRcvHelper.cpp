#include "NodeBCRcvHelper.h"

#include <string.h>
#include <iostream>
#include "CommonFunc.h"

namespace DDRCommLib {

void _appendVec(const char *pData, int nDataLen, std::vector<char> &vec2Append)
{
	if (!pData || nDataLen <= 0) { return; }
	int pos = (int)vec2Append.size();
	vec2Append.resize(pos + nDataLen);
	memcpy(&vec2Append[pos], pData, nDataLen);
}

NodeBCRcvHelper::NodeBCRcvHelper(const char *pNodeName,
	                             char type, char transProto,
								 U32 bcIP_BE,
								 unsigned short bcPort_BE)
{
	m_bcRcv.JoinGroup(bcIP_BE, bcPort_BE);
	m_nodeName = pNodeName;
	m_typeChar = type;
	m_transChar = transProto;
}

const int NodeBCRcvHelper::s_MaxInactivePeriod = 5000;
const int NodeBCRcvHelper::s_MaxInfoLength = 200;

bool NodeBCRcvHelper::Try2FindNode(U32 *pNodeIP_BE,
	                               unsigned short *pNodePort_BE)
{
	// trying to receive and update received strings
	const int OnePackLen = 256;
	char ttt[OnePackLen];
	auto _tic = std::chrono::system_clock::now();
	U32 bcSrcIP;
	U16 bcSrcPort;
	while (1) {
		int n = m_bcRcv.RecvFrom(ttt, OnePackLen, &bcSrcIP, &bcSrcPort);
		if (n <= 0) {
			break;
		}
		bool bExistingSrc = false;
		for (int j = 0; j < (int)m_srcIP.size(); ++j) {
			if (bcSrcIP == m_srcIP[j] && bcSrcPort == m_srcPort[j]) {
				_appendVec(ttt, n, m_srcString[j]);
				m_bNewBytes[j] = true;
				m_srcTic[j] = _tic;
				bExistingSrc = true;
				break;
			}
		}
		if (!bExistingSrc) {
			m_srcIP.emplace_back(bcSrcIP);
			m_srcPort.emplace_back(bcSrcPort);
			m_srcString.emplace_back(std::vector<char>());
			_appendVec(ttt, n, m_srcString.back());
			m_srcTic.emplace_back(_tic);
			m_bNewBytes.push_back(true);
		}
	}

	// checking each source
	bool bRet = false;
	for (int i = (int)m_srcIP.size() - 1; i >= 0; --i) {
		if (!m_bNewBytes[i]) {
			I64 timediff = std::chrono::duration_cast
				<std::chrono::milliseconds>(_tic - m_srcTic[i]).count();
			if (timediff > s_MaxInactivePeriod) {
				m_bNewBytes.erase(m_bNewBytes.begin() + i);
				m_srcIP.erase(m_srcIP.begin() + i);
				m_srcPort.erase(m_srcPort.begin() + i);
				m_srcString.erase(m_srcString.begin() + i);
				m_srcTic.erase(m_srcTic.begin() + i);
			}
			continue;
		}
		if (m_srcString[i].empty()) {
			continue;
		}
		const char *pC = &m_srcString[i][0];
		const char *const pEnd = pC + m_srcString[i].size();
		while (pC < pEnd) {
			if (pEnd - pC < (int)sizeof(int)) {
				break;
			}
			int nlen;
			if (!VerifyDataLen_32(pC, &nlen) || nlen > s_MaxInfoLength) {
				++pC;
				continue;
			}
			if (pEnd - pC - (int)sizeof(int) < nlen) {
				break;
			}
			pC += sizeof(int);
			const char *pNextPos = pC + nlen;
			if (0 == m_nodeName.compare(pC) &&
				(int)m_nodeName.length() + 1 + 3 +
				(int)sizeof(unsigned short) == nlen) {
				pC += m_nodeName.length() + 1;
				if (*pC == m_typeChar && pC[1] == m_transChar) {
					*pNodeIP_BE = m_srcIP[i];
					*pNodePort_BE = *((unsigned short*)(pC + 3));
					bRet = true;
				}
				pC += 3 + sizeof(unsigned short);
				if (pC != pNextPos) {
					*pNodeIP_BE = 0;
					*pNodePort_BE = 0;
					bRet = false;
				}
				continue;
			} else {
				pC = pNextPos;
			}
		}
		if (pEnd > pC) {
			memmove(&m_srcString[i][0], pC, pEnd - pC);
		}
		m_srcString[i].resize(pEnd - pC);
		m_bNewBytes[i] = false;
	}

	return bRet;
}

}