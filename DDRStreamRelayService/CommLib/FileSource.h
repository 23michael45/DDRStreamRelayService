// Read only file source
#ifndef __DDR_COMMLIB_FILE_SOURCE_H_INCLUDED__
#define __DDR_COMMLIB_FILE_SOURCE_H_INCLUDED__

#include "FileCache.h"

namespace DDRCommLib {

class FileSource {
public:
	FileSource(const char *pWorkingDirName = nullptr);
	const char* GetWorkingPath() const;
	bool IsValid() const;

	/* return value:
	0 - success, outVec containing a RESP sentence
	-1 - unexpected exception
	1 - sentence invalid
	2 - INQ file / folder name illegal
	3 - INQ:list file / folder not existing
	4 - access denied;
	5 - busy
	NOT THREAD-SAFE THUS USE IT IN "OFF-LINE" LIKE MODE */
	int GenerateDirSentence(const char *pDirName, // dir name that can include multiple levels of wildcards (e.g, "OneRoute*\\Path*.txt")
		                    std::vector<char> &outVec);

	/* return value:
	0 - success, outVec containing a RESP sentence
	-1 - unexpected exception
	1 - sentence invalid
	2 - INQ file / folder name illegal
	3 - INQ:list file / folder not existing
	4 - access denied;
	5 - busy
	NOT THREAD-SAFE THUS USE IT IN "OFF-LINE" LIKE MODE */
	int GenerateFileSentence(const char *pFileName, // file name that can include multiple levels of wildcards (e.g, "OneRoute*\\Path*.txt")
		                     std::vector<char> &outVec);

	int Respond2Sentence(const char *pSentence, int nSentenceLen,
		                 std::vector<char> &outVec);

protected:
	FileCache m_FC;
};

}

#endif // __DDR_COMMLIB_FILE_SOURCE_H_INCLUDED__
