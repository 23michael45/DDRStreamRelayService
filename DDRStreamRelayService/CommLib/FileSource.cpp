#include "FileSource.h"

#include <string.h>
#include <vector>

namespace DDRCommLib {

FileSource::FileSource(const char *pWorkingDirName)
	: m_FC(FC_READONLY, 0, pWorkingDirName) {}

const char* FileSource::GetWorkingPath() const
{
	return m_FC.GetWorkingPath();
}

bool FileSource::IsValid() const
{
	return m_FC.IsValid();
}

int FileSource::GenerateDirSentence(const char *pDirName,
	                                std::vector<char> &outVec)
{
	std::vector<char> tvec;
	FileCache::FormOneSentence(tvec, FC_INQ, FC_LISTFILE,
		                       pDirName, strlen(pDirName));
	return m_FC.Process(outVec, &tvec[0], (int)tvec.size());
}

int FileSource::GenerateFileSentence(const char *pFileName,
	std::vector<char> &outVec)
{
	std::vector<char> tvec;
	FileCache::FormOneSentence(tvec, FC_INQ, FC_GETFILE,
		                       pFileName, strlen(pFileName));
	return m_FC.Process(outVec, &tvec[0], (int)tvec.size());
}

int FileSource::Respond2Sentence(const char *pSentence, int nSentenceLen,
	                             std::vector<char> &outVec)
{
	int type, func;
	FileCache::AnalyzeSentence(pSentence, nSentenceLen,
		                       &type, &func);
	if (0 != type) {
		return 4;
	}
	return m_FC.Process(outVec, pSentence, nSentenceLen, nullptr, -1);
}

}