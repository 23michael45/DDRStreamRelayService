#include "FileCache.h"

#include <stdio.h>
#include <string.h>
#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iostream>

#include "FIOWrapper.h"

namespace DDRCommLib {

class TimedMutexLocker
{
public:
	TimedMutexLocker(std::timed_mutex &tmutex, int nMillsecs)
	{
		m_pTMutex = &tmutex;
		if (nMillsecs < 0) {
			tmutex.lock();
			m_bLocked = true;
		} else {
			m_bLocked = tmutex.try_lock_for(std::chrono::milliseconds(nMillsecs));
		}
	}
	~TimedMutexLocker()
	{
		if (m_bLocked) {
			m_pTMutex->unlock();
		}
	}
	operator bool() const
	{
		return m_bLocked;
	}
private:
	std::timed_mutex *m_pTMutex;
	bool m_bLocked;
};

struct FCSentenceInfo
{
	unsigned char type; // 0 - INQ; 1 - RESP
	unsigned char func; // 0 - GetFile; 1 - ListFile; 2 - ListDir
	const char *pParamStart, *pParamEnd; // pointers to start and end of <param> [start, end)
	const char *pDataStart, *pDataEnd; // pointers to start and end of <param> [start, end)
	FCSentenceInfo() : type(0xFF), func(0xFF),
		pParamStart(nullptr), pParamEnd(nullptr),
		pDataStart(nullptr), pDataEnd(nullptr) {}

	bool IsValid() const
	{
		if (0xFF == type || 0xFF == func ||
			!pParamStart || !pParamEnd ||
			pParamStart >= pParamEnd) {
			return false;
		}
		if (0x00 == type && (!pParamStart || !pParamEnd)) {
			// INQ does not come with fileExist or pData info
			return false;
		}
		if (0x01 == type && (0x01 == func || 0x02 == func) &&
			(pDataStart || pDataEnd)) {
			// results by ListFile or ListDir is in <param> not <filedata>
			return false;
		}
		return true;
	}

	bool ParseSentence(const char *pSentenceStart, int nSentenceLen)
	{
		if (!pSentenceStart || nSentenceLen <= 6 + 7) { return false; }
		if (strncmp(pSentenceStart, "<head>", 6) != 0 ||
			strncmp(pSentenceStart + nSentenceLen - 7, "</head>", 7) != 0) {
			return false;
		}
		const char *pC = pSentenceStart + 6;
		const char *const pEnd = pSentenceStart + nSentenceLen - 7;
		while (pC < pEnd) {
			if ('<' != *pC) { return false; }
			const char *pt = pC + 1;
			for (; pt < pEnd && '>' != *pt; ++pt);
			if (pt + 1 + sizeof(int) >= pEnd) { return false; }
			int len = *((int*)(pt + 1));
			if (len < 0) { return false; }
			if (pt + 1 + sizeof(int) + len > pEnd) { return false; }
			const char *pContent = pt + 1 + sizeof(int);

			if (pt - pC - 1 == 4 && strncmp(pC + 1, "type", 4) == 0) {
				if (type != 0xFF) { return false; }
				if (3 == len && strncmp(pContent, "INQ", 3) == 0) {
					type = 0x00;
				}
				else if (4 == len && strncmp(pContent, "RESP", 4) == 0) {
					type = 0x01;
				}
				else { return false; }
			}
			else if (pt - pC - 1 == 8 && strncmp(pC + 1, "function", 8) == 0) {
				if (func != 0xFF) { return false; }
				if (7 == len && strncmp(pContent, "GetFile", 7) == 0) {
					func = 0x00;
				}
				else if (8 == len && strncmp(pContent, "ListFile", 8) == 0) {
					func = 0x01;
				}
				else if (7 == len && strncmp(pContent, "ListDir", 7) == 0) {
					func = 0x02;
				}
				else { return false; }
			}
			else if (pt - pC - 1 == 5 && strncmp(pC + 1, "param", 5) == 0) {
				if (pParamStart) { return false; }
				pParamStart = pContent;
				pParamEnd = pContent + len;
			}
			else if (pt - pC - 1 == 8 && strncmp(pC + 1, "filedata", 8) == 0) {
				if (pDataStart) { return false; }
				pDataStart = pContent;
				pDataEnd = pContent + len;
			}

			pC = pContent + len;
		}

		return IsValid();
	}
};

struct FileCache::__IMPL
{
	bool m_bValid;
	FCType m_FCType;
	std::string m_workpath;
	std::timed_mutex m_Process_lock;
	unsigned int m_expTime;
	std::chrono::system_clock::time_point m_tpLastUpdate;

	/*
	Retur value:
	0  - sentence successfully processed
	-1 - object invalid or unexpected exception
	1  - sentence invalid
	2  - INQ file/folder name illegal
	3  - INQ:list file/folder not existing
	4  - access denied;
	5  - busy
	*/
	int Process(std::vector<char> &respVec,
		        const char *pHead, int nLength,
		        std::vector<char> *pMissingFileList = nullptr,
		        int waitTimeMilSec = 50);

	bool getList(bool bFile,
		         const char *pNameHead, int nNameLen,
				 std::vector<char> &nameList,
				 std::vector<char> *pMissingList);

	bool getFileContent(const char *pNameHead, int nNameLen,
		                std::vector<char> &nameList,
						std::vector<char> &contents,
						std::vector<char> *pMissingList);

	bool writeFileDir(const char *pRelativeName, int nNameLen,
		              int type, // 0 - file, 1 - directory
					  const char *pFileData = nullptr,
					  int nFileDataLen = 0);

	void ensureWrkDir();
};


void FileCache::__IMPL::ensureWrkDir()
{
	if (m_workpath.length() > 1 && '/' == m_workpath.back()) {
		m_workpath.back() = '\0';
		_createDir(m_workpath.c_str());
		m_workpath.back() = '/';
	}
}

bool FileCache::__IMPL::getList(bool bFile,
		                        const char *pNameHead, int nNameLen,
								std::vector<char> &nameList,
								std::vector<char> *pMissingList)
{
	if (!pNameHead || nNameLen <= 0) {
		return false;
	}

	nameList.resize(0);
	std::vector<char> name(nNameLen + 1);
	memcpy(&name[0], pNameHead, nNameLen);
	name.back() = '\0';
	char *p0 = &name[0];
	char *p1 = p0;

	long long mHandle = (pMissingList != nullptr ? _getNewMissingHandle() : 0);
	for (; p1 <= &(name.back()); ++p1) {
		if (*p1 != '|' && *p1 != '\0') {
			continue;
		}
		if (p1 <= p0) {
			p0 = ++p1;
			continue;
		}
		*p1 = '\0';

		const char *pFN;
		bool bIsFile;
		long long handle = _findFirstFileHandle(m_workpath.c_str(), p0,
			                                    &pFN, &bIsFile, mHandle,
												(FC_READWRITE == m_FCType));
		if (handle > 0) {
			do {
				if (bIsFile==bFile) {
					int slen = strlen(pFN);
					int pos = (int)nameList.size();
					nameList.resize(pos + slen + 1);
					memcpy(&nameList[pos], pFN, slen);
					nameList.back() = '|';
				}
			} while (_findNextFile(handle, &pFN, &bIsFile));
			_closeFindHandle(handle);
		}

		p0 = ++p1;
	}

	if (pMissingList && mHandle != 0) {
		_rewindMissingEntry(mHandle);
		const char *pFN;
		while (_findNextMissingEntry(mHandle, &pFN)) {
			int slen = strlen(pFN);
			int pos = (int)pMissingList->size();
			pMissingList->resize(pos + slen + 1);
			memcpy(&(*pMissingList)[pos], pFN, slen);
			pMissingList->back() = '|';
		}
		pMissingList->emplace_back('\0');
		pMissingList->pop_back();
	}
	_closeMissingHandle(mHandle);
	return true;
}

bool FileCache::__IMPL::getFileContent(const char *pNameHead, int nNameLen,
	                                   std::vector<char> &nameList,
									   std::vector<char> &contents,
									   std::vector<char> *pMissingList)
{
	if (!getList(true, pNameHead, nNameLen, nameList, pMissingList)) {
		return false;
	}
	contents.resize(4);
	int nFiles = 0;
	if (nameList.size() > 0) {
		int wdLen = (int)m_workpath.length();
		std::vector<char> fn(wdLen);
		std::vector<char> neList;
		if (wdLen > 0) {
			memcpy(&fn[0], m_workpath.c_str(), wdLen);
		}
		const char *p0 = &nameList[0];
		const char *p1 = p0;
		for (; p1 <= &(nameList.back()); ++p1) {
			if ('|' != *p1 && '\0' != *p1) {
				continue;
			}
			if (p1 <= p0) {
				p0 = ++p1;
				continue;
			}
			fn.resize(wdLen + (p1 - p0) + 1);
			memcpy(&fn[wdLen], p0, p1 - p0);
			fn.back() = '\0';
			bool bFileNonEmpty = false;
			std::ifstream ifs(&fn[0], std::ios::binary);
			if (ifs.is_open()) {
				ifs.seekg(0, std::ios::end);
				int fileLen = (int)ifs.tellg();
				if (fileLen > 0) {
					bFileNonEmpty = true;
					int dataPos = (int)contents.size();
					contents.resize(dataPos + sizeof(int) + fileLen);
					*((int*)(&contents[dataPos])) = fileLen;
					ifs.seekg(0, std::ios::beg);
					ifs.read(&contents[dataPos + sizeof(int)], fileLen);
				}
				ifs.close();
			}
			if (bFileNonEmpty) {
				int pos = (int)neList.size();
				neList.resize(pos + (int)fn.size() - wdLen);
				memcpy(&neList[pos], &fn[wdLen], (int)fn.size() - wdLen - 1);
				neList.back() = '|';
				++nFiles;
			} else if (pMissingList) {
				int mlPos = (int)pMissingList->size();
				pMissingList->resize(mlPos + p1 - p0 + 1);
				memcpy(&((*pMissingList)[mlPos]), p0, p1 - p0);
				pMissingList->back() = '|';
			}
			p0 = ++p1;
		}
		neList.swap(nameList);
	} // end if (nameList NON-empty)
	*((int*)(&contents[0])) = nFiles;
	return true;
}

bool FileCache::__IMPL::writeFileDir(const char *pRelativeName, int nNameLen,
	                                 int type,
									 const char *pFileData, int nFileDataLen)
{
	if (FC_READWRITE != m_FCType) {
		return false;
	}
	if (!pRelativeName || nNameLen <= 0 ||
		(type != 0 && type != 1)) {
		return false;
	}
	std::vector<char> vec;
	int lPos;
	if ('\0' == *m_workpath.c_str()) {
		lPos = 0;
	} else {
		lPos = (int)m_workpath.length();
	}
	vec.resize(lPos + nNameLen + 1);
	if (lPos > 0) {
		memcpy(&vec[0], m_workpath.c_str(), lPos);
	}
	memcpy(&vec[lPos], pRelativeName, nNameLen);
	vec.back() = '\0';
	for (char *pC = &vec[lPos]; '\0' != *pC; ++pC) {
		if ('\\' == *pC || '/' == *pC) {
			*pC = '\0';
			_createDir(&vec[0]);
			if (FC_READWRITE == m_FCType) {
				_makeTimeNow(&vec[0]);
			}
			*pC = '/';
		}
	}
	if (0 == type) { // file
		std::ofstream ofs(&vec[0], std::ios::binary);
		if (!pFileData || nFileDataLen <= 0) {
			_makeTimeNow(&vec[0]);
			return true;
		}
		if (!ofs.is_open()) {
			return false;
		}
		if (pFileData && nFileDataLen > 0) {
			ofs.write(pFileData, nFileDataLen);
		}
	} else { // directory
		_createDir(&vec[0]);
		_makeTimeNow(&vec[0]);
	}
	return true;
}

int FileCache::__IMPL::Process(std::vector<char> &respVec,
	                           const char *pHead, int nLength,
							   std::vector<char> *pMissingFileList,
							   int waitTimeMilSec)
{
	if (!m_bValid) {
		return -1; // object invalid
	}
	FCSentenceInfo fcsi;
	if (!fcsi.ParseSentence(pHead, nLength)) {
		return 1; // sentence invalid
	}
	TimedMutexLocker tml(m_Process_lock, waitTimeMilSec);
	if (!tml) {
		return 5; // busy
	}

	respVec.resize(0);
	if (0x00 == fcsi.type) { // INQ
		ensureWrkDir();
		if (0x00 == fcsi.func) { // get file content
			std::vector<char> names, contents;
			if (!getFileContent(fcsi.pParamStart, fcsi.pParamEnd - fcsi.pParamStart,
				                names, contents, pMissingFileList)) {
				return 2; // INQ file/folder name illegal
			}
			if (names.empty()) {
				return 3; // INQ files not existing
			}
			FormOneSentence(respVec, FC_RESP, FC_GETFILE,
				            &names[0], (int)names.size(),
							&contents[0], (int)contents.size());
			return 0;
		} // end if "INQ:GetFile"

		else if (0x01 == fcsi.func) { // ListFile
			std::vector<char> names;
			if (!getList(true, fcsi.pParamStart,
				         fcsi.pParamEnd - fcsi.pParamStart,
						 names, nullptr)) {
				return 2;
			}
			if (names.empty()) {
				return 3; // INQ file/folder not existing
			}
			FormOneSentence(respVec, FC_RESP, FC_LISTFILE,
				            &names[0], (int)names.size());
			return 0;
		} // end if "INQ:ListFile"

		else if (0x02 == fcsi.func) { // ListDir
			std::vector<char> names;
			if (!getList(false, fcsi.pParamStart,
				         fcsi.pParamEnd - fcsi.pParamStart,
						 names, nullptr)) {
				return 2;
			}
			if (names.empty()) {
				return 3; // INQ file/folder not existing
			}
			FormOneSentence(respVec, FC_RESP, FC_LISTDIR,
				            &names[0], (int)names.size());
			return 0;
		} // end if "INQ:ListDir"
	}

	else if (0x01 == fcsi.type) { // RESP
		if (FC_READONLY == m_FCType) {
			return 4; // access denied
		}

		if (0x00 == fcsi.func) { // GetFile
			int nFiles = *((int*)fcsi.pDataStart);
			const char *pCurData = fcsi.pDataStart + sizeof(int);
			const char *pLastFNHead = fcsi.pParamStart;
			const char *pC = fcsi.pParamStart;
			for (; pC < fcsi.pParamEnd; ++pC) {
				if ('|' != *pC && '\0' != *pC) {
					continue;
				}
				if ('\\' == *pLastFNHead ||
					'/' == *pLastFNHead) {
					++pLastFNHead;
				}
				if (pC <= pLastFNHead) {
					pLastFNHead = pC + 1;
					continue;
				}
				--nFiles;
				if (fcsi.pDataEnd - pCurData < 4) {
					return 1; // invalid sentence
				}
				int fileLen = *((int*)pCurData);
				pCurData += sizeof(int);
				if (fcsi.pDataEnd - pCurData < fileLen) {
					return 1; // invalid sentence
				}
				std::string ttstr = std::string(pLastFNHead, pC);
				writeFileDir(pLastFNHead, pC - pLastFNHead,
					         0, pCurData, fileLen);
				pCurData += fileLen;
				pLastFNHead = pC + 1;
			}
			if (pC != fcsi.pParamEnd ||
				0 != nFiles || pCurData != fcsi.pDataEnd) {
				return 1; // invalid sentence
			} else {
				return 0;
			}
		} // end if "RESP:GetFile"

		else if (0x01 == fcsi.func) { // ListFile
			const char *pLastSeg = fcsi.pParamStart;
			const char *pC = pLastSeg + 1;
			for (; pC < fcsi.pParamEnd; ++pC) {
				if ('|' != *pC) {
					continue;
				}
				//std::string str = std::string(pLastSeg, pC);
				//std::cout << "str = " << str << std::endl;
				writeFileDir(pLastSeg, pC - pLastSeg, 0);
				++pC;
				pLastSeg = pC;
			}
			return 0;
		} // end if "RESP:ListFile"

		else if (0x02 == fcsi.func) { // ListDir
			const char *pLastSeg = fcsi.pParamStart;
			const char *pC = pLastSeg + 1;
			for (; pC < fcsi.pParamEnd; ++pC) {
				if ('|' != *pC) {
					continue;
				}
				writeFileDir(pLastSeg, pC - pLastSeg, 1);
				++pC;
				pLastSeg = pC;
			}
			return 0;
		} // end if "RESP:ListDir"
	}

	return -1;
}


FileCache::FileCache(FCType type, unsigned int expireTime,
	                 const char *workpath)
{
	m_pImp = new __IMPL;
	if (!m_pImp) {
		return;
	}
	m_pImp->m_bValid = false;
	m_pImp->m_FCType = type;
	m_pImp->m_expTime = expireTime;
	if (!workpath || '\0' == *workpath ||
		0 == strcmp(".", workpath) ||
		0 == strcmp("/", workpath) ||
		0 == strcmp("\\", workpath) ||
		0 == strcmp("./", workpath) ||
		0 == strcmp(".\\", workpath)) {
		m_pImp->m_workpath = std::string();
	} else {
		int slen = strlen(workpath);
		if (slen > 0 && ('/' == workpath[0] || '\\' == workpath[0])) {
			++workpath;
			--slen;
		}
		if (slen > 0 && ('\\' == workpath[slen - 1] ||
			'/' == workpath[slen - 1])) {
			--slen;
		}
		if (slen > 0) {
			m_pImp->m_workpath = std::string(workpath, slen) + '/';
			_createDir(m_pImp->m_workpath.c_str());
		} else {
			m_pImp->m_workpath = std::string();
		}
	}
	m_pImp->m_bValid = true;
}

const char* FileCache::GetWorkingPath() const
{
	if (m_pImp) {
		return m_pImp->m_workpath.c_str();
	}
	return nullptr;
}

FileCache::~FileCache()
{
	if (m_pImp) {
		delete m_pImp;
	}
}

bool FileCache::IsValid() const
{
	return (m_pImp && m_pImp->m_bValid);
}

int FileCache::Process(std::vector<char> &respVec,
	const char *pHead, int nLength,
	std::vector<char> *pMissingFileList,
	int waitTimeMilSec)
{
	if (IsValid()) {
		return m_pImp->Process(respVec, pHead, nLength, pMissingFileList, waitTimeMilSec);
	}
	return -1;
}

bool FileCache::Update()
{
	if (!IsValid()) {
		return false;
	}
	if (FC_READONLY == m_pImp->m_bValid) {
		return true;
	}
	TimedMutexLocker _tml(m_pImp->m_Process_lock, 0);
	if (_tml) {
		auto _tic = std::chrono::system_clock::now();
		I64 timediff = std::chrono::duration_cast
			<std::chrono::seconds>(_tic - m_pImp->m_tpLastUpdate).count();
		if (timediff >= (long long)((m_pImp->m_expTime) >> 4)) {
			_clearExpiredFiles(m_pImp->m_workpath.c_str(), m_pImp->m_expTime);
			m_pImp->m_tpLastUpdate = std::chrono::system_clock::now();
		}
		return true;
	}
	return false;
}

int FileCache::ReadFile(const char *pRelativeName,
	                    std::vector<char> &outVec)
{
	if (!IsValid()) {
		return -1;
	}
	if (!pRelativeName || '\0' == *pRelativeName) {
		return 1;
	}

	std::vector<char> vec;
	int lPos;
	if ('\0' == *(m_pImp->m_workpath.c_str())) {
		lPos = 0;
	} else {
		lPos = (int)m_pImp->m_workpath.length();
	}
	int nNameLen = strlen(pRelativeName);
	vec.resize(lPos + nNameLen + 1);
	if (lPos > 0) {
		memcpy(&vec[0], m_pImp->m_workpath.c_str(), lPos);
	}
	memcpy(&vec[lPos], pRelativeName, nNameLen);
	vec.back() = '\0';

	TimedMutexLocker tml(m_pImp->m_Process_lock, 10);
	if (!tml) {
		return 2;
	}
	std::ifstream ifs(&vec[0], std::ios::binary);
	if (ifs.is_open()) {
		ifs.seekg(0, std::ios::end);
		int fileLen = (int)ifs.tellg();
		ifs.seekg(0, std::ios::beg);
		if (fileLen > 0) {
			outVec.resize(fileLen);
			ifs.read(&outVec[0], fileLen);
			ifs.close();
			if (FC_READWRITE == m_pImp->m_FCType) {
				_makeTimeNow(&vec[0]);
			}
			return 0;
		}
	}
	return 1;
}

static void _append(std::vector<char> &vec, const char *pTxt)
{
	int slen = strlen(pTxt);
	int pos = (int)vec.size();
	vec.resize(pos + slen);
	memcpy(&vec[pos], pTxt, slen);
}

static void _append(std::vector<char> &vec, int nVal)
{
	int pos = (int)vec.size();
	vec.resize(pos + 4);
	memcpy(&vec[pos], &nVal, 4);
}

static void _append(std::vector<char> &vec, const void *pData, int nDataLen)
{
	if (nDataLen <= 0) { return; }
	int pos = (int)vec.size();
	vec.resize(pos + nDataLen);
	memcpy(&vec[pos], pData, nDataLen);
}


bool FileCache::FormOneSentence(std::vector<char> &retVec,
	                            FCProtocolType type, FCFunction func,
								const char *pParamHead, int nParamLen,
								const char *pFileDataHead, int nFileDataLength)
{
	if (!pParamHead || nParamLen <= 0) { return false; }
	if (FC_RESP == type && FC_GETFILE == func &&
		(!pFileDataHead || nFileDataLength <= 0)) {
		return false;
	}
	retVec.resize(0);
	_append(retVec, "<head><type>");
	if (FC_INQ == type) {
		_append(retVec, 3);
		_append(retVec, "INQ");
	} else if (FC_RESP == type) {
		_append(retVec, 4);
		_append(retVec, "RESP");
	} else {
		return false;
	}
	_append(retVec, "<function>");
	if (FC_GETFILE == func) {
		_append(retVec, 7);
		_append(retVec, "GetFile");
	} else if (FC_LISTFILE == func) {
		_append(retVec, 8);
		_append(retVec, "ListFile");
	} else if (FC_LISTDIR == func) {
		_append(retVec, 7);
		_append(retVec, "ListDir");
	} else {
		return false;
	}
	_append(retVec, "<param>");
	_append(retVec, nParamLen);
	_append(retVec, pParamHead, nParamLen);
	if (FC_RESP == type && FC_GETFILE == func) {
		_append(retVec, "<filedata>");
		_append(retVec, nFileDataLength);
		_append(retVec, pFileDataHead, nFileDataLength);
	}
	_append(retVec, "</head>");
	return true;
}

bool FileCache::AnalyzeSentence(const char *pSentence, int nSentenceLen,
	                            int *pType, int *pFunc,
								int *pParamOffset, int *pParamLen)
{
	FCSentenceInfo fcsi;
	if (!fcsi.ParseSentence(pSentence, nSentenceLen)) {
		return false;
	}
	if (pType) {
		*pType = fcsi.type;
	}
	if (pFunc) {
		*pFunc = fcsi.func;
	}
	if (pParamOffset) {
		*pParamOffset = fcsi.pParamStart - pSentence;
	}
	if (pParamLen) {
		*pParamLen = fcsi.pParamEnd - pSentence;
	}
	return true;
}

}