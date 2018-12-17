#include "InfoStrProc.h"

#include <string.h>
#include <string>
#include <iostream>
#include <fstream>
#include "CommLib/CommonFunc.h"

namespace DDRCommLib { namespace MARS {

bool PackVecs(const char *pHead, int nLen,
	          std::vector<char> &dstVec,
			  bool bAppend) {
	if (!pHead || nLen <= 0) { return false; }
	int pos = bAppend ? ((int)dstVec.size()) : 0;
	dstVec.resize(pos + SizeOfELen_64() + nLen + EXTRA_BYTES_FOR_ENCRYPTION);
	*((I64*)(&dstVec[pos])) = EncryptDataLen_64(nLen + EXTRA_BYTES_FOR_ENCRYPTION);
	Txt_Encrypt(pHead, nLen, &dstVec[pos + SizeOfELen_64()],
		nLen + EXTRA_BYTES_FOR_ENCRYPTION);
	return true;
}

bool PackVecs(const std::vector<char> &srcVec,
	          std::vector<char> &dstVec,
			  bool bAppend) {
	if (srcVec.empty()) { return false; }
	int pos = bAppend ? ((int)dstVec.size()) : 0;
	dstVec.resize(pos + SizeOfELen_64() + (int)srcVec.size() + EXTRA_BYTES_FOR_ENCRYPTION);
	*((I64*)(&dstVec[pos])) = EncryptDataLen_64((int)srcVec.size() + EXTRA_BYTES_FOR_ENCRYPTION);
	Txt_Encrypt(&srcVec[0], (int)srcVec.size(),
		&dstVec[pos + SizeOfELen_64()],
		(int)srcVec.size() + EXTRA_BYTES_FOR_ENCRYPTION);
	return true;
}

OneChAVInfo& OneChAVInfo::operator = (const OneChAVInfo &ori) {
	if (0 == ori.avType) {
		this->avType = 0;
	} else {
		this->devType = ori.devType;
		this->avType = ori.avType;
		this->bwReq = ori.bwReq;
		this->accessStr = ori.accessStr;
		memcpy(this->chName, ori.chName, MAX_CHANNEL_NAME_LENGTH + 1);
	}
	return (*this);
}

OneChAllocation& OneChAllocation::operator = (const OneChAllocation &ori) {
	if (!ori.bAllocated) {
		this->bAllocated = false;
	} else {
		this->bAllocated = true;
		this->sessionID = ori.sessionID;
		memcpy(this->avpass, ori.avpass, MAX_AVPASS_LENGTH + 1);
		this->uip_BE = ori.uip_BE;
		this->port_BE = ori.port_BE;
	}
	return (*this);
}

bool AddHeaderAndPack(const char *pHeader, int nHeaderLen,
	                  const char *pContent, int nContentLen,
					  std::vector<char> &dstVec,
					  bool bAppend) {
	if (!pHeader || nHeaderLen <= 0 ||
		!pContent || nContentLen <= 0) {
		return false;
	}
	int pos = bAppend ? ((int)dstVec.size()) : 0;
	int newEncLen = nHeaderLen + nContentLen + EXTRA_BYTES_FOR_ENCRYPTION;
	dstVec.resize(pos + SizeOfELen_64() + newEncLen);
	*((I64*)(&dstVec[pos])) = EncryptDataLen_64(newEncLen);
	Txt_Encrypt2(pHeader, nHeaderLen, pContent, nContentLen,
		         &dstVec[pos + SizeOfELen_64()], newEncLen);
	return true;
}

DDR_IOSTR_TYPE AnalyzeInfoStr(const std::vector<char> &strVec,
	                          int *pStrContPos) {
	if (strVec.empty()) {
		return EMPTY;
	}

	if (strcmp((const char*)&strVec[0], "login") == 0) {
		*pStrContPos = 6;
		return LOGIN;
	}
	else if (strcmp((const char*)&strVec[0], "notice") == 0) {
		*pStrContPos = 7;
		return NOTICE;
	}
	else if (strcmp((const char*)&strVec[0], "status") == 0) {
		*pStrContPos = 7;
		return STATUS;
	}
	else if (strcmp((const char*)&strVec[0], "file") == 0) {
		*pStrContPos = 5;
		return FILE;
	}
	else if (strcmp((const char*)&strVec[0], "cmdO") == 0) {
		if (strVec.size() < 16) { return ILLEGAL; }
		int nLevels = (int)((unsigned char)strVec[5]);
		int pos = 5 + 1
			+ (int)(sizeof(U32) + sizeof(U16)) * nLevels
			+ sizeof(long long);
		if ((int)strVec.size() < pos + 2) {
			return ILLEGAL;
		}
		const char *pC = &strVec[pos];
		for (; *pC != '\0' && pC <= &(strVec.back()); ++pC);
		if (pC > &(strVec.back())) { return ILLEGAL; }
		*pStrContPos = pos;
		return COMMAND;
	}
	else if (strcmp((const char*)&strVec[0], "cmdR") == 0) {
		if (strVec.size() < 14) { return ILLEGAL; }
		int nLevels = (int)((unsigned char)strVec[5]);
		int pos = 5 + 1
			+ (int)(sizeof(U32) + sizeof(U16)) * nLevels
			+ sizeof(long long);
		if ((int)strVec.size() < pos) {
			return ILLEGAL;
		}
		*pStrContPos = pos;
		return CMD_RESP;
	}
	else if (strcmp((const char*)&strVec[0], "chat") == 0) {
		*pStrContPos = 5;
		return CHATTING;
	}
	else if (strcmp((const char*)&strVec[0], "alarm") == 0) {
		*pStrContPos = 6;
		return ALARM;
	}

	return ILLEGAL;
}

DDR_IOSTR_TYPE DecodeInfoStr(const char *pBufHead, int nBufLen,
	                         const char **pNextBufPos,
							 std::vector<char> &strVec,
							 int *pStrContPos)
{
	*pNextBufPos = pBufHead;
	if (nBufLen <= SizeOfELen_64()) { return INSUFFICIENT; }
	unsigned int elen;
	if (!VerifyDataLen_64(pBufHead, &elen)) {
		++(*pNextBufPos);
		return ILLEGAL;
	}
	if (0 == elen) {
		*pNextBufPos += SizeOfELen_64();
		return EMPTY;
	}
	if (nBufLen < SizeOfELen_64() + (int)elen) {
		return INSUFFICIENT;
	}

	*pNextBufPos += SizeOfELen_64() + elen;
	if (elen <= EXTRA_BYTES_FOR_ENCRYPTION) {
		return ILLEGAL;
	}
	strVec.resize(elen - EXTRA_BYTES_FOR_ENCRYPTION);
	if (!Txt_Decrypt(pBufHead + SizeOfELen_64(), elen,
		&strVec[0], elen - EXTRA_BYTES_FOR_ENCRYPTION)) {
		return ILLEGAL;
	}

	if (strcmp((const char*)&strVec[0], "login") == 0) {
		*pStrContPos = 6;
		return LOGIN;
	}
	else if (strcmp((const char*)&strVec[0], "notice") == 0) {
		*pStrContPos = 7;
		return NOTICE;
	}
	else if (strcmp((const char*)&strVec[0], "status") == 0) {
		*pStrContPos = 7;
		return STATUS;
	}
	else if (strcmp((const char*)&strVec[0], "file") == 0) {
		*pStrContPos = 5;
		return FILE;
	}
	else if (strcmp((const char*)&strVec[0], "cmdO") == 0) {
		if (strVec.size() < 16) { return ILLEGAL; }
		int nLevels = (int)((unsigned char)strVec[5]);
		int pos = 5 + 1
			+ (int)(sizeof(U32) + sizeof(U16)) * nLevels
			+ sizeof(long long);
		if ((int)strVec.size() < pos + 2) {
			return ILLEGAL;
		}
		const char *pC = &strVec[pos];
		for (; *pC != '\0' && pC <= &(strVec.back()); ++pC);
		if (pC > &(strVec.back())) { return ILLEGAL; }
		*pStrContPos = pos;
		return COMMAND;
	}
	else if (strcmp((const char*)&strVec[0], "cmdR") == 0) {
		if (strVec.size() < 14) { return ILLEGAL; }
		int nLevels = (int)((unsigned char)strVec[5]);
		int pos = 5 + 1
			+ (int)(sizeof(U32) + sizeof(U16)) * nLevels
			+ sizeof(long long);
		if ((int)strVec.size() < pos) {
			return ILLEGAL;
		}
		*pStrContPos = pos;
		return CMD_RESP;
	}
	else if (strcmp((const char*)&strVec[0], "chat") == 0) {
		*pStrContPos = 5;
		return CHATTING;
	}
	else if (strcmp((const char*)&strVec[0], "alarm") == 0) {
		*pStrContPos = 6;
		return ALARM;
	}

	return ILLEGAL;
}

int DecodeNextMessage(ByteQ *pByteQ, std::vector<char> &tmpVec,
	                  std::vector<char> &dstVec, void*)
{
	I64 elen;
	if (pByteQ->Copy(&elen, SizeOfELen_64()) < SizeOfELen_64()) {
		return 1;
	}
	unsigned int len1;
	if (!VerifyDataLen_64(&elen, &len1) ||
		(len1 <= EXTRA_BYTES_FOR_ENCRYPTION &&
		0 != len1)) {
		pByteQ->Pop(nullptr, 1);
		return -1;
	}
	if (0 == len1) { // empty string
		pByteQ->Pop(nullptr, SizeOfELen_64());
		dstVec.resize(0);
		return 0;
	}
	const char *pEncTxt;
	int nRet = pByteQ->GetContMem((const void**)(&pEncTxt), nullptr,
		                          SizeOfELen_64(), (int)len1);
	if (0 == nRet) { // contiguous memory, decrypt directly
		dstVec.resize((int)len1 - EXTRA_BYTES_FOR_ENCRYPTION);
		int ret;
		if (Txt_Decrypt(pEncTxt, (int)len1,
			            &dstVec[0], (int)dstVec.size())) {
			ret = 0;
		} else {
			ret = -1;
		}
		pByteQ->Pop(nullptr, SizeOfELen_64() + (int)len1);
		return ret;
	} else if (1 == nRet) { // memory around the tail, copy first, decrypt then
		tmpVec.resize((int)len1);
		pByteQ->Copy(&tmpVec[0], (int)len1, SizeOfELen_64());
		pByteQ->Pop(nullptr, SizeOfELen_64() + (int)len1);
		dstVec.resize((int)len1 - EXTRA_BYTES_FOR_ENCRYPTION);
		if (Txt_Decrypt(&tmpVec[0], (int)tmpVec.size(),
			            &dstVec[0], (int)dstVec.size())) {
			return 0;
		} else {
			return -1;
		}
	} else { // insufficient
		return 1;
	}
}

bool ReadChAVInfo(const char *pFileName,
	              OneChAVInfo *pChAVInfo) {
	std::ifstream ifs(pFileName);
	if (!ifs.is_open()) {
		return false;
	}
	std::string str;
	for (int i = 0; i < MAX_AUDIOVIDEO_CHANNELS; ++i) {
		int a;
		if (!(ifs >> a)) { return false; }
		if (a < 0) {
			pChAVInfo[i].avType = 0;
			pChAVInfo[i].bwReq = 0;
			continue;
		}
		pChAVInfo[i].devType = a;
		if (!(ifs >> str)) { return false; }
		a = -1;
		if (str.compare("A") == 0) {
			a = 1;
		} else if (str.compare("V") == 0) {
			a = 2;
		} else if (str.compare("AV") == 0) {
			a = 3;
		}
		if (-1 == a) { return false; }
		pChAVInfo[i].avType = (char)a;
		if (!(ifs >> pChAVInfo[i].accessStr)) { return false; }
		if (!(ifs >> str)) { return false; }
		if (str.length() > MAX_CHANNEL_NAME_LENGTH) { return false; }
		memcpy(pChAVInfo[i].chName, str.c_str(), str.length() + 1);
		if (!(ifs >> pChAVInfo[i].bwReq)) { return false; }
		if (pChAVInfo[i].bwReq <= 0) {
			pChAVInfo[i].avType = 0;
			pChAVInfo[i].bwReq = 0;
		}
	}
	return true;
}

bool Gen_RobotLogin(const char *pRID, const char *pPWD,
	                char cOpLv, long long cOpTime,
					const OneChAVInfo *pAVChannels,
					unsigned int UID,
					std::vector<char> &vec) {
	if (cOpLv < 0 || cOpLv > 9) { return false; }
	int ridLen = strlen(pRID);
	int pwdLen = strlen(pPWD);
	int nn = 6 + 15 + ridLen + 5 + pwdLen +
		     6 + 1 + 8 + sizeof(long long) +
			 4 + 1;
	for (int i = 0; i < MAX_AUDIOVIDEO_CHANNELS; ++i) {
		++nn;
		if (0 != pAVChannels[i].avType) {
			nn += strlen(pAVChannels[i].chName) + 1 + sizeof(int);
		} else if (pAVChannels[i].avType < 0 || pAVChannels[i].avType > 9) {
			return false;
		}
	}
	if (UID != 0) {
		nn += 5 + sizeof(unsigned int);
	}
	vec.resize(nn);
	int pos = 0;
	memcpy(&vec[pos], "login\0", 6);
	pos += 6;
	memcpy(&vec[pos], "Role=ROBOT;RID=", 15);
	pos += 15;
	memcpy(&vec[pos], pRID, ridLen);
	pos += ridLen;
	memcpy(&vec[pos], ";PWD=", 5);
	pos += 5;
	memcpy(&vec[pos], pPWD, pwdLen);
	pos += pwdLen;
	memcpy(&vec[pos], ";OpLv=", 6);
	pos += 6;
	vec[pos++] = cOpLv + '0';
	memcpy(&vec[pos], ";OpTime=", 8);
	pos += 8;
	*((long long*)(&vec[pos])) = cOpTime;
	pos += sizeof(long long);
	memcpy(&vec[pos], ";AV=", 4);
	pos += 4;
	for (int i = 0; i < MAX_AUDIOVIDEO_CHANNELS; ++i) {
		if (0 == pAVChannels[i].avType) {
			vec[pos++] = '0';
			continue;
		}
		vec[pos++] = pAVChannels[i].avType + '0';
		int slen = strlen(pAVChannels[i].chName);
		memcpy(&vec[pos], pAVChannels[i].chName, slen + 1);
		pos += slen + 1;
		*((int*)(&vec[pos])) = pAVChannels[i].bwReq;
		pos += sizeof(int);
	}
	if (UID != 0) {
		memcpy(&vec[pos], ";UID=", 5);
		pos += 5;
		*((unsigned int*)(&vec[pos])) = UID;
		pos += sizeof(unsigned int);
	}
	vec[pos++] = ';';
	return true;
}

bool Gen_MonitorLogin(const char *pRID, const char *pPWD,
	                  int mOpLv, const char *pNickname,
					  unsigned int UID,
					  std::vector<char> &vec) {
	if (mOpLv < 0 || mOpLv > 9) { return false; }
	int slen = strlen(pRID);
	vec.resize(6 + 17 + slen + 5 + strlen(pPWD) + 6 + 1 + 10 + strlen(pNickname) + 1
		+ (UID != 0 ? (5 + sizeof(unsigned int)) : 0));
	int pos = 0;
	memcpy(&vec[pos], "login\0", 6);
	pos += 6;
	memcpy(&vec[pos], "Role=MONITOR;RID=", 17);
	pos += 17;
	memcpy(&vec[pos], pRID, slen);
	pos += slen;
	memcpy(&vec[pos], ";PWD=", 5);
	pos += 5;
	slen = strlen(pPWD);
	memcpy(&vec[pos], pPWD, slen);
	pos += slen;
	memcpy(&vec[pos], ";OpLv=", 6);
	pos += 6;
	vec[pos++] = mOpLv + '0';
	memcpy(&vec[pos], ";Nickname=", 10);
	pos += 10;
	slen = strlen(pNickname);
	memcpy(&vec[pos], pNickname, slen);
	pos += slen;
	if (UID != 0) {
		memcpy(&vec[pos], ";UID=", 5);
		pos += 5;
		*((unsigned int*)(&vec[pos])) = UID;
		pos += sizeof(unsigned int);
	}
	vec[pos] = ';';
	return true;
}

bool Gen_ClientLogin(const char *pRID, const char *pPWD,
	                 int mOpLv, const char *pNickname,
					 unsigned int UID,
				     std::vector<char> &vec) {
	if (mOpLv < 0 || mOpLv > 9) { return false; }
	int slen = strlen(pRID);
	vec.resize(6 + 16 + slen + 5 + strlen(pPWD) + 6 + 1 + 10 + strlen(pNickname) + 1
		+ (UID != 0 ? (5 + sizeof(unsigned int)) : 0));
	int pos = 0;
	memcpy(&vec[pos], "login\0", 6);
	pos += 6;
	memcpy(&vec[pos], "Role=CLIENT;RID=", 16);
	pos += 16;
	memcpy(&vec[pos], pRID, slen);
	pos += slen;
	memcpy(&vec[pos], ";PWD=", 5);
	pos += 5;
	slen = strlen(pPWD);
	memcpy(&vec[pos], pPWD, slen);
	pos += slen;
	memcpy(&vec[pos], ";OpLv=", 6);
	pos += 6;
	vec[pos++] = mOpLv + '0';
	memcpy(&vec[pos], ";Nickname=", 10);
	pos += 10;
	slen = strlen(pNickname);
	memcpy(&vec[pos], pNickname, slen);
	pos += slen;
	if (UID != 0) {
		memcpy(&vec[pos], ";UID=", 5);
		pos += 5;
		*((unsigned int*)(&vec[pos])) = UID;
		pos += sizeof(unsigned int);
	}
	vec[pos] = ';';
	return true;
}

bool Gen_LSMLogin(const char *pModuleName,
	              unsigned int UID,
	              std::vector<char> &vec) {
	int slen = strlen(pModuleName);
	vec.resize(6 + 33 + slen + 1
		+ (UID != 0 ? (5 + sizeof(unsigned int)) : 0));
	int pos = 0;
	memcpy(&vec[pos], "login\0", 6);
	pos += 6;
	memcpy(&vec[pos], "Role=LOCALSERVICEMODULE;Nickname=", 33);
	pos += 33;
	memcpy(&vec[pos], pModuleName, slen);
	pos += slen;
	if (UID != 0) {
		memcpy(&vec[pos], ";UID=", 5);
		pos += 5;
		*((unsigned int*)(&vec[pos])) = UID;
		pos += sizeof(unsigned int);
	}
	vec[pos] = ';';
	return true;
}

bool Gen_SubChLogin(const char *pRID, const char *pPWD, unsigned int UID,
	                std::vector<char> &vec) {
	if (!pRID || '\0' == *pRID ||
		!pPWD || '\0' == *pPWD) {
		return false;
	}
	int slen1 = strlen(pRID);
	int slen2 = strlen(pPWD);
	vec.resize(6 + 20 + slen1 + 5 + slen2 + 1
		+ (UID != 0 ? (5 + sizeof(unsigned int)) : 0));
	int pos = 0;
	memcpy(&vec[pos], "login\0", 6);
	pos += 6;
	memcpy(&vec[pos], "Role=SUBCHANNEL;RID=", 20);
	pos += 20;
	memcpy(&vec[pos], pRID, slen1);
	pos += slen1;
	memcpy(&vec[pos], ";PWD=", 5);
	pos += 5;
	memcpy(&vec[pos], pPWD, slen2);
	pos += slen2;
	if (UID != 0) {
		memcpy(&vec[pos], ";UID=", 5);
		pos += 5;
		*((unsigned int*)(&vec[pos])) = UID;
		pos += sizeof(unsigned int);
	}
	vec[pos] = ';';
	return true;
}

bool Parse_LoginStr(const char *pStr, int nStrLen, LoginInfo &li) {
	if (!pStr || nStrLen <= 0) { return false; }
	li.UID = 0;
	li.pID = nullptr;
	li.nIDLen = 0;
	li.pNickName = nullptr;
	li.nNickNameLen = 0;
	li.pPWD = nullptr;
	li.nPWDLen = 0;

	const char *const pEnd = (const char*)pStr + nStrLen;
	const char *pC = (const char*)pStr;
	int bitSet = 0; // bit #0 - role; #1 - RID; #2 - PWD; #3 - OpLv; #4 - OpTime; #5- Nickname; #6 - AVCh; #7 - UID
	while (pC < pEnd) {
		const char *pt = pC;
		for (; pt < pEnd && '=' != *pt; ++pt);
		if (pEnd == pt) { return false; }
		const char *pe = pt + 1;
		for (; pe < pEnd && ';' != *pe; ++pe);
		if (pEnd == pe) { return false; }
		// pt@'=', pe@';'

		if (pt - pC == 4 && strncmp(pC, "Role", 4) == 0) { // "Role=...."
			if (bitSet & 0x01) { return false; }
			++pt;
			if (pe - pt == 5 && strncmp(pt, "ROBOT", 5) == 0) {
				li.role = 0;
			}
			else if (pe - pt == 7 && strncmp(pt, "MONITOR", 7) == 0) {
				li.role = 1;
			}
			else if (pe - pt == 6 && strncmp(pt, "CLIENT", 6) == 0) {
				li.role = 2;
			}
			else if (pe - pt == 18 && strncmp(pt, "LOCALSERVICEMODULE", 18) == 0) {
				li.role = 3;
			}
			else if (pe - pt == 10 && strncmp(pt, "SUBCHANNEL", 10) == 0) {
				li.role = 4;
			}
			else { return false; }
			bitSet |= 0x01;
		}

		else if (pt - pC == 3 && strncmp(pC, "RID", 3) == 0) { // "rID=..."
			if (bitSet & 0x02) { return false; }
			li.pID = ++pt;
			li.nIDLen = pe - pt;
			bitSet |= 0x02;
		}

		else if (pt - pC == 3 && strncmp(pC, "PWD", 3) == 0) { // "PWD=..."
			if (bitSet & 0x04) { return false; }
			li.pPWD = ++pt;
			li.nPWDLen = pe - pt;
			bitSet |= 0x04;
		}

		else if (pt - pC == 4 && strncmp(pC, "OpLv", 4) == 0) {
			// for monitor, this means the access level it requires
			// for robot, this means other parties' (e.g., local clients) highest access level
			if (bitSet & 0x08) { return false; }
			if (pe - pt != 2) { return false; }
			if (pt[1] < '0' || pt[1] > '9') { return false; }
			li.OpAccessLevel = pt[1] - '0';
			bitSet |= 0x08;
		}

		else if (pt - pC == 6 && strncmp(pC, "OpTime", 6) == 0) { // robot only
			// for robot, this means the connection time of the other party with the hightest access level (e.g., local clients)
			if (bitSet & 0x10) {
				return false;
			}
			if (pEnd - 1 - (++pt) < (int)sizeof(long long)) {
				return false;
			}
			li.OpTime = *((long long*)pt);
			pe = pt + sizeof(long long);
			bitSet |= 0x10;
		}

		else if (pt - pC == 8 && strncmp(pC, "Nickname", 8) == 0) { // "Nickname=..."
			if (bitSet & 0x20) { return false; }
			li.pNickName = ++pt;
			li.nNickNameLen = pe - pt;
			bitSet |= 0x20;
		}

		else if (pt - pC == 2 && strncmp(pC, "AV", 2) == 0) {
			// "AV=[AVType(0-N.A; 1-audio; 2-vidio; 3-AV]+[chName(ending with '\0')]+[bandwidth required(4 bytes integer)]
			if (bitSet & 0x40) { return false; }
			++pt;
			for (int i = 0; i < MAX_AUDIOVIDEO_CHANNELS; ++i) {
				if (*pt < '0' || *pt > '3') {
					return false;
				}
				li.chs[i].avType = (int)(*pt) - '0';
				if ('0' == *pt) {
					++pt;
					continue;
				}
				const char *pp = ++pt;
				for (; pp < pEnd && pp <= pt + MAX_CHANNEL_NAME_LENGTH && *pp != '\0'; ++pp);
				if (pp >= pEnd || pp > pt + MAX_CHANNEL_NAME_LENGTH) {
					return false;
				}
				if (pp + 1 + sizeof(int) + 1 >= pEnd) {
					return false;
				}
				if (*((int*)(pp + 1)) <= 0) {
					return false;
				}
				memcpy(li.chs[i].chName, pt, pp - pt + 1);
				li.chs[i].bwReq = *((int*)(pp + 1));
				pt = pp + 1 + sizeof(int);
			}
 			if (';' != *pt) {
				return false;
			}
			pe = pt;
			bitSet |= 0x40;
		}

		else if (pt - pC == 3 && strncmp(pC, "UID", 3) == 0) {
			if (bitSet & 0x80) { return false; }
			if (pEnd - 1 - (++pt) < (int)sizeof(unsigned int)) {
				return false;
			}
			pe = pt + sizeof(unsigned int);
			if (';' != *pe) { return false; }
			li.UID = *((unsigned int*)pt);
			bitSet |= 0x80;
		}

		else { return false; }

		pC = pe + 1;
	}

	if (!(bitSet & 0x01)) { return false; }
	if (0 == li.role && (bitSet != 0x5F && bitSet != 0xDF)) {
		// robot, all fields except nickname
		return false;
	}
	else if ((1 == li.role || 2 == li.role) && (bitSet != 0x2F && bitSet != 0xAF)) {
		// monitor/client - role/ID/PWD/OpLv/nickname
		return false;
	}
	else if (3 == li.role && (bitSet != 0x21 && bitSet != 0xA1)) {
		// local service module - role/nickname
		return false;
	}
	else if (4 == li.role && bitSet != 0x87) {
		return false;
	}
	return true;
}

bool Gen_LoginResp(int nRet, int role, const char *pRID,
				   unsigned int UID,
				   std::vector<char> &vec) {
	if (role < 0 || role > 3) { return false; }
	if (0 == nRet) {
		if (!pRID) { return false; }
		int slen = strlen(pRID);
		vec.resize(6 + 9 + 23 + slen + 5 + sizeof(unsigned int) + 1);
		int pos = 0;
		memcpy(&vec[pos], "login\0", 6);
		pos += 6;
		memcpy(&vec[pos], "YourRole=", 9);
		pos += 9;
		switch (role) {
		case 0:
			memcpy(&vec[pos], "ROBOT;RID=", 10);
			pos += 10;
			break;
		case 1:
			memcpy(&vec[pos], "MONITOR;RID=", 12);
			pos += 12;
			break;
		case 2:
			memcpy(&vec[pos], "CLIENT;RID=", 11);
			pos += 11;
			break;
		case 3:
			memcpy(&vec[pos], "LOCALSERVICEMODULE;RID=", 23);
			pos += 23;
			break;
		}
		memcpy(&vec[pos], pRID, slen);
		pos += slen;
		memcpy(&vec[pos], ";UID=", 5);
		pos += 5;
		*((unsigned int*)(&vec[pos])) = UID;
		pos += sizeof(unsigned int);
		vec[pos++] = ';';
		vec.resize(pos);
	} else {
		vec.resize(6 + 6);
		memcpy(&vec[0], "login\0", 6);
		memcpy(&vec[6], "Ret=1;", 6);
		vec[10] = nRet + '0';
	}
	return true;
}

bool Parse_LoginResp(const char *pStr, int nStrLen,
	                 int *pRetCode, int *pRole,
					 const char **pRIDPtr, int *pRIDLen,
					 unsigned int *pUID) {
	if (!pStr || nStrLen <= 0) { return false; }
	const char *const pEnd = (const char*)pStr + nStrLen;
	const char *pC = (const char*)pStr;
	int bitSet = 0;
	*pRetCode = 0;
	while (pC < pEnd) {
		const char *pt = pC;
		for (; pt < pEnd && '=' != *pt; ++pt);
		if (pEnd == pt) { return false; }
		const char *pe = pt + 1;
		for (; pe < pEnd && ';' != *pe; ++pe);
		if (pEnd == pe) { return false; }
		// pt@'=', pe@';'

		if (pt - pC == 8 && strncmp(pC, "YourRole", 8) == 0) { // "YourRole=...."
			if (bitSet & 0x01) { return false; }
			++pt;
			if (pe - pt == 5 && strncmp(pt, "ROBOT", 5) == 0) {
				*pRole = 0;
			}
			else if (pe - pt == 7 && strncmp(pt, "MONITOR", 7) == 0) {
				*pRole = 1;
			}
			else if (pe - pt == 6 && strncmp(pt, "CLIENT", 6) == 0) {
				*pRole = 2;
			}
			else if (pe - pt == 18 && strncmp(pt, "LOCALSERVICEMODULE", 18) == 0) {
				*pRole = 3;
			}
			else { return false; }
			bitSet |= 0x01;
		}

		else if (pt - pC == 3 && strncmp(pC, "RID", 3) == 0) { // "RID=..."
			if (bitSet & 0x02) { return false; }
			*pRIDPtr = ++pt;
			*pRIDLen = pe - pt;
			bitSet |= 0x02;
		}

		else if (pt - pC == 3 && strncmp(pC, "Ret", 3) == 0) { // "Ret=..."
			if (bitSet & 0x04) { return false; }
			if (pe - pt != 2) { return false; }
			*pRetCode = (int)(pt[1] - '0');
			bitSet |= 0x04;
		}

		else if (pt - pC == 3 && strncmp(pC, "UID", 3) == 0) { // "Ret=..."
			if (bitSet & 0x08) { return false; }
			if (pEnd - 1 - (++pt) != sizeof(unsigned int)) {
				return false;
			}
			pe = pt + sizeof(unsigned int);
			if (';' != *pe) { return false; }
			*pUID = *((unsigned int*)pt);
			bitSet |= 0x08;
		}

		else { return false; }

		pC = pe + 1;
	}

	return (0x04 == bitSet || 0x0B == bitSet);
}

bool Gen_RS2RR_AVPU(const OneChAVInfo *pReservedInfo, // only one channel
	                const OneChAllocation *pReservedAlloc,
					const OneChAVInfo *pDynamicAVInfo,
					const OneChAllocation *pDynamicAlloc,
					std::vector<char> &vec) {
	if (!pDynamicAVInfo || !pDynamicAlloc) {
		return false;
	}
	int nn = 7 + 16 + 3 + 1;
	if (pReservedInfo && pReservedAlloc &&
		pReservedAlloc->bAllocated) {
		nn += 4 + sizeof(U32) + sizeof(U16)
			  + strlen(pReservedAlloc->avpass) + 1 + 1;
	}
	nn += MAX_AUDIOVIDEO_CHANNELS;
	for (int i = 0; i < MAX_AUDIOVIDEO_CHANNELS; ++i) {
		if (pDynamicAVInfo[i].avType > 0 &&
			pDynamicAVInfo[i].avType < 4 &&
			pDynamicAlloc[i].bAllocated) {
			nn += sizeof(U32) + sizeof(U16)
				  + strlen(pDynamicAlloc[i].avpass) + 1;
		}
	}
	vec.resize(nn);
	int pos = 0;
	memcpy(&vec[pos], "notice\0", 7);
	pos += 7;
	memcpy(&vec[pos], "type=RS2RR_AVPU;", 16);
	pos += 16;
	if (pReservedInfo && pReservedAlloc &&
		pReservedAlloc->bAllocated) {
		memcpy(&vec[pos], "Res=", 4);
		pos += 4;
		*((U32*)(&vec[pos])) = pReservedAlloc->uip_BE;
		pos += sizeof(U32);
		*((U16*)(&vec[pos])) = pReservedAlloc->port_BE;
		pos += sizeof(U16);
		int slen = strlen(pReservedAlloc->avpass);
		memcpy(&vec[pos], pReservedAlloc->avpass, slen + 1);
		pos += slen + 1;
		vec[pos++] = ';';
	}
	memcpy(&vec[pos], "AV=", 3);
	pos += 3;
	for (int i = 0; i < MAX_AUDIOVIDEO_CHANNELS; ++i) {
		if (pDynamicAVInfo[i].avType > 0 &&
			pDynamicAVInfo[i].avType < 4 &&
			pDynamicAlloc[i].bAllocated) {
			vec[pos++] = (char)pDynamicAVInfo[i].avType;
			*((U32*)(&vec[pos])) = pDynamicAlloc[i].uip_BE;
			pos += sizeof(U32);
			*((U16*)(&vec[pos])) = pDynamicAlloc[i].port_BE;
			pos += sizeof(U16);
			int slen = strlen(pDynamicAlloc[i].avpass);
			memcpy(&vec[pos], pDynamicAlloc[i].avpass, slen + 1);
			pos += slen + 1;
		} else {
			vec[pos++] = (char)0x00;
		}
	}
	vec[pos] = ';';
	return true;
}

bool Parse_RS2RR_AVPU(const char *pStr, int nStrLen,
	                  OneChAllocation *pRes,
					  OneChAllocation *pDynamicAlloc) {
	if (!pStr || nStrLen <= 0) { return false; }
	const char *const pEnd = pStr + nStrLen;
	const char *pC = pStr;
	if (pEnd < pC + 16 || 0 != strncmp(pC, "type=RS2RR_AVPU;", 16)) {
		return false;
	}
	pC += 16;
	if (pEnd < pC + 5) { return false; }
	pRes->bAllocated = false;
	if (0 == strncmp(pC, "Res=", 4)) {
		pC += 4;
		if (pEnd <= pC + sizeof(U32) + sizeof(U16) + 2) {
			return false;
		}
		pRes->uip_BE = *((U32*)pC);
		pC += sizeof(U32);
		pRes->port_BE = *((U16*)pC);
		pC += sizeof(U16);
		const char *pp = pC;
		for (; pp < pEnd - 1 && *pp != '\0'; ++pp);
		if (pp >= pEnd - 1 || pp > pC + MAX_AVPASS_LENGTH) {
			return false;
		}
		memcpy(pRes->avpass, pC, pp - pC + 1);
		pRes->bAllocated = true;
		pC = pp + 1;
		if (pC >= pEnd || ';' != *pC) { return false; }
		pRes->bAllocated = true;
		++pC;
	}
	if (pEnd < pC + 3 || 0 != strncmp(pC, "AV=", 3)) {
		return false;
	}
	pC += 3;
	if (pEnd < pC + MAX_AUDIOVIDEO_CHANNELS) {
		return false;
	}
	for (int i = 0; i < MAX_AUDIOVIDEO_CHANNELS; ++i) {
		if (pC >= pEnd) { return false; }
		if ((char)0x00 == *pC) {
			++pC;
			pDynamicAlloc[i].bAllocated = false;
			pDynamicAlloc[i].avpass[0] = '\0';
			continue;
		} else if (*pC < 0 || *pC > 3) {
			return false;
		}
		++pC;
		if (pEnd <= pC + sizeof(U32) + sizeof(U16) + 2) {
			return false;
		}
		pDynamicAlloc[i].uip_BE = *((U32*)pC);
		pC += sizeof(U32);
		pDynamicAlloc[i].port_BE = *((U16*)pC);
		pC += sizeof(U16);
		const char *pp = pC;
		for (; pp < pEnd - 1 && *pp != '\0'; ++pp);
		if (pp >= pEnd - 1 || pp > pC + MAX_AVPASS_LENGTH) {
			return false;
		}
		memcpy(pDynamicAlloc[i].avpass, pC, pp - pC + 1);
		pDynamicAlloc[i].bAllocated = true;
		pC = pp + 1;
	}
	return (pEnd - 1 == pC && ';' == *pC);
}

bool Gen_RS2RR_ACCPU(int nMonitors, int mAccLv, long long mOpTime,
	                 std::vector<char> &vec) {
	if (mAccLv < 0 || mAccLv > 9 || nMonitors < 0) {
		return false;
	}
	vec.resize(7 + 27 + sizeof(int) + 8 + 1 + 9 + sizeof(long long) + 1);
	int pos = 0;
	memcpy(&vec[pos], "notice\0", 7);
	pos += 7;
	memcpy(&vec[pos], "type=RS2RR_ACCPU;nMonitors=", 27);
	pos += 27;
	*((int*)(&vec[pos])) = nMonitors;
	pos += sizeof(int);
	memcpy(&vec[pos], ";mAccLv=", 8);
	pos += 8;
	vec[pos++] = mAccLv + '0';
	memcpy(&vec[pos], ";mOpTime=", 9);
	pos += 9;
	*((long long*)(&vec[pos])) = mOpTime;
	pos += sizeof(long long);
	vec[pos] = ';';
	return true;
}

bool Parse_RS2RR_ACCPU(const char *pStr, int nStrLen,
	                   int *pnMonitors, int *pmAccLv,
					   long long *pmOpTime) {
	if (!pStr || nStrLen != 27 + sizeof(int) + 8 + 1 + 9 + sizeof(long long) + 1) {
		return false;
	}
	const char *const pEnd = pStr + nStrLen;
	const char *pC = pStr;
	if (pEnd <= pC + 27 || 0 != strncmp(pC, "type=RS2RR_ACCPU;nMonitors=", 27)) {
		return false;
	}
	pC += 27;
	if (pEnd <= pC + sizeof(int)) {
		return false;
	}
	*pnMonitors = *((int*)pC);
	pC += sizeof(int);
	if (pEnd <= pC + 8 || 0 != strncmp(pC, ";mAccLv=", 8)) {
		return false;
	}
	pC += 8;
	*pmAccLv = *(pC++) - '0';
	if (*pmAccLv < 0 || *pmAccLv > 9) { return false; }
	if (pEnd <= pC + 9 || 0 != strncmp(pC, ";mOpTime=", 9)) {
		return false;
	}
	pC += 9;
	*pmOpTime = *((long long*)pC);
	pC += sizeof(long long);
	return (pC + 1 == pEnd && ';' == pC[0]);
}

bool Gen_RR2RS_ACCPU(int nClients, int cAccLv, long long cOpTime,
	                 std::vector<char> &vec) {
	if (nClients < 0 || cAccLv < 0 || cAccLv > 9) {
		return false;
	}
	vec.resize(7 + 26 + sizeof(int) + 8 + 1 + 9 + sizeof(long long) + 1);
	int pos = 0;
	memcpy(&vec[pos], "notice\0", 7);
	pos += 7;
	memcpy(&vec[pos], "type=RR2RS_ACCPU;nClients=", 26);
	pos += 26;
	*((int*)(&vec[pos])) = nClients;
	pos += sizeof(int);
	memcpy(&vec[pos], ";cAccLv=", 8);
	pos += 8;
	vec[pos++] = cAccLv + '0';
	memcpy(&vec[pos], ";cOpTime=", 9);
	pos += 9;
	*((long long*)(&vec[pos])) = cOpTime;
	pos += sizeof(long long);
	vec[pos] = ';';
	return true;
}

bool Parse_RR2RS_ACCPU(const char *pStr, int nStrLen,
	                   int *pnClients, int *pCAccLv,
					   long long *pCOpTime) {
	if (!pStr || nStrLen != 26 + sizeof(int) + 8 + 1 + 9 + sizeof(long long) + 1) {
		return false;
	}
	const char *const pEnd = pStr + nStrLen;
	const char *pC = pStr;
	if (pEnd <= pC + 26 || 0 != strncmp(pC, "type=RR2RS_ACCPU;nClients=", 26)) {
		return false;
	}
	pC += 26;
	if (pEnd <= pC + sizeof(int)) {
		return false;
	}
	*pnClients = *((int*)pC);
	pC += sizeof(int);
	if (pEnd <= pC + 8 || 0 != strncmp(pC, ";cAccLv=", 8)) {
		return false;
	}
	pC += 8;
	*pCAccLv = *(pC++) - '0';
	if (*pCAccLv < 0 || *pCAccLv > 9) { return false; }
	if (pEnd <= pC + 9 || 0 != strncmp(pC, ";cOpTime=", 9)) {
		return false;
	}
	pC += 9;
	*pCOpTime = *((long long*)pC);
	pC += sizeof(long long);
	return (pC + 1 == pEnd && ';' == pC[0]);
}

bool Gen_RS2M_AVPU(const OneChAVInfo *pReservedInfo, // only one channel
	               const OneChAllocation *pReservedAlloc,
				   const OneChAVInfo *pDynamicAVInfo,
				   const OneChAllocation *pDynamicAlloc,
				   const bool *pbDynamicDownstreamAllocated,
				   std::vector<char> &vec) {
	if (!pDynamicAVInfo || !pDynamicAlloc) {
		return false;
	}
	int nn = 7 + 15 + 3 + 1;
	if (pReservedInfo && pReservedAlloc &&
		pReservedAlloc->bAllocated) {
		nn += 4 + sizeof(int) + sizeof(U32) + sizeof(U16)
			+ strlen(pReservedAlloc->avpass) + 1 + 1;
	}
	nn += MAX_AUDIOVIDEO_CHANNELS;
	for (int i = 0; i < MAX_AUDIOVIDEO_CHANNELS; ++i) {
		if (pDynamicAVInfo[i].avType > 0 &&
			pDynamicAVInfo[i].avType < 4 &&
			pDynamicAlloc[i].bAllocated &&
			pbDynamicDownstreamAllocated[i]) {
			nn += sizeof(U32) + sizeof(U16)
				+ strlen(pDynamicAlloc[i].avpass) + 1
				+ strlen(pDynamicAVInfo[i].chName) + 1;
		}
	}
	vec.resize(nn);
	int pos = 0;
	memcpy(&vec[pos], "notice\0", 7);
	pos += 7;
	memcpy(&vec[pos], "type=RS2M_AVPU;", 15);
	pos += 15;
	if (pReservedInfo && pReservedAlloc &&
		pReservedAlloc->bAllocated) {
		memcpy(&vec[pos], "Res=", 4);
		pos += 4;
		*((int*)(&vec[pos])) = pReservedInfo->bwReq;
		pos += sizeof(int);
		*((U32*)(&vec[pos])) = pReservedAlloc->uip_BE;
		pos += sizeof(U32);
		*((U16*)(&vec[pos])) = pReservedAlloc->port_BE;
		pos += sizeof(U16);
		int slen = strlen(pReservedAlloc->avpass);
		memcpy(&vec[pos], pReservedAlloc->avpass, slen + 1);
		pos += slen + 1;
		vec[pos++] = ';';
	}
	memcpy(&vec[pos], "AV=", 3);
	pos += 3;
	for (int i = 0; i < MAX_AUDIOVIDEO_CHANNELS; ++i) {
		if (pDynamicAVInfo[i].avType > 0 &&
			pDynamicAVInfo[i].avType < 4 &&
			pDynamicAlloc[i].bAllocated &&
			pbDynamicDownstreamAllocated[i]) {
			vec[pos++] = (char)pDynamicAVInfo[i].avType;
			*((U32*)(&vec[pos])) = pDynamicAlloc[i].uip_BE;
			pos += sizeof(U32);
			*((U16*)(&vec[pos])) = pDynamicAlloc[i].port_BE;
			pos += sizeof(U16);
			int slen = strlen(pDynamicAlloc[i].avpass);
			memcpy(&vec[pos], pDynamicAlloc[i].avpass, slen + 1);
			pos += slen + 1;
			slen = strlen(pDynamicAVInfo[i].chName);
			memcpy(&vec[pos], pDynamicAVInfo[i].chName, slen + 1);
			pos += slen + 1;
		} else {
			vec[pos++] = (char)0x00;
		}
	}
	vec[pos] = ';';
	return true;
}

bool Parse_RS2M_AVPU(const char *pStr, int nStrLen,
	                 OneChAVInfo *pResInfo, // one channel
					 OneChAllocation *pResAlloc,
					 OneChAVInfo *pDynamicInfo, // MAX_AUDIOVIDEO_CHANNELS
					 OneChAllocation *pDynamicAlloc) {
	if (!pStr || nStrLen <= 0) { return false; }
	const char *const pEnd = pStr + nStrLen;
	const char *pC = pStr;
	if (pEnd < pC + 15 || 0 != strncmp(pC, "type=RS2M_AVPU;", 15)) {
		return false;
	}
	pC += 15;
	if (pEnd < pC + 5) { return false; }
	pResAlloc->bAllocated = false;
	if (0 == strncmp(pC, "Res=", 4)) {
		pC += 4;
		if (pEnd <= pC + sizeof(int) + sizeof(U32) + sizeof(U16) + 2) {
			return false;
		}
		pResInfo->bwReq = *((int*)pC);
		pC += sizeof(int);
		pResAlloc->uip_BE = *((U32*)pC);
		pC += sizeof(U32);
		pResAlloc->port_BE = *((U16*)pC);
		pC += sizeof(U16);
		const char *pp = pC;
		for (; pp < pEnd - 1 && *pp != '\0'; ++pp);
		if (pp >= pEnd - 1 || pp > pC + MAX_AVPASS_LENGTH) {
			return false;
		}
		memcpy(pResAlloc->avpass, pC, pp - pC + 1);
		pC = pp + 1;
		if (pC >= pEnd || ';' != *pC) { return false; }
		pResAlloc->bAllocated = true;
		++pC;
	}
	if (pEnd < pC + 3 || 0 != strncmp(pC, "AV=", 3)) {
		return false;
	}
	pC += 3;
	if (pEnd < pC + MAX_AUDIOVIDEO_CHANNELS) {
		return false;
	}
	for (int i = 0; i < MAX_AUDIOVIDEO_CHANNELS; ++i) {
		if (pC >= pEnd) { return false; }
		if ((char)0x00 == *pC) {
			++pC;
			continue;
		} else if (*pC < 0 || *pC > 3) {
			return false;
		}
		pDynamicInfo[i].avType = *((unsigned char*)(pC++));
		if (pEnd <= pC + sizeof(U32) + sizeof(U16) + 2) {
			return false;
		}
		pDynamicAlloc[i].uip_BE = *((U32*)pC);
		pC += sizeof(U32);
		pDynamicAlloc[i].port_BE = *((U16*)pC);
		pC += sizeof(U16);
		const char *pp = pC;
		for (; pp < pEnd - 1 && *pp != '\0'; ++pp);
		if (pp >= pEnd - 1 || pp > pC + MAX_AVPASS_LENGTH) {
			return false;
		}
		memcpy(pDynamicAlloc[i].avpass, pC, pp - pC + 1);
		pC = ++pp;
		for (; pp < pEnd - 1 && *pp != '\0'; ++pp);
		if (pp >= pEnd - 1 || pp > pC + MAX_CHANNEL_NAME_LENGTH) {
			return false;
		}
		memcpy(pDynamicInfo[i].chName, pC, pp - pC + 1);
		pC = pp + 1;
		pDynamicAlloc[i].bAllocated = true;
	}
	return (pEnd - 1 == pC && ';' == *pC);
}

bool Gen_RS2M_ACCPU(bool bRobotOnline, bool bOp,
	                int nMonitors, int nClients,
				    std::vector<char> &vec) {
	if (nMonitors < 0 || nClients < 0) { return false; }
	vec.resize(7 + 29 + 1 + 5 + 1 + 11 + sizeof(int) + 10 + sizeof(int) + 1);
	int pos = 0;
	memcpy(&vec[pos], "notice\0", 7);
	pos += 7;
	memcpy(&vec[pos], "type=RS2M_ACCPU;bRobotOnline=", 29);
	pos += 29;
	vec[pos++] = bRobotOnline ? '1' : '0';
	memcpy(&vec[pos], ";bOp=", 5);
	pos += 5;
	vec[pos++] = bOp ? '1' : '0';
	memcpy(&vec[pos], ";nMonitors=", 11);
	pos += 11;
	*((int*)(&vec[pos])) = nMonitors;
	pos += sizeof(int);
	memcpy(&vec[pos], ";nClients=", 10);
	pos += 10;
	*((int*)(&vec[pos])) = nClients;
	pos += sizeof(int);
	vec[pos] = ';';
	return true;
}

bool Parse_RS2M_ACCPU(const char *pStr, int nStrLen,
	                  bool *pbRobotOnline, bool *pbOp,
					  int *pnMonitors, int *pnClients) {
	if (!pStr || nStrLen != 29 + 1 + 5 + 1 + 11 + sizeof(int) + 10 + sizeof(int) + 1) {
		return false;
	}
	const char *const pEnd = pStr + nStrLen;
	const char *pC = pStr;
	if (0 != strncmp(pC, "type=RS2M_ACCPU;bRobotOnline=", 29)) {
		return false;
	}
	pC += 29;
	*pbRobotOnline = ('1' == *(pC++)) ? true : false;
	if (0 != strncmp(pC, ";bOp=", 5)) {
		return false;
	}
	pC += 5;
	*pbOp = ('1' == *(pC++)) ? true : false;
	if (0 != strncmp(pC, ";nMonitors=", 11)) {
		return false;
	}
	pC += 11;
	*pnMonitors = *((int*)pC);
	pC += sizeof(int);
	if (0 != strncmp(pC, ";nClients=", 10)) {
		return false;
	}
	pC += 10;
	*pnClients = *((int*)pC);
	pC += sizeof(int);
	return (pC + 1 == pEnd && ';' == pC[0]);
}

bool Gen_RS2M_PWDDenial(std::vector<char> &vec) {
	vec.resize(7 + 24);
	memcpy(&vec[0], "notice\0", 7);
	memcpy(&vec[7], "type=INCORRECT_PASSWORD;", 24);
	return true;
}


bool Parse_RS2M_PWDDenail(const char *pStr, int nStrLen) {
	if (!pStr || nStrLen != 7 + 24) { return false; }
	return (0 == strcmp(pStr, "notice") &&
		    0 == strncmp(pStr + 7, "type=INCORRECT_PASSWORD;", 24));
}

bool Gen_MTalk(long long msgID1, long long msgID2,
	           int avType, std::vector<char> &vec) {
	if (avType <= 0 || avType > 3) { return false; }
	vec.resize(7 + 17 + sizeof(long long) + sizeof(long long) + 10);
	int pos = 0;
	memcpy(&vec[0], "notice\0", 7);
	pos += 7;
	memcpy(&vec[pos], "type=MTalk;msgID=", 17);
	pos += 17;
	*((long long*)(&vec[pos])) = msgID1;
	pos += sizeof(long long);
	*((long long*)(&vec[pos])) = msgID2;
	pos += sizeof(long long);
	memcpy(&vec[pos], ";avType=0;", 10);
	pos += 10;
	vec[pos - 2] = avType + '0';
	return true;
}

bool Check_MTalk(const char *pStr, int nStrLen) {
	return (pStr &&
		    17 + sizeof(long long) + sizeof(long long) + 10 == nStrLen &&
		    0 == strncmp(pStr, "type=MTalk;msgID=", 17));
}

bool Parse_MTalk(const char *pStr, int nStrLen,
	             long long *pMsgID1, long long *pMsgID2,
				 int *pAVType) {
	if (!pStr ||
		17 + sizeof(long long) + sizeof(long long) + 10 != nStrLen ||
		0 != strncmp(pStr, "type=MTalk;msgID=", 17)) {
		return false;
	}
	int pos = 17;
	*pMsgID1 = *((long long*)(pStr + pos));
	pos += sizeof(long long);
	*pMsgID2 = *((long long*)(pStr + pos));
	pos += sizeof(long long);
	if (0 != strncmp(pStr + pos, ";avType=", 8)) {
		return false;
	}
	pos += 8;
	*pAVType = pStr[pos] - '0';
	return (*pAVType > 0 && *pAVType <= 3 && ';' == pStr[pos + 1]);
}

bool Gen_MTalkResp(long long msgID1, long long msgID2,
	               std::vector<char> &vec) {
	vec.resize(7 + 21 + sizeof(long long) + sizeof(long long) + 1);
	int pos = 0;
	memcpy(&vec[0], "notice\0", 7);
	pos += 7;
	memcpy(&vec[pos], "type=MTalkResp;msgID=", 21);
	pos += 21;
	*((long long*)(&vec[pos])) = msgID1;
	pos += sizeof(long long);
	*((long long*)(&vec[pos])) = msgID2;
	pos += sizeof(long long);
	vec[pos] = ';';
	return true;
}

bool Check_MTalkResp(const char *pStr, int nStrLen) {
	return (pStr && 21 + sizeof(long long) + sizeof(long long) + 1 == nStrLen &&
		    0 == strncmp(pStr, "type=MTalkResp;msgID=", 21));
}

bool Parse_MTalkResp(const char *pStr, int nStrLen,
	                 long long *pMsgID1, long long *pMsgID2) {
	if (!pStr || 21 + sizeof(long long) + sizeof(long long) + 1 != nStrLen ||
		0 != strncmp(pStr, "type=MTalkResp;msgID=", 21)) {
		return false;
	}
	*pMsgID1 = *((long long*)(pStr + 21));
	*pMsgID2 = *((long long*)(pStr + 21 + sizeof(long long)));
	return (';' == pStr[nStrLen - 1]);
}

bool Gen_CTalk(long long msgID1, long long msgID2,
	           int avType, std::vector<char> &vec) {
	if (avType <= 0 || avType > 3) { return false; }
	vec.resize(7 + 17 + sizeof(long long) + sizeof(long long) + 10);
	int pos = 0;
	memcpy(&vec[pos], "notice\0", 7);
	pos += 7;
	memcpy(&vec[pos], "type=CTalk;msgID=", 17);
	pos += 17;
	*((long long*)(&vec[pos])) = msgID1;
	pos += sizeof(long long);
	*((long long*)(&vec[pos])) = msgID2;
	pos += sizeof(long long);
	memcpy(&vec[pos], ";avType=3;", 10);
	vec[pos + 8] = avType + '0';
	return true;
}

bool Check_CTalk(const char *pStr, int nStrLen) {
	return (pStr && 17 + sizeof(long long) + sizeof(long long) + 10 == nStrLen &&
	 	    0 == strncmp(pStr, "type=CTalk;msgID=", 17));
}

bool Parse_CTalk(const char *pStr, int nStrLen,
	             long long *pMsgID1, long long *pMsgID2,
				 int *pAVType) {
	if (!pStr ||
		17 + sizeof(long long) + sizeof(long long) + 10 != nStrLen ||
		0 != strncmp(pStr, "type=CTalk;msgID=", 17)) {
		return false;
	}
	int pos = 17;
	*pMsgID1 = *((long long*)(pStr + pos));
	pos += sizeof(long long);
	*pMsgID2 = *((long long*)(pStr + pos));
	pos += sizeof(long long);
	if (0 != strncmp(pStr + pos, ";avType=", 8)) {
		return false;
	}
	pos += 8;
	*pAVType = pStr[pos] - '0';
	return (*pAVType > 0 && *pAVType <= 3 && ';' == pStr[pos + 1]);
}

bool Gen_CTalkResp(long long msgID1, long long msgID2,
	               std::vector<char> &vec) {
	vec.resize(7 + 21 + sizeof(long long) + sizeof(long long) + 1);
	int pos = 0;
	memcpy(&vec[0], "notice\0", 7);
	pos += 7;
	memcpy(&vec[pos], "type=CTalkResp;msgID=", 21);
	pos += 21;
	*((long long*)(&vec[pos])) = msgID1;
	pos += sizeof(long long);
	*((long long*)(&vec[pos])) = msgID2;
	pos += sizeof(long long);
	vec[pos] = ';';
	return true;
}

bool Check_CTalkResp(const char *pStr, int nStrLen) {
	return (pStr && 21 + sizeof(long long) + sizeof(long long) + 1 == nStrLen &&
		    0 == strncmp(pStr, "type=CTalkResp;msgID=", 21));
}

bool Parse_CTalkResp(const char *pStr, int nStrLen,
	                 long long *pMsgID1, long long *pMsgID2) {
	if (!pStr || 21 + sizeof(long long) + sizeof(long long) + 1 != nStrLen ||
		0 != strncmp(pStr, "type=CTalkResp;msgID=", 21)) {
		return false;
	}
	*pMsgID1 = *((long long*)(pStr + 21));
	*pMsgID2 = *((long long*)(pStr + 21 + sizeof(long long)));
	return (';' == pStr[nStrLen - 1]);
}

bool Gen_RTalk(long long msgID1, long long msgID2,
	           std::vector<char> &vec) {
	vec.resize(7 + 17 + sizeof(long long) + sizeof(long long) + 1);
	int pos = 0;
	memcpy(&vec[0], "notice\0", 7);
	pos += 7;
	memcpy(&vec[pos], "type=RTalk;msgID=", 17);
	pos += 17;
	*((long long*)(&vec[pos])) = msgID1;
	pos += sizeof(long long);
	*((long long*)(&vec[pos])) = msgID2;
	pos += sizeof(long long);
	vec[pos] = ';';
	return true;
}

bool Check_RTalk(const char *pStr, int nStrLen) {
	return (pStr && 17 + sizeof(long long) + sizeof(long long) + 1 == nStrLen &&
		    0 == strncmp(pStr, "type=RTalk;msgID=", 17));
}

bool Parse_RTalk(const char *pStr, int nStrLen,
	             long long *pMsgID1, long long *pMsgID2) {
	if (!pStr || 17 + sizeof(long long) + sizeof(long long) + 1 != nStrLen ||
		0 != strncmp(pStr, "type=RTalk;msgID=", 17)) {
		return false;
	}
	*pMsgID1 = *((long long*)(pStr + 17));
	*pMsgID2 = *((long long*)(pStr + 17 + sizeof(long long)));
	return (';' == pStr[nStrLen - 1]);
}

bool Gen_RTalkResp(long long msgID1, long long msgID2,
	               int avType, int srcMode,
				   std::vector<char> &vec) {
	if (avType <= 0 || avType > 3 ||
		(srcMode != 1 && srcMode != 2)) {
		return false;
	}
	vec.resize(7 + 21 + sizeof(long long) + sizeof(long long) + 10 + 6);
	int pos = 0;
	memcpy(&vec[pos], "notice\0", 7);
	pos += 7;
	memcpy(&vec[pos], "type=RTalkResp;msgID=", 21);
	pos += 21;
	*((long long*)(&vec[pos])) = msgID1;
	pos += sizeof(long long);
	*((long long*)(&vec[pos])) = msgID2;
	pos += sizeof(long long);
	memcpy(&vec[pos], ";avType=3;", 10);
	pos += 10;
	vec[pos - 2] = avType + '0';
	memcpy(&vec[pos], "src=1;", 6);
	pos += 6;
	vec[pos - 2] = srcMode + '0';
	return true;
}

bool Check_RTalkResp(const char *pStr, int nStrLen) {
	return (pStr &&	21 + sizeof(long long) + sizeof(long long) + 10 + 6 == nStrLen &&
		    0 == strncmp(pStr, "type=RTalkResp;msgID=", 21));
}

bool Parse_RTalkResp(const char *pStr, int nStrLen,
	                 long long *pMsgID1, long long *pMsgID2,
					 int *pAVType, int *pSrc) {
	if (!pStr || 21 + sizeof(long long) + sizeof(long long) + 10 + 6 != nStrLen ||
		0 != strncmp(pStr, "type=RTalkResp;msgID=", 21)) {
		return false;
	}
	int pos = 21;
	*pMsgID1 = *((long long*)(pStr + pos));
	pos += sizeof(long long);
	*pMsgID2 = *((long long*)(pStr + pos));
	pos += sizeof(long long);
	if (0 != strncmp(pStr + pos, ";avType=", 8)) {
		return false;
	}
	pos += 8;
	*pAVType = pStr[pos++] - '0';
	if (*pAVType <= 0 || *pAVType > 3) { return false; }
	if (0 != strncmp(pStr + pos, ";src=", 5)) {
		return false;
	}
	pos += 5;
	*pSrc = pStr[pos++] - '0';
	if ((*pSrc != 1 && *pSrc != 2) || ';' != pStr[pos]) {
		return false;
	}
	return true;
}

bool Gen_Command(const char *pCmdKeyword,
	             const char *pArgs, int nArgsLenInBytes,
				 long long cmdID,
				 std::vector<char> &vec) {
	if (!pCmdKeyword) { return false; }
	int slen = strlen(pCmdKeyword);
	if (0 == slen) { return false; }
	if (!pArgs || nArgsLenInBytes < 0) {
		nArgsLenInBytes = 0;
	}
	vec.resize(5 + 1 + sizeof(long long) + slen + 1 + nArgsLenInBytes);
	int pos = 0;
	memcpy(&vec[pos], "cmdO\0", 5);
	pos += 5;
	vec[pos++] = 0x00;
	*((long long*)(&vec[pos])) = cmdID;
	pos += sizeof(long long);
	memcpy(&vec[pos], pCmdKeyword, slen + 1);
	pos += slen + 1;
	if (nArgsLenInBytes) {
		memcpy(&vec[pos], pArgs, nArgsLenInBytes);
	}
	return true;
}

bool Pass_Command(const char *pCmdHead, int nCmdLen,
	              U32 srcIP_BE, U16 srcPort_BE,
				  std::vector<char> &vec) {
	if (!pCmdHead || nCmdLen < 8 + (int)sizeof(long long)) { return false; }
	int pos = 5;
	int nPreRoutes = ((unsigned char*)pCmdHead)[pos++];
	if (nPreRoutes >= 0xFF) { return false; } // too many pre-routes!
	pos += (sizeof(U32) + sizeof(U16)) * nPreRoutes;
	pos += sizeof(long long);
	if (pos + 2 > nCmdLen) { return false; }
	for (; pCmdHead[pos] != '\0' && pos < nCmdLen; ++pos);
	if (pos >= nCmdLen) { return false; }
	// string checking passed
	vec.resize(nCmdLen + sizeof(U32) + sizeof(U16));
	memcpy(&vec[0], "cmdO\0", 5);
	pos = 5;
	*((unsigned char*)(&vec[pos++])) = (unsigned char)(nPreRoutes + 1);
	*((U32*)(&vec[pos])) = srcIP_BE;
	pos += sizeof(U32);
	*((U16*)(&vec[pos])) = srcPort_BE;
	pos += sizeof(U16);
	memcpy(&vec[pos], pCmdHead + 6, nCmdLen - 6);
	return true;
}

bool Parse_Command(const char *pCmdHead, int nCmdLen,
	               const char **ppKeyword,
				   const char **ppArgs,
				   int *pArgLen)
{
	if (!pCmdHead || nCmdLen < 8 + (int)sizeof(long long)) {
		return false;
	}
	int pos = 5;
	int nPreRoutes = ((unsigned char*)pCmdHead)[pos++];
	pos += (sizeof(U32) + sizeof(U16)) * nPreRoutes;
	pos += sizeof(long long);
	if (pos + 2 > nCmdLen) {
		return false;
		}
	*ppKeyword = pCmdHead + pos;
	for (; pCmdHead[pos] != '\0' && pos < nCmdLen; ++pos);
	if (pos >= nCmdLen) {
		return false;
	}
	*ppArgs = pCmdHead + (++pos);
	*pArgLen = nCmdLen - pos;
	return true;
}

bool Gen_CmdResp(const char *pCmdHead, int nCmdLen, const char *pKeyword,
	             const char *pResponse, int nRespLen,
				 std::vector<char> &vec) {
	if (!pCmdHead || nCmdLen < 8 + (int)sizeof(long long) ||
		pKeyword < pCmdHead + 6 + (int)sizeof(long long) ||
		pKeyword >= pCmdHead + nCmdLen) {
		return false;
	}
	if (!pResponse || nRespLen < 0) {
		nRespLen = 0;
	}
	vec.resize(pKeyword - pCmdHead + nRespLen);
	memcpy(&vec[0], pCmdHead, pKeyword - pCmdHead);
	if (nRespLen > 0) {
		memcpy(&vec[pKeyword - pCmdHead], pResponse, nRespLen);
	}
	vec[3] = 'R'; // "cmdO" to "cmdR"
	return true;
}

bool Pass_CmdResp(const char *pCmdRespHead, int nCmdRespLen,
	              U32 *pTarIP_BE, U16 *pTarPort_BE,
				  std::vector<char> &vec) {
	if (nCmdRespLen < 6 + (int)sizeof(long long)) {
		return false;
	}
	int pos = 5;
	int nPreRoutes = ((unsigned char*)pCmdRespHead)[pos++];
	if (0 == nPreRoutes) { return false; }
	if (nCmdRespLen - pos < nPreRoutes * (int)(sizeof(U32) + sizeof(U16))
		+ (int)sizeof(long long)) {
		return false;
	}
	*pTarIP_BE = *((U32*)(pCmdRespHead + pos));
	pos += sizeof(U32);
	*pTarPort_BE = *((U16*)(pCmdRespHead + pos));
	pos += sizeof(U16);
	vec.resize(nCmdRespLen - (sizeof(U32) + sizeof(U16)));
	pos = 0;
	memcpy(&vec[0], "cmdR\0", 5);
	pos += 5;
	*((unsigned char*)(&vec[pos++])) = (unsigned char)(nPreRoutes - 1);
	memcpy(&vec[pos], pCmdRespHead + 6 + (sizeof(U32) + sizeof(U16)),
		   nCmdRespLen - 6 - (sizeof(U32) + sizeof(U16)));
	return true;
}

bool Parse_CmdResp(const char *pCmdRespHead, int nCmdRespLen,
	               long long *pCmdID,
				   const char **ppResp, int *pRespLen) {
	if (nCmdRespLen < 6 + (int)sizeof(long long)) {
		return false;
	}
	int pos = 5;
	if (0 != pCmdRespHead[pos++]) {
		return false;
		}
	if (nCmdRespLen - pos < (int)sizeof(long long)) {
		return false;
	}
	*pCmdID = *((long long*)(pCmdRespHead + pos));
	pos += sizeof(long long);
	if (pos < nCmdRespLen) {
		*ppResp = pCmdRespHead + pos;
		*pRespLen = nCmdRespLen - pos;
	} else {
		*ppResp = nullptr;
		*pRespLen = 0;
	}
	return true;
}

bool Gen_LS2C_AVPU(const OneChAVInfo *pLocalRes,
	               const OneChAVInfo *pLocalDynamicChs,
				   std::vector<char> &vec) {
	int nn = 7 + 15 + 3 + 1;
	nn += MAX_AUDIOVIDEO_CHANNELS;
	for (int i = 0; i < MAX_AUDIOVIDEO_CHANNELS; ++i) {
		if (pLocalDynamicChs[i].avType < 0 ||
			pLocalDynamicChs[i].avType > 3) {
			return false;
		}
		if (pLocalDynamicChs[i].avType > 0) {
			nn += pLocalDynamicChs[i].accessStr.length() + 1 +
				  strlen(pLocalDynamicChs[i].chName) + 1;
		}
	}
	if (pLocalRes) {
		nn += 9 + pLocalRes->accessStr.length() + 1 + 1;
	}
	vec.resize(nn);
	int pos = 0;
	memcpy(&vec[pos], "notice\0", 7);
	pos += 7;
	memcpy(&vec[pos], "type=LS2C_AVPU;", 15);
	pos += 15;
	if (pLocalRes) {
		memcpy(&vec[pos], "LocalRes=", 9);
		pos += 9;
		memcpy(&vec[pos], pLocalRes->accessStr.c_str(),
			   pLocalRes->accessStr.length() + 1);
		pos += pLocalRes->accessStr.length() + 1;
		vec[pos++] = ';';
	}
	memcpy(&vec[pos], "AV=", 3);
	pos += 3;
	for (int i = 0; i < MAX_AUDIOVIDEO_CHANNELS; ++i) {
		if (0 == pLocalDynamicChs[i].avType) {
			vec[pos++] = '0';
		} else if (pLocalDynamicChs[i].avType <= 3) {
			vec[pos++] = pLocalDynamicChs[i].avType + '0';

			memcpy(&vec[pos], pLocalDynamicChs[i].accessStr.c_str(),
				   pLocalDynamicChs[i].accessStr.length() + 1);
			pos += pLocalDynamicChs[i].accessStr.length() + 1;
			int slen = strlen(pLocalDynamicChs[i].chName);
			memcpy(&vec[pos], pLocalDynamicChs[i].chName, slen + 1);
			pos += slen + 1;
		}
	}
	vec[pos] = ';';
	return true;
}

bool Parse_LS2C_AVPU(const char *pStr, int nStrLen,
	                 OneChAVInfo *pLocalRes,
					 OneChAVInfo *pLocalDynamicChs) {
	if (!pStr || nStrLen < 15 + 3 + MAX_AUDIOVIDEO_CHANNELS + 1) {
		return false;
	}
	int pos = 0;
	if (0 != strncmp(pStr, "type=LS2C_AVPU;", 15)) {
		return false;
	}
	pos += 15;
	if (0 == strncmp(pStr + pos, "LocalRes=", 9)) {
		pos += 9;
		const char *pp = pStr + pos;
		for (; *pp != '\0' && pp < pStr + nStrLen - 1; ++pp);
		if (pp >= pStr + nStrLen - 1 ||
			';' != pp[1]) { return false; }
		pLocalRes->accessStr = std::string(pStr + pos);
		pos = pp + 2 - pStr;
	}
	if (pos + 3 + MAX_AUDIOVIDEO_CHANNELS > nStrLen) {
		return false;
	}
	if (0 != strncmp(pStr + pos, "AV=", 3)) { return false; }
	pos += 3;
	for (int i = 0; i < MAX_AUDIOVIDEO_CHANNELS; ++i) {
		pLocalDynamicChs[i].avType = pStr[pos++] - '0';
		if (pLocalDynamicChs[i].avType < 0 ||
			pLocalDynamicChs[i].avType > 3) {
			return false;
		}
		if (0 == pLocalDynamicChs[i].avType) { continue; }
		const char *pp = pStr + pos;
		for (; *pp != '\0' && pp < pStr + nStrLen - 1; ++pp);
		if (pp >= pStr + nStrLen - 1) { return false; }
		pLocalDynamicChs[i].accessStr = std::string(pStr + pos);
		pos = (++pp) - pStr;
		for (; *pp != '\0' && pp < pStr + nStrLen - 1; ++pp);
		if (pp >= pStr + nStrLen - 1 ||
			pp > pStr + pos + MAX_CHANNEL_NAME_LENGTH) { return false; }
		memcpy(pLocalDynamicChs[i].chName, pStr + pos,
			   pp - (pStr + pos) + 1);
		pos = pp + 1 - pStr;
	}
	return (nStrLen - 1 == pos && ';' == pStr[pos]);
}

bool Gen_LS2LSM_AVPU(const OneChAllocation *pRemoteRes,
	                 const OneChAVInfo *pLocalRes,
					 const OneChAVInfo *pAccessInfoChannels,
	                 const OneChAllocation *pRemoteAllocChannels,
					 std::vector<char> &vec) {
	int nn = 7 + 17 + 9 + pLocalRes->accessStr.length() + 1 +
		     4 + MAX_AUDIOVIDEO_CHANNELS + 1;
	for (int i = 0; i < MAX_AUDIOVIDEO_CHANNELS; ++i) {
#if 1
		if (pAccessInfoChannels[i].avType > 0 &&
			pAccessInfoChannels[i].avType <= 3) {
			nn += 1 + pAccessInfoChannels[i].accessStr.length() + 1 +
				sizeof(int) + sizeof(U32) + sizeof(U16) + 1;
			if (pRemoteAllocChannels[i].bAllocated) {
				nn += strlen(pRemoteAllocChannels[i].avpass);
			}
#else
		if (pRemoteAllocChannels[i].bAllocated &&
			pAccessInfoChannels[i].avType > 0 &&
			pAccessInfoChannels[i].avType <= 3) {
			nn += 1 + pAccessInfoChannels[i].accessStr.length() + 1 +
				sizeof(int) + sizeof(U32) + sizeof(U16) +
				strlen(pRemoteAllocChannels[i].avpass) + 1;
#endif
		} else if (pAccessInfoChannels[i].avType < 0 ||
			       pAccessInfoChannels[i].avType > 3) {
			return false;
		}
	}
	if (pRemoteRes && pRemoteRes->bAllocated) {
		nn += 10 + sizeof(U32) + sizeof(U16) +
			  strlen(pRemoteRes->avpass) + 1 + 1;
	}
	vec.resize(nn);
	int pos = 0;
	memcpy(&vec[pos], "notice\0", 7);
	pos += 7;
	memcpy(&vec[pos], "type=LS2LSM_AVPU;", 17);
	pos += 17;
	if (pRemoteRes && pRemoteRes->bAllocated) {
		memcpy(&vec[pos], "RemoteRes=", 10);
		pos += 10;
		*((U32*)(&vec[pos])) = pRemoteRes->uip_BE;
		pos += sizeof(U32);
		*((U16*)(&vec[pos])) = pRemoteRes->port_BE;
		pos += sizeof(U16);
		int slen = strlen(pRemoteRes->avpass);
		memcpy(&vec[pos], pRemoteRes->avpass, slen + 1);
		pos += slen + 1;
		vec[pos++] = ';';
	}
	memcpy(&vec[pos], "LocalRes=", 9);
	pos += 9;
	memcpy(&vec[pos], pLocalRes->accessStr.c_str(),
		   pLocalRes->accessStr.length() + 1);
	pos += pLocalRes->accessStr.length() + 1;
	memcpy(&vec[pos], ";AV=", 4);
	pos += 4;
	for (int i = 0; i < MAX_AUDIOVIDEO_CHANNELS; ++i) {
#if 1
		if (//pRemoteAllocChannels[i].bAllocated &&
			pAccessInfoChannels[i].avType > 0 &&
			pAccessInfoChannels[i].avType <= 3) {
			vec[pos++] = pAccessInfoChannels[i].avType + '0';
			vec[pos++] = pAccessInfoChannels[i].devType + '0';
			memcpy(&vec[pos], pAccessInfoChannels[i].accessStr.c_str(),
				pAccessInfoChannels[i].accessStr.length() + 1);
			pos += pAccessInfoChannels[i].accessStr.length() + 1;
			*((int*)(&vec[pos])) = pAccessInfoChannels[i].bwReq;
			pos += sizeof(int);
			*((U32*)(&vec[pos])) = pRemoteAllocChannels[i].uip_BE;
			pos += sizeof(U32);
			*((U16*)(&vec[pos])) = pRemoteAllocChannels[i].port_BE;
			pos += sizeof(U16);
			int slen = 0;
			if (pRemoteAllocChannels[i].bAllocated) {
				slen = strlen(pRemoteAllocChannels[i].avpass);
				memcpy(&vec[pos], pRemoteAllocChannels[i].avpass, slen + 1);
			} else {
				vec[pos] = '\0';
			}
			pos += slen + 1;
#else
		if (pRemoteAllocChannels[i].bAllocated &&
			pAccessInfoChannels[i].avType > 0 &&
			pAccessInfoChannels[i].avType <= 3) {
			vec[pos++] = pAccessInfoChannels[i].avType + '0';
			vec[pos++] = pAccessInfoChannels[i].devType + '0';
			memcpy(&vec[pos], pAccessInfoChannels[i].accessStr.c_str(),
				   pAccessInfoChannels[i].accessStr.length() + 1);
			pos += pAccessInfoChannels[i].accessStr.length() + 1;
			*((int*)(&vec[pos])) = pAccessInfoChannels[i].bwReq;
			pos += sizeof(int);
			*((U32*)(&vec[pos])) = pRemoteAllocChannels[i].uip_BE;
			pos += sizeof(U32);
			*((U16*)(&vec[pos])) = pRemoteAllocChannels[i].port_BE;
			pos += sizeof(U16);
			int slen = strlen(pRemoteAllocChannels[i].avpass);
			memcpy(&vec[pos], pRemoteAllocChannels[i].avpass, slen + 1);
			pos += slen + 1;
#endif
		} else {
			vec[pos++] = '0';
		}
	}
	vec[pos] = ';';
	return true;
}

bool Parse_LS2LSM_AVPU(const char *pStr, int nStrLen,
	                   OneChAllocation *pRemoteRes,
					   OneChAVInfo *pLocalRes,
	                   OneChAVInfo *pAccessInfoChannels,
					   OneChAllocation *pRemoteAllocChannels) {
	if (!pStr || nStrLen <= 0) { return false; }
	int pos = 0;
	if (nStrLen - pos < 17 || 0 != strncmp(pStr, "type=LS2LSM_AVPU;", 17)) {
		return false;
	}
	pos += 17;
	const char *pp;
	pRemoteRes->bAllocated = false;
	if (nStrLen - pos >= 10 && 0 == strncmp(pStr + pos, "RemoteRes=", 10)) {
		pos += 10;
		if (nStrLen - pos <= (int)sizeof(U32)
		                      + (int)sizeof(U16) + 2) {
			return false;
		}
		pRemoteRes->uip_BE = *((U32*)(pStr + pos));
		pos += sizeof(U32);
		pRemoteRes->port_BE = *((U16*)(pStr + pos));
		pos += sizeof(U16);
		pp = pStr + pos;
		for (; *pp != '\0' && pp < pStr + nStrLen - 1; ++pp);
		if (pp >= pStr + nStrLen - 1 || ';' != pp[1] ||
			pp > pStr + pos + MAX_CHANNEL_NAME_LENGTH) {
			return false;
		}
		memcpy(pRemoteRes->avpass, pStr + pos, pp - (pStr + pos) + 1);
		pos = pp + 2 - pStr;
		pRemoteRes->bAllocated = true;
	}
	if (nStrLen - pos < 9 || 0 != strncmp(pStr + pos, "LocalRes=", 9)) {
		return false;
	}
	pos += 9;
	pp = pStr + pos;
	for (; *pp != '\0' && pp < pStr + nStrLen - 1; ++pp);
	if (pp >= pStr + nStrLen - 1 || ';' != pp[1]) {
		return false;
	}
	pLocalRes->accessStr = std::string(pStr + pos);
	pos = pp + 2 - pStr;
	if (nStrLen - pos < 3 || 0 != strncmp(pStr + pos, "AV=", 3)) {
		return false;
	}
	pos += 3;
	for (int i = 0; i < MAX_AUDIOVIDEO_CHANNELS; ++i) {
		if (pos + 1 >= nStrLen) { return false; }
		pAccessInfoChannels[i].avType = pStr[pos++] - '0';
		if (pAccessInfoChannels[i].avType < 0 ||
			pAccessInfoChannels[i].avType > 3) {
			return false;
		}
		if (0 == pAccessInfoChannels[i].avType) { continue; }
		if (pos + 1 >= nStrLen) { return false; }
		pAccessInfoChannels[i].devType = pStr[pos++] - '0';
		if (pAccessInfoChannels[i].devType < 0) {
			return false;
		}
		pp = pStr + pos;
		for (; *pp != '\0' && pp < pStr + nStrLen - 1; ++pp);
		if (pp >= pStr + nStrLen - 1 - (sizeof(U32) + sizeof(U16) + 2)) {
			return false;
		}
		pAccessInfoChannels[i].accessStr = std::string(pStr + pos);
		pos = (++pp) - pStr;
		if (pos + (int)(sizeof(int) + sizeof(U32) + sizeof(U16))
			+ 2 >= nStrLen) {
			return false;
		}
		pAccessInfoChannels[i].bwReq = *((int*)(pStr + pos));
		pos += sizeof(int);
		pRemoteAllocChannels[i].uip_BE = *((U32*)(pStr + pos));
		pos += sizeof(U32);
		pRemoteAllocChannels[i].port_BE = *((U16*)(pStr + pos));
		pos += sizeof(U16);
		pp = pStr + pos;
		for (; *pp != '\0' && pp < pStr + nStrLen - 1; ++pp);
		if (pp >= pStr + nStrLen - 1 ||
			pp > pStr + pos + MAX_CHANNEL_NAME_LENGTH) {
			return false;
		}
#if 1
		if (pp > pStr + pos) {
			memcpy(pRemoteAllocChannels[i].avpass, pStr + pos,
				   pp - (pStr + pos) + 1);
			pRemoteAllocChannels[i].bAllocated = true;
		}
		else {
			pRemoteAllocChannels[i].avpass[0] = '\0';
			pRemoteAllocChannels[i].bAllocated = false;
		}
#else
		memcpy(pRemoteAllocChannels[i].avpass, pStr + pos,
			   pp - (pStr + pos) + 1);
		pRemoteAllocChannels[i].bAllocated = true;
#endif
		pos = pp + 1 - pStr;
	}
	return (nStrLen - 1 == pos && ';' == pStr[pos]);
}

bool Gen_LS2C_ACCPU(bool bOp,
	                int nMonitors, int nClients,
					std::vector<char> &vec) {
	if (nMonitors < 0 || nClients < 0) {
		return false;
	}
	vec.resize(7 + 19 + 1 + 11 + sizeof(int) + 10 + sizeof(int) + 1);
	int pos = 0;
	memcpy(&vec[pos], "notice\0", 7);
	pos += 7;
	memcpy(&vec[pos], "type=LS2C_ACCPU;bOp", 19);
	pos += 19;
	vec[pos++] = bOp ? '1' : '0';
	memcpy(&vec[pos], ";nMonitors=", 11);
	pos += 11;
	*((int*)(&vec[pos])) = nMonitors;
	pos += sizeof(int);
	memcpy(&vec[pos], ";nClients=", 10);
	pos += 10;
	*((int*)(&vec[pos])) = nClients;
	pos += sizeof(int);
	vec[pos] = ';';
	return true;
}

bool Parse_LS2C_ACCPU(const char *pStr, int nStrLen,
	                  bool *pbOp, int *pnMonitors, int *pnClients) {
	if (!pStr || nStrLen <= 0) {
		return false;
	}
	int pos = 0;
	if (pos + 19 > nStrLen || 0 != strncmp(pStr + pos, "type=LS2C_ACCPU;bOp", 19)) {
		return false;
	}
	pos += 19;
	if (pos + 1 > nStrLen) {
		return false;
	}
	*pbOp = (pStr[pos++] == '1');
	if (pos + 11 > nStrLen || 0 != strncmp(pStr + pos, ";nMonitors=", 11)) {
		return false;
	}
	pos += 11;
	if (pos + (int)sizeof(int) > nStrLen) {
		return false;
	}
	*pnMonitors = *((int*)(pStr + pos));
	pos += sizeof(int);
	if (pos + 10 > nStrLen || 0 != strncmp(pStr + pos, ";nClients=", 10)) {
		return false;
	}
	pos += 10;
	*pnClients = *((int*)(pStr + pos));
	pos += sizeof(int);
	return (pos + 1 == nStrLen && ';' == pStr[pos]);
}

bool Gen_LS2C_ACCPU(bool bOp, bool bRSConnected,
	                int nMonitors, int nClients,
					std::vector<char> &vec) {
	if (nMonitors < 0 || nClients < 0) {
		return false;
	}
	vec.resize(7 + 30 + 1 + + 5 + 1 + 11 + sizeof(int) + 10 + sizeof(int) + 1);
	int pos = 0;
	memcpy(&vec[pos], "notice\0", 7);
	pos += 7;
	memcpy(&vec[pos], "type=LS2C_ACCPU;bConnected2RS=", 30);
	pos += 30;
	vec[pos++] = bRSConnected ? '1' : '0';
	memcpy(&vec[pos], ";bOp=", 5);
	pos += 5;
	vec[pos++] = bOp ? '1' : '0';
	memcpy(&vec[pos], ";nMonitors=", 11);
	pos += 11;
	*((int*)(&vec[pos])) = nMonitors;
	pos += sizeof(int);
	memcpy(&vec[pos], ";nClients=", 10);
	pos += 10;
	*((int*)(&vec[pos])) = nClients;
	pos += sizeof(int);
	vec[pos] = ';';
	return true;
}

bool Parse_LS2C_ACCPU(const char *pStr, int nStrLen,
	                  bool *pbOp, bool *pbRSConnected,
					  int *pnMonitors, int *pnClients) {
	if (!pStr || nStrLen <= 0) {
		return false;
	}
	int pos = 0;
	if (pos + 30 > nStrLen || 0 != strncmp(pStr + pos, "type=LS2C_ACCPU;bConnected2RS=", 30)) {
		return false;
	}
	pos += 30;
	if (pos + 1 > nStrLen) {
		return false;
	}
	*pbRSConnected = (pStr[pos++] == '1');
	if (pos + 5 > nStrLen || 0 != strncmp(pStr + pos, ";bOp=", 5)) {
		return false;
	}
	pos += 5;
	if (pos + 1 > nStrLen) {
		return false;
	}
	*pbOp = (pStr[pos++] == '1');
	if (pos + 11 > nStrLen || 0 != strncmp(pStr + pos, ";nMonitors=", 11)) {
		return false;
	}
	pos += 11;
	if (pos + (int)sizeof(int) > nStrLen) {
		return false;
	}
	*pnMonitors = *((int*)(pStr + pos));
	pos += sizeof(int);
	if (pos + 10 > nStrLen || 0 != strncmp(pStr + pos, ";nClients=", 10)) {
		return false;
	}
	pos += 10;
	*pnClients = *((int*)(pStr + pos));
	pos += sizeof(int);
	return (pos + 1 == nStrLen && ';' == pStr[pos]);
}


bool Gen_LS2LSM_ACCPU(bool bRSConnected,
	                  int opMode,
					  int nMonitors, int nClients,
					  std::vector<char> &vec) {
	if (opMode < 0 || opMode > 2 || nMonitors < 0 || nClients < 0) {
		return false;
	}
	vec.resize(7 + 32 + 1 + 8 + 1 + 11 + sizeof(int) + 10 + sizeof(int) + 1);
	int pos = 0;
	memcpy(&vec[pos], "notice\0", 7);
	pos += 7;
	memcpy(&vec[pos], "type=LS2LSM_ACCPU;bConnected2RS=", 32);
	pos += 32;
	vec[pos++] = bRSConnected ? '1' : '0';
	memcpy(&vec[pos], ";opMode=", 8);
	pos += 8;
	vec[pos++] = opMode + '0';
	memcpy(&vec[pos], ";nMonitors=", 11);
	pos += 11;
	*((int*)(&vec[pos])) = nMonitors;
	pos += sizeof(int);
	memcpy(&vec[pos], ";nClients=", 10);
	pos += 10;
	*((int*)(&vec[pos])) = nClients;
	pos += sizeof(int);
	vec[pos] = ';';
	return true;
}

bool Parse_LS2LSM_ACCPU(const char *pStr, int nStrLen,
	                    bool *pbRSConnected, int *pOpMode,
						int *pnMonitors, int *pnClients) {

	if (!pStr || nStrLen <= 0) {
		return false;
	}
	int pos = 0;
	if (pos + 32 > nStrLen || 0 != strncmp(pStr + pos, "type=LS2LSM_ACCPU;bConnected2RS=", 32)) {
		return false;
	}
	pos += 32;
	if (pos + 1 > nStrLen) {
		return false;
	}
	*pbRSConnected = (pStr[pos++] == '1');
	if (pos + 8 > nStrLen || 0 != strncmp(pStr + pos, ";opMode=", 8)) {
		return false;
	}
	pos += 8;
	if (pos + 1 > nStrLen) {
		return false;
	}
	*pOpMode = (pStr[pos++] - '0');
	if (*pOpMode < 0 || *pOpMode > 2) { return false; }
	if (pos + 11 > nStrLen || 0 != strncmp(pStr + pos, ";nMonitors=", 11)) {
		return false;
	}
	pos += 11;
	if (pos + (int)sizeof(int) > nStrLen) {
		return false;
	}
	*pnMonitors = *((int*)(pStr + pos));
	pos += sizeof(int);
	if (pos + 10 > nStrLen || 0 != strncmp(pStr + pos, ";nClients=", 10)) {
		return false;
	}
	pos += 10;
	*pnClients = *((int*)(pStr + pos));
	pos += sizeof(int);
	return (pos + 1 == nStrLen && ';' == pStr[pos]);
}

}}
