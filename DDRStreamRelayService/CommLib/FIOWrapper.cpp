#include "FIOWrapper.h"

//#define DEBUGGIN_INFO_DISP

#include <stdio.h>
#include <string.h>
//#include <stdlib.h>
#include <time.h>
#include <vector>
#ifdef DEBUGGIN_INFO_DISP
#include <iostream>
#endif
#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <io.h>
#include <direct.h>
#define WIN32_LEAN_AND_MEAN
#include "Windows.h"
#endif

#ifdef __linux__
#include <sys/time.h>
#include <unistd.h>
#include <utime.h>
#include <glob.h>
#include <fcntl.h>
#endif

#include "MsgStruct.h"

namespace DDRCommLib {

bool _createDir(const char *pDirName)
{
#ifdef _WIN32
	return (0 == _mkdir(pDirName));
#endif
#ifdef __linux__
	return (0 == mkdir(pDirName, 0));
#endif
}

bool _makeTimeNow(const char *pFileDirName)
{
#ifdef _WIN32
	HANDLE hFile = CreateFileA(pFileDirName,
		                       GENERIC_WRITE | GENERIC_READ,  //������GENERIC_READ���Բ��ܵõ�ʱ��
							   FILE_SHARE_READ,
							   NULL,
							   OPEN_EXISTING,
							   FILE_FLAG_BACKUP_SEMANTICS,
							   NULL);
	if (INVALID_HANDLE_VALUE == hFile) {
		return false;
	}
	FILETIME ft;
	time_t seconds;
	time(&seconds);
	LONGLONG ll = Int32x32To64(seconds, 10000000) + 116444736000000000;
	ft.dwLowDateTime = (DWORD)ll;
	ft.dwHighDateTime = (DWORD)(ll >> 32);
	::SetFileTime(hFile, NULL, &ft, &ft);
	::CloseHandle(hFile);
	return true;
#endif

#ifdef __linux__
	utimes(pFileDirName, nullptr);
	return true;
#endif
}

typedef struct
{
	std::vector<char> vec;
	int pos;
} __MissingStruct__;


long long _getNewMissingHandle()
{
	__MissingStruct__ *pNew = new __MissingStruct__;
	if (!pNew) {
		return 0;
	} else {
		pNew->pos = 0;
		return (long long)pNew;
	}
}

bool _rewindMissingEntry(long long missingHandle)
{
	if (missingHandle) {
		((__MissingStruct__*)missingHandle)->pos = 0;
		return true;
	}
	return false;
}

bool _findNextMissingEntry(long long missingHandle, const char **pNextEntry)
{
	if (missingHandle) {
		__MissingStruct__ *pS = (__MissingStruct__*)missingHandle;
		if (pS->pos < (int)pS->vec.size()) {
			*pNextEntry = &pS->vec[pS->pos];
			pS->pos += strlen(*pNextEntry) + 1;
			return true;
		}
	}
	return false;
}

void _closeMissingHandle(long long missingHandle)
{
	if (missingHandle) {
		delete (__MissingStruct__*)missingHandle;
	}
}

void __testMsgQ(void *pMsgQ)
{
	std::vector<std::vector<char>> tmpVecs;
	std::vector<char> vec;
	int x = GetNumMsg_MsgQ(pMsgQ);
	for (int i = 0; i < x; ++i) {
		int nlen = GetNextMsgLen_MsgQ(pMsgQ);
		vec.resize(nlen);
		PopMsg_MsgQ(pMsgQ, (nlen > 0 ? &vec[0] : nullptr), nlen);
		tmpVecs.emplace_back(vec);
		vec.emplace_back('\0');
		if (nlen > 0) {
			printf("Msg #%d: %s\n", i, &vec[0]);
		} else {
			printf("Msg #%d: (empty)\n", i);
		}
	}
	for (int i = 0; i < x; ++i) {
		if (tmpVecs[i].size() > 0) {
			Add_MsgQ(pMsgQ, &(tmpVecs[i])[0], (int)tmpVecs[i].size());
		} else {
			Add_MsgQ(pMsgQ, nullptr, 0);
		}
	}
}

typedef struct
{
	bool bUpdateAccT;

	int wrkPathLen;
	std::vector<char> searchTxt;
	const char *pLastLvStart;
	const char *pTxtEnd;
	void *pMsgQ;
	std::vector<char> resultName;
	std::vector<char> tmpVec;
	std::vector<char> tvec;

#ifdef _WIN32
	long hFileHandle;
#endif
#ifdef __linux__
	int bGlobValid;
	glob_t globbuf;
	int curPathC;
#endif
	long long msHandle;
	int curEntryStart;
	bool bCurEntryFoundAny;
} __FindDataStruct__;

long long _findFirstFileHandle(const char *pWorkingDir, const char *pInqName,
	                           const char **pFileDirName, bool *pbFile,
							   long long missingHandle, bool bUpdateAccessTime)
{
	if (!pInqName || '\0' == *pInqName) {
		return 0;
	}
	if ('/' == *pInqName || '\\' == *pInqName ||
		0 == strcmp(".", pWorkingDir)) {
		++pInqName;
	}
	int slen2 = strlen(pInqName);
	if (0 == slen2) {
		return 0;
	}
	if ('/' == pInqName[slen2 - 1] ||
		'\\' == pInqName[slen2 - 1]) {
		--slen2;
	}
	if (0 == slen2) {
		return 0;
	}
	int slen1;
	if (!pWorkingDir || '\0' == *pWorkingDir) {
		slen1 = 0;
	} else {
		if ('/' == *pWorkingDir || '\\' == *pWorkingDir) {
			++pWorkingDir;
		}
		slen1 = strlen(pWorkingDir);
		if (slen1 >= 2 && (0 == strncmp(".\\", pWorkingDir, 2) ||
			0 == strncmp("./", pWorkingDir, 2))) {
			pWorkingDir += 2;
			slen1 -= 2;
		}
		for (int i = 0; i < slen1; ++i) {
			if ('*' == pWorkingDir[i] || '?' == pWorkingDir[i]) {
				return 0;
			}
		}
		if (slen1 > 0) {
			if ('/' == pWorkingDir[0] || '\\' == pWorkingDir[0]) {
				return 0;
			}
			if ('\\' == pWorkingDir[slen1 - 1] ||
				'/' == pWorkingDir[slen1 - 1]) {
				--slen1;
			}
		}
	}

	__FindDataStruct__ *pFD = new __FindDataStruct__;
	if (!pFD) {
		return -1;
	}
#ifdef _WIN32
	pFD->hFileHandle = -1;
#endif
#ifdef __linux__
	pFD->bGlobValid = false;
#endif
	pFD->bUpdateAccT = bUpdateAccessTime;
	pFD->pMsgQ = 0;
	pFD->wrkPathLen = slen1 + (slen1 > 0 ? 1 : 0);
	pFD->searchTxt.resize(pFD->wrkPathLen + slen2 + 1);
	int pos = 0;
	if (slen1 > 0) {
		memcpy(&(pFD->searchTxt[0]), pWorkingDir, slen1);
		pos += slen1;
		pFD->searchTxt[pos++] = '/';
	}
	if (slen2 > 0) {
		memcpy(&(pFD->searchTxt[pos]), pInqName, slen2);
		pos += slen2;
	}
	for (int i = 0; i < pos; ++i) {
		if ('\\' == pFD->searchTxt[i]) {
			pFD->searchTxt[i] = '/';
		}
	}
	pFD->searchTxt[pos] = '\0';

	pFD->pMsgQ = Create_MsgQ();
	if (!pFD->pMsgQ) {
		delete pFD;
		return -1;
	}
	Add_MsgQ(pFD->pMsgQ, nullptr, 0);

	const char *pLvStart = &(pFD->searchTxt[pFD->wrkPathLen]);
	const char *pLvEnd;
	const char *const pEnd = &(pFD->searchTxt.back());

	pFD->tmpVec.resize(pFD->wrkPathLen);
	if (pFD->wrkPathLen > 0) {
		memcpy(&(pFD->tmpVec[0]), &(pFD->searchTxt[0]), pFD->wrkPathLen);
	}

	for (;; pLvStart = pLvEnd + 1) { // loop for each intermediate level
		for (pLvEnd = pLvStart; pLvEnd < pEnd && '/' != *pLvEnd; ++pLvEnd);
		if (pLvEnd >= pEnd) {
			break; // last level reached;
		}
		if (pLvEnd <= pLvStart) { // inq syntax error
			Destroy_MsgQ(pFD->pMsgQ);
			delete pFD;
			return 0;
		}
		// now [pLvStart, pLvEnd) forms current level of condition
		// e.g., "OneRoute*" inside of "XX/OneRoute*/Path*.txt".
		int nMsg = GetNumMsg_MsgQ(pFD->pMsgQ);
		if (0 == nMsg) { // no successful match from previous levels
			Destroy_MsgQ(pFD->pMsgQ);
			delete pFD;
			return -1;
		}
		// pop matches till last level, add current level of condition,
		// search, and push back new matches
		for (int i = 0; i < nMsg; ++i) {
			int nlen = GetNextMsgLen_MsgQ(pFD->pMsgQ);
			if (nlen < 0) { // unexpected error
				Destroy_MsgQ(pFD->pMsgQ);
				delete pFD;
				return -1;
			}
			pFD->tmpVec.resize(pFD->wrkPathLen + nlen + (pLvEnd - pLvStart) + 1);
			PopMsg_MsgQ(pFD->pMsgQ, &(pFD->tmpVec[pFD->wrkPathLen]), nlen);
			memcpy(&(pFD->tmpVec[pFD->wrkPathLen + nlen]), pLvStart,
				   pLvEnd - pLvStart);
			pFD->tmpVec.back() = '\0';
			if (bUpdateAccessTime) {
				if (pFD->wrkPathLen - 1 + nlen >= 0) {
					pFD->tmpVec[pFD->wrkPathLen - 1 + nlen] = '\0';
					_makeTimeNow(&(pFD->tmpVec[0]));
					pFD->tmpVec[pFD->wrkPathLen - 1 + nlen] = '/';
				}
			}

			bool bFoundAny = false;
#ifdef _WIN32
			struct _finddata_t fileinfo;
			long hFile = _findfirst(&pFD->tmpVec[0], &fileinfo);
			if (-1 != hFile) {
				do {
					if (strcmp(fileinfo.name, ".") == 0 ||
						strcmp(fileinfo.name, "..") == 0) {
						continue;
					}
					if ((fileinfo.attrib & _A_SUBDIR) != 0) {
						bFoundAny = true;
						int dirlen = strlen(fileinfo.name);
						pFD->tvec.resize(nlen + dirlen + 1);
						if (nlen > 0) {
							memcpy(&(pFD->tvec[0]), &(pFD->tmpVec[pFD->wrkPathLen]), nlen);
						}
						memcpy(&(pFD->tvec[nlen]), fileinfo.name, dirlen);
						pFD->tvec.back() = '/';
						Add_MsgQ(pFD->pMsgQ, &(pFD->tvec[0]), (int)pFD->tvec.size());
					}
				} while (0 == _findnext(hFile, &fileinfo));
				_findclose(hFile);
			}
#endif
#ifdef __linux__
			glob_t globbuf;
			glob(&pFD->tmpVec[0], 0, NULL, &globbuf);
			for (int i = 0; i < (int)globbuf.gl_pathc; ++i) {
				int fuLen = strlen(globbuf.gl_pathv[i]);
				if (fuLen <= pFD->wrkPathLen) {
					continue;
				}
				if (0 == strcmp(".", globbuf.gl_pathv[i]) ||
					0 == strcmp("..", globbuf.gl_pathv[i]) ||
					(fuLen > 2 &&
					0 == strncmp("/.", globbuf.gl_pathv[i] + fuLen - 2, 2)) ||
					(fuLen > 3 &&
					0 == strncmp("/..", globbuf.gl_pathv[i] + fuLen - 3, 3))) {
					continue;
				}
				struct stat sb;
				if (stat(globbuf.gl_pathv[i], &sb) < 0) {
					continue;
				}
				if ((sb.st_mode & S_IFMT) != S_IFDIR) {
					continue;
				}
				bFoundAny = true;
				Combine_MsgQ(pFD->pMsgQ,
					         globbuf.gl_pathv[i] + pFD->wrkPathLen,
							 fuLen - pFD->wrkPathLen,
							 "/", 1);
			}
			globfree(&globbuf);
#endif
			if (!bFoundAny && missingHandle != 0) {
				std::vector<char> &vv = ((__MissingStruct__*)missingHandle)->vec;
				pos = vv.size();
				vv.resize(pos + nlen + pEnd - pLvStart + 1);
				if (nlen > 0) {
					memcpy(&vv[pos], &(pFD->tmpVec[pFD->wrkPathLen]), nlen);
				}
				memcpy(&vv[pos + nlen], pLvStart, pEnd - pLvStart);
				vv.back() = '\0';
			}
		} // end for each pop-ed (last level) entry
	} // end for (each intermediate level)

	// now only last level pending
	pFD->pLastLvStart = pLvStart;
	pFD->pTxtEnd = pEnd;
	pFD->msHandle = missingHandle;
#if 0
	__testMsgQ(pFD->pMsgQ);
#endif

	if (_findNextFile((long long)pFD, pFileDirName, pbFile)) {
		return (long long)pFD;
	} else {
		_closeFindHandle((long long)pFD);
		return -1;
	}
}

bool _findNextFile(long long handle, const char **pFileDirName, bool *pbFile)
{
	__FindDataStruct__ *pFD = (__FindDataStruct__*)handle;
	if (!pFD || !pFD->pMsgQ) {
		return false;
	}

#ifdef _WIN32
	std::vector<char> tmpVec;
#endif
	while (1) {
		bool bFound = false;
#ifdef _WIN32
		struct _finddata_t fileinfo;
		if (-1 != pFD->hFileHandle) {
			if (0 == _findnext(pFD->hFileHandle, &fileinfo)) {
				if (strcmp(fileinfo.name, ".") == 0 ||
					strcmp(fileinfo.name, "..") == 0) {
					continue;
				}
				bFound = true;
				pFD->bCurEntryFoundAny = true;
			} else {
				_findclose(pFD->hFileHandle);
				pFD->hFileHandle = -1;
			}
#endif
#ifdef __linux__
		struct stat sb;
		if (pFD->bGlobValid) {
			if (++(pFD->curPathC) < (int)pFD->globbuf.gl_pathc) {
				int fuLen = strlen(pFD->globbuf.gl_pathv[pFD->curPathC]);
				if (fuLen <= pFD->wrkPathLen) {
					continue;
				}
				if (0 == strcmp(".", pFD->globbuf.gl_pathv[pFD->curPathC]) ||
					0 == strcmp("..", pFD->globbuf.gl_pathv[pFD->curPathC]) ||
					(fuLen > 2 &&
					0 == strncmp("/.", pFD->globbuf.gl_pathv[pFD->curPathC] + fuLen - 2, 2)) ||
					(fuLen > 3 &&
					0 == strncmp("/..", pFD->globbuf.gl_pathv[pFD->curPathC] + fuLen - 3, 3))) {
					continue;
				}
				if (stat(pFD->globbuf.gl_pathv[pFD->curPathC], &sb) < 0) {
					continue;
				}
				bFound = true;
				pFD->bCurEntryFoundAny = true;
			} else {
				globfree(&pFD->globbuf);
				pFD->bGlobValid = false;
			}
#endif
		} else {
			// no active handle
			int nlen = GetNextMsgLen_MsgQ(pFD->pMsgQ);
			if (nlen < 0) {
				return false;
			}
			pFD->tmpVec.resize(pFD->wrkPathLen + nlen + (pFD->pTxtEnd - pFD->pLastLvStart) + 1);
			PopMsg_MsgQ(pFD->pMsgQ, &(pFD->tmpVec[pFD->wrkPathLen]), nlen);
			memcpy(&(pFD->tmpVec[pFD->wrkPathLen + nlen]), pFD->pLastLvStart,
				   pFD->pTxtEnd - pFD->pLastLvStart);
			pFD->tmpVec.back() = '\0';
			pFD->bCurEntryFoundAny = false;
			pFD->curEntryStart = nlen;
#ifdef _WIN32
			pFD->hFileHandle = _findfirst(&pFD->tmpVec[0], &fileinfo);
			if (-1 != pFD->hFileHandle) {
				do {
					if (strcmp(fileinfo.name, ".") == 0 ||
						strcmp(fileinfo.name, "..") == 0) {
						continue;
					}
					bFound = true;
					pFD->bCurEntryFoundAny = true;
					break;
				} while (0 == _findnext(pFD->hFileHandle, &fileinfo));
#endif
#ifdef __linux__
            pFD->bGlobValid = (0 == glob(&pFD->tmpVec[0], 0, NULL, &(pFD->globbuf)));
            //pFD->bGlobValid = (0 == glob("tmp*/tmp*/*/X*", 0, NULL, &(pFD->globbuf)));
            if (pFD->bGlobValid) {
                if (pFD->globbuf.gl_pathc > 0) {
                    pFD->curPathC = 0;
                    do {
                        int fuLen = strlen(pFD->globbuf.gl_pathv[pFD->curPathC]);
                        if (fuLen <= pFD->wrkPathLen) {
                            continue;
                        }
                        if (0 == strcmp(".", pFD->globbuf.gl_pathv[pFD->curPathC]) ||
                            0 == strcmp("..", pFD->globbuf.gl_pathv[pFD->curPathC]) ||
                            (fuLen > 2 &&
                            0 == strncmp("/.", pFD->globbuf.gl_pathv[pFD->curPathC] + fuLen - 2, 2)) ||
                            (fuLen > 3 &&
                            0 == strncmp("/..", pFD->globbuf.gl_pathv[pFD->curPathC] + fuLen - 3, 3))) {
                            continue;
                        }
                        if (stat(pFD->globbuf.gl_pathv[pFD->curPathC], &sb) >= 0) {
                            bFound = true;
                            pFD->bCurEntryFoundAny = true;
                            break;
                        }
                    } while (++(pFD->curPathC) < (int)pFD->globbuf.gl_pathc);
                    if (!bFound) {
                        globfree(&pFD->globbuf);
                        pFD->bGlobValid = false;
                    }
                }
#endif
            }
		}

		if (!pFD->bCurEntryFoundAny && pFD->msHandle != 0) {
			std::vector<char> &vv = ((__MissingStruct__*)pFD->msHandle)->vec;
			int pos = vv.size();
			vv.resize(pos + pFD->curEntryStart + pFD->pTxtEnd - pFD->pLastLvStart + 1);
			if (pFD->curEntryStart > 0) {
				memcpy(&vv[pos], &(pFD->tmpVec[pFD->wrkPathLen]), pFD->curEntryStart);
			}
			memcpy(&vv[pos + pFD->curEntryStart], pFD->pLastLvStart, pFD->pTxtEnd - pFD->pLastLvStart);
			vv.back() = '\0';
		}
		if (bFound) {
#ifdef _WIN32
			int fnLen = strlen(fileinfo.name);
			pFD->resultName.resize(pFD->curEntryStart + fnLen + 1);
			if (pFD->curEntryStart > 0) {
				memcpy(&(pFD->resultName[0]), &(pFD->tmpVec[pFD->wrkPathLen]), pFD->curEntryStart);
			}
			memcpy(&(pFD->resultName[pFD->curEntryStart]), fileinfo.name, fnLen);
			pFD->resultName.back() = '\0';
			if (pFD->bUpdateAccT) {
				tmpVec.resize(pFD->wrkPathLen + pFD->curEntryStart + fnLen + 1);
				memcpy(&tmpVec[0], &(pFD->tmpVec[0]), pFD->wrkPathLen + pFD->curEntryStart);
				memcpy(&tmpVec[pFD->wrkPathLen + pFD->curEntryStart], fileinfo.name, fnLen);
				tmpVec.back() = '\0';
				_makeTimeNow(&tmpVec[0]);
			}
#endif
#ifdef __linux__
            int fnLen = strlen(pFD->globbuf.gl_pathv[pFD->curPathC]);
			pFD->resultName.resize(fnLen - pFD->wrkPathLen + 1);
			memcpy(&(pFD->resultName[0]),
			       pFD->globbuf.gl_pathv[pFD->curPathC] + pFD->wrkPathLen,
			       fnLen - pFD->wrkPathLen);
			pFD->resultName.back() = '\0';
			if (pFD->bUpdateAccT) {
				_makeTimeNow(pFD->globbuf.gl_pathv[pFD->curPathC]);
			}
#endif
			if (pFileDirName) {
				*pFileDirName = &(pFD->resultName[0]);
#if 1 // make sure resultName stores a valid, non-segemented string
				for (int i = 0; i < (int)pFD->resultName.size() - 1; ++i) {
					if ('\0' == pFD->resultName[i]) {
						int y = 3 * 8;
						int x = 5 / (25 - y - 1);
					}
				}
#endif
			}
			if (pbFile) {
#ifdef _WIN32
				*pbFile = ((fileinfo.attrib & _A_SUBDIR) == 0);
#endif
#ifdef __linux__
				*pbFile = ((sb.st_mode & S_IFMT) != S_IFDIR);
#endif
			}
#ifdef DEBUGGIN_INFO_DISP
			std::cout << "name = '" << *pFileDirName << "'\n";
#endif
			return true;
		}
	}
}

void _closeFindHandle(long long handle)
{
	__FindDataStruct__ *pFD = (__FindDataStruct__*)handle;
	if (pFD) {
		if (pFD->pMsgQ) {
			Destroy_MsgQ(pFD->pMsgQ);
		}
#ifdef _WIN32
		if (-1 != pFD->hFileHandle) {
			_findclose(pFD->hFileHandle);
		}
#endif
#ifdef __linux__
		if (pFD->bGlobValid) {
			globfree(&pFD->globbuf);
		}
#endif
		delete pFD;
	}
}


int _deleteFile(const char *pFileName)
{
	return remove(pFileName);
}

int _deleteDir(const char *pDirName)
{
	void *pMsgSt = Create_MsgSt();
	if (!pMsgSt) {
		return -1;
	}
	int slen = strlen(pDirName);
	char ttt = (char)0x00; // not expanded yet
	Combine_MsgSt(pMsgSt, &ttt, 1, pDirName, slen);
	std::vector<char> vec;

	while (GetNumMsg_MsgSt(pMsgSt) > 0) {
		int nlen = GetNextMsgLen_MsgSt(pMsgSt);
		if (nlen <= 0) {
			break;
		}
		vec.resize(nlen);
		PopMsg_MsgSt(pMsgSt, &vec[0], nlen);
		if ((char)0x01 == vec[0]) { // already expanded
			vec.emplace_back('\0');
#ifdef DEBUGGIN_INFO_DISP
			std::cout << "Deleting folder (EXPANDED) '" << &vec[1] << "'\n";
#endif
#ifdef _WIN32
			_rmdir(&vec[1]);
#endif
#ifdef __linux__
			rmdir(&vec[1]);
#endif
			continue;
		}
		// expanding
		vec.resize(nlen + 3);
		memcpy(&vec[nlen], "/*\0", 3);
#ifdef DEBUGGIN_INFO_DISP
		std::cout << "Expanding '" << &vec[1] << "'\n";
#endif
		bool bAnySubFolder = false;
#ifdef _WIN32
		struct _finddata_t fileinfo;
		long hFile = _findfirst(&vec[1], &fileinfo);
		vec.resize(nlen + 1);
		if (-1 != hFile) {
			do {
				if (0 == strcmp(fileinfo.name, ".") ||
					0 == strcmp(fileinfo.name, "..")) {
					continue;
				}
				if (0 == (fileinfo.attrib & _A_SUBDIR)) { // file
					int pos = (int)vec.size();
					int fnLen = strlen(fileinfo.name);
					vec.resize(pos + fnLen + 1);
					memcpy(&vec[pos], fileinfo.name, fnLen + 1);
#ifdef DEBUGGIN_INFO_DISP
					std::cout << "Deleting file '" << &vec[1] << "'\n";
#endif
					remove(&vec[1]);
					vec.resize(pos);
				} else { // sub folder
					if (!bAnySubFolder) {
						// push back current folder as expanded
						vec[0] = (char)0x01;
						Add_MsgSt(pMsgSt, &vec[0], nlen);
						bAnySubFolder = true;
					}
					int pos = (int)vec.size();
					int fnLen = strlen(fileinfo.name);
					vec.resize(pos + fnLen);
					memcpy(&vec[pos], fileinfo.name, fnLen);
					vec[0] = (char)0x00; // not expanded yet
#ifdef DEBUGGIN_INFO_DISP
					vec.emplace_back('\0');
					std::cout << "Adding sub-folder '" << &vec[1] << "'\n";
					vec.pop_back();
#endif
					Add_MsgSt(pMsgSt, &vec[0], (int)vec.size());
					vec.resize(pos);
				}
			} while (0 == _findnext(hFile, &fileinfo));
		}
		_findclose(hFile);
#endif
#ifdef __linux__
		glob_t globbuf;
		glob(&vec[1], 0, NULL, &globbuf);
		vec.resize(nlen + 1);
		if (globbuf.gl_pathc > 0) {
			for (int i = 0; i < (int)globbuf.gl_pathc; ++i) {
				int fuLen = strlen(globbuf.gl_pathv[i]);
				if (0 == strcmp(".", globbuf.gl_pathv[i]) ||
					0 == strcmp("..", globbuf.gl_pathv[i]) ||
					(fuLen > 2 &&
					0 == strncmp("/.", globbuf.gl_pathv[i] + fuLen - 2, 2)) ||
					(fuLen > 3 &&
					0 == strncmp("/..", globbuf.gl_pathv[i] + fuLen - 3, 3))) {
					continue;
				}
				struct stat sb;
				if (stat(globbuf.gl_pathv[i], &sb) < 0) {
					continue;
				}
				if ((sb.st_mode & S_IFMT) != S_IFDIR) { // regular file
#ifdef DEBUGGIN_INFO_DISP
					std::cout << "Deleting file '" << globbuf.gl_pathv[i] << "'\n";
#endif
					remove(globbuf.gl_pathv[i]);
				} else {
					if (!bAnySubFolder) {
						// push back current folder as expanded
						vec[0] = (char)0x01;
#ifdef DEBUGGIN_INFO_DISP
						vec.emplace_back('\0');
						std::cout << "Adding back cur-folder '" << &vec[1] << "'\n";
						vec.pop_back();
#endif
						Add_MsgSt(pMsgSt, &vec[0], nlen);
						bAnySubFolder = true;
					}
#ifdef DEBUGGIN_INFO_DISP
					std::cout << "Adding sub-folder '" << globbuf.gl_pathv[i] << "'\n";
#endif
					ttt = (char)0x00;
					Combine_MsgSt(pMsgSt, &ttt, 1, globbuf.gl_pathv[i], strlen(globbuf.gl_pathv[i]));
				}
			}
		}
		globfree(&globbuf);
#endif

		if (!bAnySubFolder) { // current folder is empty
			vec.back() = '\0';
#ifdef DEBUGGIN_INFO_DISP
			std::cout << "Deleting folder (EMPTY) '" << &vec[1] << "'\n";
#endif
#ifdef _WIN32
			_rmdir(&vec[1]);
#endif
#ifdef __linux__
			rmdir(&vec[1]);
#endif
			continue;
		}
	}

	Destroy_MsgSt(pMsgSt);
#ifdef _WIN32
	return (-1 == _access(pDirName, 0) ? 0 : (-1));
#endif
#ifdef __linux__
	return (-1 == access(pDirName, F_OK) ? 0 : (-1));
#endif
}

int _deleteFileDir(const char *pWorkingDir, const char *pName)
{
	if (!pName || '\0' == *pName) {
		return 1;
	}
	if ('/' == *pName || '\\' == *pName) {
		++pName;
	}
	int slen2 = strlen(pName);
	if (0 == slen2) {
		return 1;
	}
	if ('/' == pName[slen2 - 1] ||
		'\\' == pName[slen2 - 1]) {
		--slen2;
	}
	if (0 == slen2) {
		return 1;
	}
	int slen1;
	if (!pWorkingDir || '\0' == *pWorkingDir) {
		slen1 = 0;
	} else {
		if ('/' == *pWorkingDir || '\\' == *pWorkingDir ||
			0 == strcmp(".", pWorkingDir)) {
			++pWorkingDir;
		}
		slen1 = strlen(pWorkingDir);
		if (slen1 >= 2 && (0 == strncmp(".\\", pWorkingDir, 2) ||
			0 == strncmp("./", pWorkingDir, 2))) {
			pWorkingDir += 2;
			slen1 -= 2;
		}
		if (slen1 > 0) {
			if ('/' == pWorkingDir[0] || '\\' == pWorkingDir[0]) {
				return 1;
			}
			if ('\\' == pWorkingDir[slen1 - 1] ||
				'/' == pWorkingDir[slen1 - 1]) {
				--slen1;
			}
		}
	}
	char *pFullName = new char[slen1 + (slen1 > 0 ? 1 : 0) + slen2 + 1];
	int pos = 0;
	if (slen1 > 0) {
		memcpy(pFullName, pWorkingDir, slen1);
		pFullName[slen1] = '/';
		pos += slen1 + 1;
	}
	memcpy(pFullName, pName, slen2);
	pos += slen2;
	pFullName[pos] = '\0';
	for (const char *pC = pFullName; *pC != '\0'; ++pC) {
		if ('*' == *pC || '?' == *pC) {
			delete pFullName;
			return 1;
		}
	}

	int ret;
#ifdef _WIN32
	DWORD attrib = ::GetFileAttributesA(pFullName);
	if (-1 == attrib) {
		delete pFullName;
		return -1;
	}
	if (attrib | FILE_ATTRIBUTE_DIRECTORY) {
		ret = _deleteDir(pFullName);
	} else {
		ret = remove(pFullName);
	}
#endif
#ifdef __linux__
	struct stat statbuf;
	if (stat(pFullName, &statbuf) < 0) {
		delete pFullName;
		return -1;
	}
	if ((statbuf.st_mode & S_IFMT) == S_IFDIR) { // dir
		_deleteDir(pFullName);
		ret = (-1 == access(pFullName, F_OK) ? 0 : (-1));
	} else {
		ret = remove(pFullName);
	}
#endif

	delete pFullName;
	return ret;
}

I64 _getFileTime(const char *pFileDirName)
{
	struct stat result;
	if (0 == stat(pFileDirName, &result)) {
		return (I64)result.st_mtime;
	}
	return -1;
}

int _clearExpiredFiles(const char *pWorkingDir, int nMaxValidTimeBack)
{
	time_t seconds;
	time(&seconds);
	seconds -= nMaxValidTimeBack;
	void *pMsgSt = Create_MsgSt();
	if (!pMsgSt) {
		return -1;
	}
	std::vector<char> vec;
	if (!pWorkingDir ||
		0 == strcmp(".", pWorkingDir) ||
		0 == strcmp("/", pWorkingDir) ||
		0 == strcmp("\\", pWorkingDir) ||
		0 == strcmp("./", pWorkingDir) ||
		0 == strcmp(".\\", pWorkingDir)) {
		vec.resize(2);
		memcpy(&vec[1], "*\0", 2);
	} else {
		int slen = strlen(pWorkingDir);
		if ('/' == pWorkingDir[slen - 1] ||
			'\\' == pWorkingDir[slen - 1]) {
			--slen;
		}
		vec.resize(slen + 1 + 2);
		if (slen > 0) {
			memcpy(&vec[0], pWorkingDir, slen);
		}
		memcpy(&vec[slen], "/*\0", 3);
	}
	Add_MsgSt(pMsgSt, &vec[0], (int)vec.size());

	int ret = 0;
	while (GetNumMsg_MsgSt(pMsgSt) > 0) {
		int nlen = GetNextMsgLen_MsgSt(pMsgSt);
		vec.resize(nlen);
		if (PopMsg_MsgSt(pMsgSt, (vec.size() > 0 ? &vec[0] : nullptr), nlen) < 2) {
			ret = -1;
			break;
		}
#ifdef __linux__
		glob_t globbuf;
		glob(&vec[0], 0, NULL, &globbuf);
		if (globbuf.gl_pathc > 0) {
			for (int i = 0; i < (int)globbuf.gl_pathc; ++i) {
				int fuLen = strlen(globbuf.gl_pathv[i]);
				if (0 == strcmp(".", globbuf.gl_pathv[i]) ||
					0 == strcmp("..", globbuf.gl_pathv[i]) ||
					(fuLen > 2 &&
					0 == strncmp("/.", globbuf.gl_pathv[i] + fuLen - 2, 2)) ||
					(fuLen > 3 &&
					0 == strncmp("/..", globbuf.gl_pathv[i] + fuLen - 3, 3))) {
					continue;
				}
				struct stat sb;
				if (stat(globbuf.gl_pathv[i], &sb) < 0) {
					continue;
				}
				if ((sb.st_mode & S_IFMT) == S_IFDIR) { // folder
					I64 _tt = _getFileTime(globbuf.gl_pathv[i]);
					if (-1 == _tt) {
						continue;
					}
					if (_tt < seconds) {
						_deleteDir(globbuf.gl_pathv[i]);
					} else {
						vec.resize(fuLen + 3);
						memcpy(&vec[0], globbuf.gl_pathv[i], fuLen);
						memcpy(&vec[fuLen], "/*\0", 3);
						Add_MsgSt(pMsgSt, &vec[0], (int)vec.size());
					}
				} else { // regular file
					I64 _tt = _getFileTime(globbuf.gl_pathv[i]);
					if (-1 == _tt) {
						continue;
					}
					if (_tt < seconds) {
						remove(globbuf.gl_pathv[i]);
					}
				}
			}
		}
		globfree(&globbuf);
#endif
#ifdef _WIN32
		struct _finddata_t fileinfo;
		long hFile = _findfirst(&vec[0], &fileinfo);
		if (-1 != hFile) {
			do {
				if (strcmp(fileinfo.name, ".") == 0 ||
					strcmp(fileinfo.name, "..") == 0) {
					continue;
				}
				if ((fileinfo.attrib & _A_SUBDIR) != 0) { // folder
					int dirlen = strlen(fileinfo.name);
					vec.resize(nlen - 2 + dirlen + 1);
					memcpy(&vec[nlen - 2], fileinfo.name, dirlen + 1);
					I64 _tt = _getFileTime(&vec[0]);
					if (-1 == _tt) {
						continue;
					}
					if (_tt < seconds) {
#ifdef DEBUGGIN_INFO_DISP
						std::cout << "Deleting folder '" << &vec[0] << "'\n";
#endif
						_deleteDir(&vec[0]);
					} else {
						vec.back() = '/';
						vec.emplace_back('*');
						vec.emplace_back('\0');
						Add_MsgSt(pMsgSt, &vec[0], (int)vec.size());
					}
				} else { // regular file
					int fnlen = strlen(fileinfo.name);
					vec.resize(nlen - 2 + fnlen + 1);
					memcpy(&vec[nlen - 2], fileinfo.name, fnlen + 1);
					I64 _tt = _getFileTime(&vec[0]);
					if (-1 == _tt) {
						continue;
					}
					if (_tt < seconds) {
#ifdef DEBUGGIN_INFO_DISP
						std::cout << "Deleting file '" << &vec[0] << "'\n";
#endif
						remove(&vec[0]);
					}
				}
			} while (0 == _findnext(hFile, &fileinfo));
			_findclose(hFile);
		}
#endif
	}

	Destroy_MsgSt(pMsgSt);
	return ret;
}

bool _changeFolder2Current()
{
	char fullName[256];
#ifdef _WIN32
	int bytes = ::GetModuleFileNameA(nullptr, fullName, 255);
	if (0 == bytes) {
		return false;
	}
#endif
#ifdef __linux__
	char szTmp[32];
	sprintf(szTmp, "/proc/%d/exe", getpid());
	int bytes = readlink(szTmp, fullName, 255);
	if (bytes > 254) {
		bytes = 254;
	}
	if (bytes > 0) {
		fullName[bytes] = '\0';
	} else {
		return false;
	}
#endif
	for (int i = bytes; i >= 0; --i) {
		if ('/' == fullName[i] ||
			'\\' == fullName[i]) {
			fullName[i] = '\0';
			break;
		}
	}
#ifdef _WIN32
	return (::SetCurrentDirectoryA(fullName) != 0);
#endif
#ifdef __linux__
	return (0 == chdir(fullName));
#endif
}


}
