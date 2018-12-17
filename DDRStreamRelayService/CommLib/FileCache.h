/*
File Cache with structured inquiry/response sentences and time activenss check.
Can be integrated to file transmission system.

- First created by Jojo and Jerry in Sep. 2018.
- First release in Oct. 2018.
- First revised version in Nov. 14, 2018.
*/

#ifndef __DDR_COMMLIB_FILE_CACHE_H_INCLUDED__
#define __DDR_COMMLIB_FILE_CACHE_H_INCLUDED__

#include <vector>

/*
[LEN] - 4 bytes (little endian) integer of length

<head>
<type>[3]INQ
<function>[7]ListDir
<param>[10]OneRoute_*
</head>

<head>
<type>[4]RESP
<function>[7]ListDir
<param>[33]OneRoute_1|OneRoute_2|OneRoute_3|
</head>

<head>
<type>[4]RESP
<function>[7]GetFile
<param>[47]OneRoute_1\\autopath.txt|OneRoute_2\\path_01.txt|
<filedata>[TotalBytes][2(# of files)][File#0Len](binData#0)[File#1Len](binData#1)
</head>
*/

namespace DDRCommLib {
	
enum FCType
{
	FC_READONLY,
	FC_READWRITE
};

enum FCProtocolType
{
	FC_INQ,
	FC_RESP
};

enum FCFunction
{
	FC_GETFILE,
	FC_LISTFILE,
	FC_LISTDIR
};

class FileCache
{
public:
	/*
	Input:
	- type:        0 - ReadOnly; 1 - ReadWrite (as an intermediate cacher).
	- expire time: For ReadWrite FC, files and directories will expire in
	               certain period of time, and calling Update() will cause
				   those expired files and directories to be deleted. In
				   SECONDS.
	- workPath:    The FileCache will not operate files beyond this level
	               specified by workPath. The default (null) setting will
				   be the current working directory of the calling process.
				   Note workPath can only be set once through the whole
				   lifetime of an FC object.
	*/
	FileCache(FCType type, unsigned int expireTime,
		      const char *workpath = nullptr);

	const char* GetWorkingPath() const;

	~FileCache();

	// return if type/expiration time/workpath are all correctly configured
	bool IsValid() const;

	/*
	Process a formatted sentence.
	Input:
	- respVec:          response only to INQ sentence.
	- pHead, nLength:   combined to specify a sentence. Starting from pHead
	                    with length nLength;
	- pMissingFileList: only apply for INQ:GetFile sentence. For successful
	                    operation, pMissingFileList will be filled with names
	                    of missing files (existing but empty), separated by
						'|'s.
	- waitTimeMilSec:   max time to wait to acquire a mutex, in milliseconds.
	Retur value:
	0  - sentence successfully processed
	-1 - object invalid or unexpected exception
	1  - sentence invalid
	2  - INQ file/folder name illegal
	3  - INQ:list file/folder not existing
	4  - access denied;
	5  - busy
	*pMissingFileList: only valid for INQ:GetFile, containes a list of
	files that are not availalbe (separated by '|')	*/
	int Process(std::vector<char> &respVec,
		        const char *pHead, int nLength,
				std::vector<char> *pMissingFileList = nullptr,
				int waitTimeMilSec = 50);

	//Clear the expired files and folders in the working directory.
	bool Update();

	// return value:
	// 0 - successful
	// -1 - object invalid
	// 1 - file not existing
	// 2 - busy
	int ReadFile(const char *pRelativeName,
		         std::vector<char> &outVec);

	/*
	type: 0 - INQ; 1 - RESP
	function: 0 - GetFile; 1 - ListFile; 2 - ListDir
	*/
	static bool FormOneSentence(std::vector<char> &retVec,
		                        FCProtocolType type, FCFunction func,
								const char *pParamHead, int nParamLen,
								const char *pFileDataHead = nullptr, int nFileDataLength = 0);

	/*
	Validating a sentence, extracting type, func, and param info.
	*/
	static bool AnalyzeSentence(const char *pSentence, int nSentenceLen,
		                        int *pType, int *pFunc,
								int *pParamOffset = nullptr,
								int *pParamLen = nullptr);

private:
	struct __IMPL;
	struct __IMPL *m_pImp;
};

}

#endif // __DDR_COMMLIB_FILE_CACHE_H_INCLUDED__
