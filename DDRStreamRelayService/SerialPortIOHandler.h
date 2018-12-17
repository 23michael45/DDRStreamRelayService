#ifndef __DDR_SERIAL_PORT_IO_H_INCLUDED__
#define __DDR_SERIAL_PORT_IO_H_INCLUDED__

#include <thread>
#include <atomic>

#include "CommonDef/CommonDef.h"

namespace DDRDrivers
{
	class SerialPortIOHandler
	{
	public:
		SerialPortIOHandler();
		SerialPortIOHandler(const SerialPortIOHandler&) = delete;
		SerialPortIOHandler& operator= (const SerialPortIOHandler&) = delete;
		~SerialPortIOHandler();

		bool SetSerialPort(int portID, unsigned int baudRate);
		bool Open();
		void Close();

		// return FALSE ONLY when disconnected
		bool Read(char *pBuf, int desiredLen2read,
			      int &actualLenRead);
		// return if bytes are successfully written
		bool Write(const char *pBuf, unsigned int nBytes);
		bool FlushRx();

		int GetState() const { return m_nState; }
		bool IsOpened() const { return (2 == m_nState); }

	protected:
		int m_nState; // 0 - uninitialized; 1 - configured, not openend; 2 - opened, running
		char m_commName[20];
		unsigned int m_baudRate;
		HANDLE m_hComm;
		DCB m_DCB;
		COMMTIMEOUTS m_CT;
		OVERLAPPED m_ReadOverlapped;
		OVERLAPPED m_WriteOverlapped;

		bool _Open();

		// 0 - not launched; 1 - running;
		// 2 - finished with failure; 3 - finished with success
		bool _PrepareConn();
		std::atomic<BYTE> m_subThreadState;
		static bool _myOpen(void *pArg) {
			return ((SerialPortIOHandler*)pArg)->_Open();
		}
		std::thread m_subTh;
	};
}


#endif // __DDR_SERIAL_PORT_IO_H_INCLUDED__

