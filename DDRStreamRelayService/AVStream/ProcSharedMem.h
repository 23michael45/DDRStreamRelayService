#ifndef __DDR_SHARED_MEMORY_FOR_IPC_H_INCLUDED__
#define __DDR_SHARED_MEMORY_FOR_IPC_H_INCLUDED__

namespace DDRGadgets
{
	class SharedMemServer
	{
	public:
		SharedMemServer();
		bool SetServer(const char *pName, int szBuf);
		SharedMemServer(const char *pName, int szBuf);
		~SharedMemServer();

		bool IsOkay() const;
		int GetBufSize() const;
		char* GetBufHead();

	private:
		struct _IMPL;
		_IMPL *m_pIMPL;
	};

	class SharedMemClient
	{
	public:
		SharedMemClient();
		bool SetClient(const char *pName, int szBuf);
		SharedMemClient(const char *pName, int szBuf);
		~SharedMemClient();

		bool IsOkay() const;
		int GetBufSize() const;
		const char* GetBufHead() const;

	private:
		struct _IMPL;
		_IMPL *m_pIMPL;
	};
}

#endif // __DDR_SHARED_MEMORY_FOR_IPC_H_INCLUDED__
