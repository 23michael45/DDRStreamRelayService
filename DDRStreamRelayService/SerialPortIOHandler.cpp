#include "SerialPortIOHandler.h"

#include <stdio.h>
#include <chrono>

namespace DDRDrivers {

SerialPortIOHandler::SerialPortIOHandler() {
	m_nState = 0;
	m_hComm = nullptr;
	memset((void*)&m_DCB, 0, sizeof(_DCB));
	m_DCB.DCBlength = sizeof(_DCB);
	memset(&m_ReadOverlapped, 0, sizeof(OVERLAPPED));
	memset(&m_WriteOverlapped, 0, sizeof(OVERLAPPED));
	m_ReadOverlapped.hEvent = nullptr;
	m_WriteOverlapped.hEvent = nullptr;
	m_CT.ReadIntervalTimeout = 0;
	m_CT.ReadTotalTimeoutMultiplier = 0;
	m_CT.ReadTotalTimeoutConstant = 1;
	m_CT.WriteTotalTimeoutMultiplier = 0;
	m_CT.WriteTotalTimeoutConstant = 1;
	m_subThreadState = (BYTE)0;
}

SerialPortIOHandler::~SerialPortIOHandler() {
	Close();
	if (m_ReadOverlapped.hEvent != nullptr) {
		::CloseHandle(m_ReadOverlapped.hEvent);
		m_ReadOverlapped.hEvent = nullptr;
	}
	if (m_WriteOverlapped.hEvent != nullptr) {
		::CloseHandle(m_WriteOverlapped.hEvent);
		m_WriteOverlapped.hEvent = nullptr;
	}
}

bool SerialPortIOHandler::SetSerialPort(int portID, unsigned int baudRate) {
	if (2 == m_nState) {
		Close();
	}
	if (portID < 0 || portID > 99) { return false; }
	if (portID < 10) {
		memcpy(m_commName, "COM", 3);
		m_commName[3] = portID + '0';
		m_commName[4] = '\0';
	}
	else {
		memcpy(m_commName, "\\\\.\\COM", 7);
		m_commName[7] = (portID / 10) + '0';
		m_commName[8] = (portID % 10) + '0';
		m_commName[9] = '\0';
	}
	m_baudRate = baudRate;
	if (nullptr == m_ReadOverlapped.hEvent)	{
		m_ReadOverlapped.hEvent = ::CreateEventA(nullptr, true,
			false, nullptr);
		if (!m_ReadOverlapped.hEvent) { return false; }
	} else {
		::ResetEvent(m_ReadOverlapped.hEvent);
	}
	if (nullptr == m_WriteOverlapped.hEvent) {
		m_WriteOverlapped.hEvent = ::CreateEventA(nullptr, true,
			false, nullptr);
		if (!m_WriteOverlapped.hEvent) { return false; }
	} else {
		::ResetEvent(m_WriteOverlapped.hEvent);
	}

	m_nState = 1;
	return true;
}

bool SerialPortIOHandler::Open() {
	if (1 == m_nState) {
		if (_Open())
		{
			m_nState = 2;
			m_subThreadState = (BYTE)0;
			return true;
		} else {
			m_subThreadState = (BYTE)0;
			return false;
		}
	} else { return (2 == m_nState); }
}

bool SerialPortIOHandler::_Open() {
	if (!(m_hComm = ::CreateFileA(m_commName, GENERIC_READ | GENERIC_WRITE,
		                          0, nullptr, OPEN_EXISTING,
								  FILE_ATTRIBUTE_NORMAL /*| FILE_FLAG_OVERLAPPED*/,
								  nullptr))) {
		m_subThreadState = 2;
		return false;
	}
	if (!::SetCommTimeouts(m_hComm, &m_CT)) {
		m_subThreadState = 2;
		return false;
	}
	if (!::GetCommState(m_hComm, &m_DCB)) {
		m_subThreadState = 2;
		return false;
	}
	m_DCB.BaudRate = m_baudRate;
	m_DCB.ByteSize = 8;
	m_DCB.Parity = NOPARITY;
	m_DCB.StopBits = ONESTOPBIT;
	if (!::SetCommState(m_hComm, &m_DCB)) {
		m_subThreadState = 2;
		return false;
	}
	::PurgeComm(m_hComm, PURGE_TXABORT | PURGE_RXABORT |
		                 PURGE_TXCLEAR | PURGE_RXCLEAR);
	m_subThreadState = 3;
	return true;
}

void SerialPortIOHandler::Close() {
	if (2 == m_nState) {
		::PurgeComm(m_hComm, PURGE_TXABORT | PURGE_RXABORT |
			                 PURGE_TXCLEAR | PURGE_RXCLEAR);
		::CloseHandle(m_hComm);
		m_hComm = nullptr;
		m_nState = 1;
	}
}

bool SerialPortIOHandler::_PrepareConn() {
	if (2 == m_nState) { return true; }
	if (1 == m_nState) {
		BYTE st = m_subThreadState;
		switch (st) {
		case (BYTE)0:
			m_subThreadState = (BYTE)1;
			m_subTh = std::thread(_myOpen, (void*)this);
			break;
		case (BYTE)1:
			return false;
			break;
		case (BYTE)2:
			m_subTh.join();
			m_subThreadState = (BYTE)0;
			break;
		case (BYTE)3:
			m_subTh.join();
			m_subThreadState = (BYTE)0;
			m_nState = 2;
			return true;
			break;
		}
	}
	return false;
}

bool SerialPortIOHandler::Read(char *pBuf, int desiredLen2read,
	                           int &actualLenRead) {
	if (!_PrepareConn()) { return false; }
	if (m_nState != 2) { return false; }
	if (desiredLen2read <= 0) { return true; }
	DWORD nErr;
	COMSTAT comStat;
	if (::ClearCommError(m_hComm, &nErr, &comStat))	{
		if (0 == comStat.cbInQue) {
			actualLenRead = 0;
			return true;
		}
		DWORD nlen;
		if (::ReadFile(m_hComm, pBuf,
			           desiredLen2read < (int)comStat.cbInQue ?
				       (DWORD)desiredLen2read : comStat.cbInQue,
					   &nlen, &m_ReadOverlapped)) {
			actualLenRead = (int)nlen;
		} else {
			actualLenRead = 0;
			int asdasdf = 1;
		}
		return true;
	}
	m_nState = 1;
	return false;
}

bool SerialPortIOHandler::Write(const char *pBuf, unsigned int nBytes) {
	if (!_PrepareConn()) { return false; }
	if (m_nState != 2) { return false; }
	DWORD nlen;
	if (!::WriteFile(m_hComm, pBuf, (DWORD)nBytes, &nlen,
		             &m_WriteOverlapped)) {
		if (::GetLastError() == ERROR_IO_PENDING) {
			WaitForSingleObject(m_WriteOverlapped.hEvent, 1);
			if (!::GetOverlappedResult(m_hComm, &m_WriteOverlapped,
				                       &nlen, false)) {
				return false;
			}
		} else { return false; } // failed to write and it is not pending
	}
	return true; // successfully written
}

bool SerialPortIOHandler::FlushRx() {
	if (2 == m_nState) {
		return (::PurgeComm(m_hComm, PURGE_RXCLEAR) != 0);
	} else { return false; }
}

}