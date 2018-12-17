#ifndef __DDRCOMMLIB_CROSSPLATFORM_FILE_IO_OPERATIONS_H_INCLUDED__
#define __DDRCOMMLIB_CROSSPLATFORM_FILE_IO_OPERATIONS_H_INCLUDED__

#ifndef I64
typedef long long I64;
#endif

namespace DDRCommLib {

bool _createDir(const char *pDirName);

/* Set modification of file specified by pFileDirName
to now! */
bool _makeTimeNow(const char *pFileDirName);

/* Return a non-zero handle on success. */
long long _getNewMissingHandle();
/* Relocate the pointer to the head. */
bool _rewindMissingEntry(long long missingHandle);
/* Try to find next file/dir in the missing handle. */
bool _findNextMissingEntry(long long missingHandle, const char **pNextEntry);

void _closeMissingHandle(long long missingHandle);

/*
Find first file that fits the condition and return a finder handle
Input:
pWorkingDir -   Working directory. Can be NULL to specify current directory.
pInqName -      Name of file/dir. Can be used with multiple levels of
                wildcards. E.g, "OneRoute*\\Path*.txt".
pFileDirName -  Pointer to a const char pointer. When function succeeds,
                *pFileDirName will be filled with first valid file/dir
				name (relative to the working directory).
pbFile -        Pointer to a bool variable. When function succeeds, *pbFile
                will be filled with whether this entry is regular file (in
				constrast to a sub-folder).
missingHandle - A handle to missing file/dir structure. Can be set to ZERO
                to prevent this function from adding missing entities.
Return value:
Positive for success; zero for syntax error; <0 for failure.
*/
long long _findFirstFileHandle(const char *pWorkingDir, const char *pInqName,
	                           const char **pFileDirName, bool *pbFile,
							   long long missingHandle, bool bUpdateAccessTime = true);
bool _findNextFile(long long handle, const char **pFileDirName, bool *pbFile);
void _closeFindHandle(long long handle);

/* return 0 for success, -1 for error. */
int _deleteFile(const char *pFileName);

/* Try to clear a folder specified by pDirName, empty or not. Return 0 for
success, -1 for error. */
int _deleteDir(const char *pDirName);

/* Delete a file or folder specified by pName under pWorkingDir. Note that NO
WILDCARDS are allowed. Return 0 for success; 1 for invalid parameters; -1 for
other exceptions. */
int _deleteFileDir(const char *pWorkingDir, const char *pName);

I64 _getFileTime(const char *pFileDirName);

/* Clear all files and folders under pWorkingDir that expired (defined as
modified before certain period of time (nMaxValidTimeBack, in seconds).
NOTE that this function will NOT remove pWorkingDir. Return 0 for sucess,
and <0  for exceptions. */
int _clearExpiredFiles(const char *pWorkingDir, int nMaxValidTimeBack);

/* Change current working directory to where the EXE is stored. */
bool _changeFolder2Current();

}

#endif // __DDRCOMMLIB_CROSSPLATFORM_FILE_IO_OPERATIONS_H_INCLUDED__
