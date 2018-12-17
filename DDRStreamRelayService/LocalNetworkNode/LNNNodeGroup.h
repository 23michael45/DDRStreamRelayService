#ifndef __DDR_LNN_LIB_NODEGROUP_INTERFACE_H_INCLUDED__
#define __DDR_LNN_LIB_NODEGROUP_INTERFACE_H_INCLUDED__

#ifdef DDR_LOCALNETWORKNODE_LIB
#define __API__DDRLNN__ __declspec(dllexport)   
#else
#define __API__DDRLNN__ __declspec(dllimport)
#ifdef _DEBUG
#ifndef _WIN64
#pragma comment (lib, "LocalNetworkNode/LocalNetworkNode_d.lib")
#else
#pragma comment (lib, "LocalNetworkNode/LocalNetworkNode_X64d.lib")
#endif
#else
#ifndef _WIN64
#pragma comment (lib, "LocalNetworkNode/LocalNetworkNode_r.lib")
#else
#pragma comment (lib, "LocalNetworkNode/LocalNetworkNode_X64r.lib")
#endif
#endif
#endif

#ifndef BYTE
typedef unsigned char BYTE;
#endif

namespace DDRLNN
{
	class LNNNodeGroupInterface
	{
	public:
		// *********************** 服务器节点创建与配置 *********************** //
		// *********************** Server node creation and configuration *********************** //

		// 添加TCP服务器节点. 如果同名服务器节点已存在则返回false; 否则返回true.
		// Try to add a TCP server node. Return false if a server node with the same name exists.
		virtual bool _stdcall AddTCPServer(const char *pName,
								           BYTE authType // 0 - no auth.; 1 - Read Auth. (C->S); 2 - Write Auth. (S->C)
										   ) = 0;
		// 添加UDP服务器节点. 如果同名服务器节点已存在则返回false; 否则返回true.
		// Try to add a UDP server node. Return false if a server node with the same node exist.
		virtual bool _stdcall AddUDPServer(const char *pName) = 0;
		// 设置服务器节点缓冲大小和最大连接数(UDP节点忽略最大连接数)
		// 成功返回1; 设置失败返回0; 服务器节点名不存在返回-1
		// Try to set server buffer and max number of connections (latter ignored for UDP).
		// Return 1 for success; 0 for failure to set (node status may prohibit this setting);
		// -1 for failure to find a server node with specified name
		virtual int _stdcall SetServerOption(const char *pServerName, // name of the targeted server node
			                                 int szSndBuf, int szRcvBuf,
											 int nMaxClients = 10) = 0;
		
		// *********************** 客户端节点的创建与配置 *********************** //
		// *********************** Client node creation and configuration *********************** //

		// 添加TCP客户端节点. 如果同名客户端节点已存在则返回false; 否则返回true.
		// Try to add a TCP client node. Return false if a server node with the same name exists.
		virtual bool _stdcall AddTCPClient(const char *pName,
			                               BYTE authType // 0 - no auth.; 1 - read auth. (C->S); 2 - write auth. (S->C)
										   ) = 0;
		// 添加UDP客户端节点. 如果同名客户端节点已存在则返回false; 否则返回true.
		// Try to add a UDP client node. Return false if a server node with the same name exists.
		virtual bool _stdcall AddUDPClient(const char *pName) = 0;
		// 设置客户端节点缓冲大小
		// 成功返回1; 设置失败返回0; 服务器节点名不存在返回-1
		// Try to set server buffer and max number of connections (ignored for UDP).
		// Return 1 for success; 0 for failure to set (node status may prohibit this setting);
		// -1 for failure to find a client node with specified name
		virtual int _stdcall SetClientOption(const char *pClientName, // name of the targeted client node
			                                 int szSndBuf, int szRcvBuf) = 0;


		// *********************** 回调函数设置 *********************** //
		// *********************** CALLBACK settings *********************** //

		// _scb_开头函数用于服务器节点, _ccb_开头函数用于客户端节点.
		// 注意此类函数中void *pArg, 在用户自定义的回调函数中可以用于任意复杂的参数传递
		// _scb_ for servers and _ccb for clients.
		// Note that "void *pArg" can be used by customed callback functions to pass arbitarily
		// complex parameters

		// 用于填充std::vector<BYTE>的回调函数
		// pVecBytes为std::vector<BYTE>实例的指针(转化为void*)
		// 注: 在用户自定义的回调函数中, 如需对pVecBytes进行resize, 请务必调用DDRLNN::ResizeVecBytes()
		// 以确保跨DLL边界调用的内存安全
		// Callback to fill up a vector of bytes
		// pVecBytes is a pointer to std::vector<BYTE> converted to void*
		// (resizing by DDRLNN::ResizeVecBytes() to ensure memory integrity across DLL boundary)
		typedef bool(*CALLBACK_VecBytesFill)(void *pVecBytes, void *pArg);
		// 用于处理接收缓冲区的回调函数
		// pBuf指向缓冲区头, nBufLen代表其缓冲区长度
		// 返回值制定缓冲区尾部需要保存的字节数长度 (例, 返回值为2代表缓冲区前部
		// (nBufLen-2)字节已经处理并需要被丢弃, 尾部2字节需要被移至缓冲区头部保存)
		// Callback to process a buffer of bytes (receving publications)
		// pBuf points to the head of the buffer and nBufLen is the length of buffer.
		// Return the length of the buffer tails to keep (e.g, return 2 means the first
		// (nBufLen-2) bytes have been processed and the last two bytes have to be preserved
		typedef unsigned int(*CALLBACK_BUF2PROCESS)(BYTE *pBuf, unsigned int nBufLen, void *pArg);
		// 用于进行读/写验证的回调函数
		// pAuthStr指向将要验证的字符串(可以包含可见或不可见字符), nAuthLen为其长度,
		// pNextPos为用来接收部分验证结果的整型指针
		// 如果验证通过, *pNextPos的值代表完整验证字符串之后的第一个字节相对于pAuthStr的偏移量
		// (特别适用于验证后还有剩余的情况), 同时函数返回true; 如果当前验证失败, 函数返回false,
		// *pNextPos的值被赋-1表示无论是否有后续字节验证一定失败, 不为-1表示最终验证结果可能取决于
		// 后续字节。
		// Callback to validate Read Auth.
		// Return true if validation is successful, and fill nextPos with next offset
		// after the complete validation string; return false for failed validatation,
		// and fill nextPos with -1 ONLY IF IT IS ALREADY IMPOSSIBLE to validate even
		// if more bytes are received, otherwise the final results depend on incoming bytes.
		typedef bool(*CALLBACK_AuthValidator)(const BYTE *pAuthStr, int nAuthLen, int *pNextPos, void *pArg);

		// 设置响应新数据发布函数的参数(void*)
		// 成功返回1; 设置失败返回0; 服务器节点名不存在返回-1
		// Set argument for new data publish function.
		// Return 1 for success; 0 for failure to set;
		// -1 for failure to find a server node with specified name
		virtual int _stdcall _scb_SetArg_NewDataPublish(const char *pServerName, // name of the targeted server node
			                                            void *pArgNDP) = 0;
		// 设置响应新数据发布函数的指针
		// 成功返回1; 设置失败返回0; 服务器节点名不存在返回-1
		// Set callback function pointer to publish new data.
		// The function pointer will be used to check if new data are available to publish.
		// Return 1 for success; 0 for failure to set;
		// -1 for failure to find a server node with specified name
		virtual int _stdcall _scb_SetFunc_NewDataPublish(const char *pServerName, // name of the targeted server node
			                                             CALLBACK_VecBytesFill pFuncNDP) = 0;
		// 设置响应空闲发布函数的参数(void*)
		// 成功返回1; 设置失败返回0; 服务器节点名不存在返回-1
		// Set argument for idling data generation function.
		// Return 1 for success; 0 for failure to set;
		// -1 for failure to find a server node with specified name
		virtual int _stdcall _scb_SetArg_IdlingGenerator(const char *pServerName, // name of the targeted server node
			                                             void *pArgIG) = 0;
		// 设置空闲周期, 单位毫秒
		// 成功返回1; 设置失败返回0; 服务器节点名不存在返回-1
		// Set idling period in milliseconds.
		// Return 1 for success; 0 for failure to set;
		// -1 for failure to find a server node with specified name
		virtual int _stdcall _scb_SetParam_IdlingGenerator(const char *pServerName, // name of the targeted server node
			                                               unsigned int idlingPeriodInMs) = 0;
		// 设置响应空闲发布函数的指针(长时间无新数据发布时此函数被调用)
		// 成功返回1; 设置失败返回0; 服务器节点名不存在返回-1
		// Set callback function pointer to generate idling data. This functions pointer will
		// be used when certain period of time (idling period) has elapsed without publishing anything.
		// Return 1 for success; 0 for failure to set;
		// -1 for failure to find a server node with specified name
		virtual int _stdcall _scb_SetFunc_IdlingGenerator(const char *pServerName, // name of the targeted server node
			                                              CALLBACK_VecBytesFill pFuncIG) = 0;
		// 设置服务器写验证生成函数的参数(void*)
		// 成功返回1; 设置失败返回0; 服务器节点名不存在返回-1
		// Set argument for Write Auth. generation function.
		// Return 1 for success; 0 for failure to set;
		// -1 for failure to find a server node with specified name
		virtual int _stdcall _scb_SetArg_WAGenerator(const char *pServerName, // name of the targeted server node
			                                         void *pArgWAG) = 0;
		// 设置服务器写验证生成函数的指针
		// 成功返回1; 设置失败返回0; 服务器节点名不存在返回-1
		// Set callback function to generate Write Auth. data (Server->Client, validated by clients).
		// Return 1 for success; 0 for failure to set;
		// -1 for failure to find a server node with specified name
		virtual int _stdcall _scb_SetFunc_WAGenerator(const char *pServerName, // name of the targeted server node
			                                          CALLBACK_VecBytesFill pFuncWAG) = 0;
		// 设置服务器读验证函数的参数(void*)
		// 成功返回1; 设置失败返回0; 服务器节点名不存在返回-1
		// set argument for Read Auth. validation function.
		// Return 1 for success; 0 for failure to set;
		// -1 for failure to find a server node with specified name
		virtual int _stdcall _scb_SetArg_RAValidator(const char *pServerName, // name of the targeted server node
			                                         void *pArgRAV) = 0;
		// 设置服务器读验证函数的指针
		// 成功返回1; 设置失败返回0; 服务器节点名不存在返回-1
		// Set callback function to validate Read Auth. (Client->Server, validated by servers).
		// Return 1 for success; 0 for failure to set;
		// -1 for failure to find a server node with specified name
		virtual int _stdcall _scb_SetFunc_RAValidator(const char *pServerName, // name of the targeted server node
			                                          CALLBACK_AuthValidator pFuncRAV) = 0;

		// 设置接收数据处理函数的参数(void*)
		// 成功返回1; 设置失败返回0; 客户端节点名不存在返回-1
		// Set argument for receiving buffer processing function.
		// Return 1 for success; 0 for failure to set;
		// -1 for failure to find a client node with specified name
		virtual int _stdcall _ccb_SetArg_onRecvData(const char *pClientName, // name of the targeted client node
			                                        void *pArgORD) = 0;
		// 设置接收数据处理函数的指针
		// 成功返回1; 设置失败返回0; 客户端节点名不存在返回-1
		// Set callback function pointer to process receiving buffer.
		// The function pointer will be used to process received content and retain unprocessed content.
		// Return 1 for success; 0 for failure to set;
		// -1 for failure to find a client node with specified name
		virtual int _stdcall _ccb_SetFunc_onRecvData(const char *pClientName, // name of the targeted client node
			                                         CALLBACK_BUF2PROCESS pFuncORD) = 0;
		// 设置客户端读验证生成函数的参数(void*)
		// 成功返回1; 设置失败返回0; 客户端节点名不存在返回-1
		// Set argument for Read Auth. generator.
		// Return 1 for success; 0 for failure to set;
		// -1 for failure to find a client node with specified name
		virtual int _stdcall _ccb_SetArg_RAGenerator(const char *pClientName, // name of the targeted client node
			void *pArgRAG) = 0;
		// 设置客户端读验证生成函数的指针
		// 成功返回1; 设置失败返回0; 客户端节点名不存在返回-1
		// Set callback function pointer to generate Read Auth. (Client->Server, validated by servers).
		// Return 1 for success; 0 for failure to set;
		// -1 for failure to find a client node with specified name
		virtual int _stdcall _ccb_SetFunc_RAGenerator(const char *pClientName, // name of the targeted client node
			                                          CALLBACK_VecBytesFill pFuncRAG) = 0;
		// 设置客户端写验证函数的参数(void*)
		// 成功返回1; 设置失败返回0; 客户端节点名不存在返回-1
		// Set argument for Write Auth. validator.
		// Return 1 for success; 0 for failure to set;
		// -1 for failure to find a client node with specified name
		virtual int _stdcall _ccb_SetArg_WAValidator(const char *pClientName, // name of the targeted client node
			                                         void *pArgWAV) = 0;
		// 设置客户端写验证函数的指针
		// 成功返回1; 设置失败返回0; 客户端节点名不存在返回-1
		// Set callback function pointer to valid Write Auth. (Server->Client, validated by clients).
		// Return 1 for success; 0 for failure to set;
		// -1 for failure to find a client node with specified name
		virtual int _stdcall _ccb_SetFunc_WAValidator(const char *pClientName, // name of the targeted client node
			                                          CALLBACK_AuthValidator pFuncWAV) = 0;

		// *********************** 节点组运行功能 *********************** //
		// *********************** Node group running functionalities *********************** //

		// 设置节点组运行节奏
		// 节点组每次循环由IO等待时间(毫秒)和休眠时间(毫秒)组成
		// Set running tempo of this node group. One iteration is composed of I/O time (millseconds)
		// plus sleep time (milliseconds).
		virtual bool _stdcall SetLoopTempo(int ioTime_ms, int sleepTime_ms) = 0;
		// 在子线程中开始大循环节点通信, 也是DDRLNN最核心的功能
		// Start a big loop of socket communications in a sub-thread. The thread will periodically
		// check I/O status, publish/receive contents through server/client nodes, and call callback
		// functions when certain conditions are met.
		virtual bool _stdcall StartRunning() = 0;
		// 结束大循环并等待子线程返回
		// End the big loop and wait for the sub-thread to finish.
		virtual bool _stdcall StopRunning() = 0;
		// 通过服务器节点发布内容
		// 成功返回1; 发布失败返回0; 服务器节点名不存在返回-1
		// Try to pulish something through a certain server node.
		// Return 1 for success; 0 for failure to publish;
		// -1 for failure to find a server node with specified name
		virtual int _stdcall PublishContent(const char *pServerName, // name of the targeted server node
			                                const void *pContent, int nContentLen) = 0;
		// 获取上次服务器端成功发布至今的时间(毫秒)
		// 如果服务器节点名不存在返回-1
		// return time (in milliseconds) since last successful publication through a certain node;
		// return -1 for failure to find a server node with specified name
		virtual __int64 _stdcall GetTimeSinceLastPublication(const char *pServerName // name of the targeted server node
			                                                 ) const = 0; // in milliseconds
		// 通过客户端节点获取订阅内容
		// pDataBytes为用于接收的缓冲区头, nBufCapacity为缓冲区最大容量
		// 成功获取后pDataBytes将被填充为获取的内容, *pnRemBytes填充为内部缓冲区剩余
		// 字节数, 函数返回获取内容的长度(0代表无); 如果客户端节点名不存在返回-1
		// Try to get received data from a certain client node.
		// Return -1 for failure to find such a client node (by name);
		// otherwise fill the buffer with content and return actual
		// number of bytes filled (<= bufCapacity guaranteed), and 
		// the unsigned integer pointed by pnRemBytes will be assigned
		// the number of bytes left in the unprocessed internal buffer
		// When nothing is available, return 0.
		virtual int _stdcall GetData(const char *pClientName, // name of the targeted client node
			                         void *pDataBytes, unsigned int nBufCapacity,
									 unsigned int *pnRemBytes // to receive number of remaining bytes
									 ) = 0;
		// 获取上次客户端获取订阅内容至今的时间(毫秒)
		// 如果客户端节点名不存在返回-1
		// return time (in milliseconds) since last successful reception of data through a certain
		//client node. Return -1 for failure to find a client node with specified name
		virtual __int64 _stdcall GetTimeSinceLastReception(const char *pClientName // name of the targeted client node
			                                              ) const = 0; // in milliseconds

		// *********************** Node info and status *********************** //

		// Return -1 for failure to find such a node;
		// Return 0 for structured data; 1 for streamed data.
		virtual int _stdcall IsStreamed(const char *pNodeName, bool bServer) const = 0;
		// Return -1 for failure to find such a node;
		// Return 0 for UDP node; 1 for TCP node.
		virtual int _stdcall IsTCPBased(const char *pNodeName, bool bServer) const = 0;
		// Return -1 for failure to find such a node;
		// Return 0 for no authorization; 1 for Read Auth.; 2 for Write Auth..
		virtual int _stdcall GetAuthType(const char *pNodeName, bool bServer) const = 0;
		// Return -1 for failure to find such a server node;
		// Return 0 if server has not been established; return 1 otherwise.
		virtual int _stdcall IsServerEstablished(const char *pServerNode) const = 0;
		// Return -1 for UDP servers;
		// return actual connected clients for TCP servers.
		virtual int _stdcall GetNumOfConnections(const char *pServerNode) const = 0;
		// Return -1 for failure to find such a client node;
		// Return 0 if client is not connected; return 1 otherwise.
		virtual int _stdcall IsClientConnected(const char *pClientNode) const = 0;

		virtual void _stdcall destroy() = 0;
		void operator delete(void *p) {
			if (p != nullptr) {
				((LNNNodeGroupInterface*)p)->destroy();
			}
		}
	};

	// 调用此函数在堆上创建LNN节点组对象, 注意其返回的是裸指针, 建议包含
	// "LNNFactory.h"文件使用智能指针包装
	// Call this factory function to create a node group object (raw pointer
	// on heap). Recommend to include "LNNFactory.h" to use smare pointer
	// to wrap around it.
	extern "C" __API__DDRLNN__ LNNNodeGroupInterface* _stdcall _createLNNNodeGroup();

	// 调用此函数改变std::vector<BYTE>的大小, 注意此函数主要的用处在于
	// 用户自定义的回调函数中, 确认std::vector操作的安全
	// Call this function to resize std::vector<BYTE> (in form of its pointer converted to void*)
	// to ensure safety across DLL boundary
	extern "C" __API__DDRLNN__ void _stdcall ResizeVecBytes(void *pVecBytes, unsigned int szNew);
	// 调用此函数获取std::vector<BYTE>缓冲区的头地址, 注意此函数主要的用处在于
	// 用户自定义的回调函数中, 确认std::vector操作的安全
	// Call this function to get head address of the buffer managed by std::vector<BYTE>
	// (in form of its pointer converted to void*) to ensure safety across DLL boundary
	extern "C" __API__DDRLNN__ void* _stdcall GetVecBytesHead(void *pVecBytes);
}

#endif // __DDR_LNN_LIB_NODEGROUP_INTERFACE_H_INCLUDED__