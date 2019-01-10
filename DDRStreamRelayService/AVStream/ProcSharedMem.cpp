#include "ProcSharedMem.h"

#include <string>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

namespace DDRGadgets {

struct SharedMemServer::_IMPL {
	HANDLE hMapFile;
	void *pBuf;
	std::string name;
	int szBuf;
	_IMPL() : hMapFile(0), pBuf(nullptr) {}
	void _release() {
		if (pBuf != nullptr) {
			::UnmapViewOfFile(pBuf);
			pBuf = 0;
		}
		if (hMapFile != 0) {
			::CloseHandle(hMapFile);
			hMapFile = 0;
		}
	}
	~_IMPL() { _release(); }
	bool Set(const char *pName, int szBuf) {
		if (!pName || *pName == '\0' || szBuf <= 0) { return false; }
		_release();
		hMapFile = ::CreateFileMappingA(INVALID_HANDLE_VALUE,
			                            nullptr,
										PAGE_READWRITE,
										0, szBuf,
										pName);
		if (0 == hMapFile) { return false; }
		pBuf = ::MapViewOfFile(hMapFile,
			                   FILE_MAP_ALL_ACCESS,
							   0, 0, szBuf);
		if (!pBuf) { return false; }
		this->name = pName;
		this->szBuf = szBuf;
		return true;
	}
};

SharedMemServer::SharedMemServer() {
	m_pIMPL = new _IMPL;
}

bool SharedMemServer::SetServer(const char *pName, int szBuf) {
	if (m_pIMPL) {
		return m_pIMPL->Set(pName, szBuf);
	} else { return false; }
}

SharedMemServer::SharedMemServer(const char *pName, int szBuf) {
	m_pIMPL = new _IMPL;
	SetServer(pName, szBuf);
}

SharedMemServer::~SharedMemServer() {
	if (m_pIMPL) {
		delete m_pIMPL;
	}
}

bool SharedMemServer::IsOkay() const {
	if (m_pIMPL) {
		return (m_pIMPL->hMapFile != 0 && m_pIMPL->pBuf != nullptr);
	} else { return false; }
}

int SharedMemServer::GetBufSize() const {
	if (IsOkay()) {
		return m_pIMPL->szBuf;
	} else { return 0; }
}

char* SharedMemServer::GetBufHead() {
	if (IsOkay()) {
		return (char*)(m_pIMPL->pBuf);
	} else { return nullptr; }
}

struct SharedMemClient::_IMPL {
	HANDLE hMapFile;
	const void *pBuf;
	std::string name;
	int szBuf;
	_IMPL() : hMapFile(0), pBuf(nullptr) {}
	void _release() {
		if (pBuf != nullptr) {
			::UnmapViewOfFile(pBuf);
			pBuf = 0;
		}
		if (hMapFile != 0) {
			::CloseHandle(hMapFile);
			hMapFile = 0;
		}
	}
	~_IMPL() { _release(); }
	bool Set(const char *pName, int szBuf) {
		if (!pName || *pName == '\0' || szBuf <= 0) { return false; }
		_release();
		hMapFile = ::OpenFileMappingA(FILE_MAP_READ, FALSE, pName);
		if (0 == hMapFile) { return false; }
		pBuf = ::MapViewOfFile(hMapFile,
			                   FILE_MAP_READ,
							   0, 0, szBuf);
		if (!pBuf) { return false; }
		this->name = pName;
		this->szBuf = szBuf;
		return true;
	}
};

SharedMemClient::SharedMemClient() {
	m_pIMPL = new _IMPL;
}

bool SharedMemClient::SetClient(const char *pName, int szBuf) {
	if (m_pIMPL) {
		return m_pIMPL->Set(pName, szBuf);
	} else { return false; }
}

SharedMemClient::SharedMemClient(const char *pName, int szBuf) {
	m_pIMPL = new _IMPL;
	SetClient(pName, szBuf);
}

SharedMemClient::~SharedMemClient() {
	if (m_pIMPL) {
		delete m_pIMPL;
	}
}

bool SharedMemClient::IsOkay() const {
	if (m_pIMPL) {
		return (m_pIMPL->hMapFile != 0 && m_pIMPL->pBuf != nullptr);
	} else { return false; }
}

int SharedMemClient::GetBufSize() const {
	if (IsOkay()) {
		return m_pIMPL->szBuf;
	} else { return 0; }
}

const char* SharedMemClient::GetBufHead() const {
	if (IsOkay()) {
		return (const char*)(m_pIMPL->pBuf);
	} else { return nullptr; }
}

}
