#ifndef __DDRCOMMLIB_MARS_LOCALSERVICEMODULE_COMPONENT_H_INCLUDED__
#define __DDRCOMMLIB_MARS_LOCALSERVICEMODULE_COMPONENT_H_INCLUDED__

#include <string>
#include <mutex>
#include <atomic>
#include "CommLib/MessageQueue.h"
#include "CommLib/SocketWrapper.h"
#include "CommLib/NodeBCRcvHelper.h"
#include "CommLib/FileSource.h"
#include "InfoStrProc.h"
#include "TalkHelper.h"
#include "LSMCommon.h"
#include "DualChTCPClientBase.h"

namespace DDRCommLib { namespace MARS {
	
#ifndef __ONE_STREAM2PUSH_CHANNEL_INFO_DEFINED__
#define __ONE_STREAM2PUSH_CHANNEL_INFO_DEFINED__
	typedef struct {
		int avType; // 1 - audio; 2 - video; 3 - audio + video; otherwise, N.A.;
		int devType; // 0 - IP device; 1 - AUX3.5/USB audio device
		std::string localAccStr;
		std::string remoteAddress;
	} _oneAVChannel2Push;
#endif

#ifndef __ONE_STREAM2PULL_CHANNEL_INFO_DEFINED__
#define __ONE_STREAM2PULL_CHANNEL_INFO_DEFINED__
	typedef struct {
		int avType; // 1 - audio; 2 - video; 3 - audio + video; otherwise, N.A.;
		std::string accStr;
	} _oneAVChannel2Pull;
#endif

class SafeMsgQueue {
public:
	SafeMsgQueue() : queue(new AdaptiveMsgQ(1, 1024)) {}
	void Send(OneChConn_Client *pOneCh, std::vector<char> &vec1, std::vector<char> &vec2);
	void Send(DualChTCPClientBase *pDualCh, std::vector<char> &vec1, std::vector<char> &vec2);
	void AddMsg(const char *pHead, int nLen);
	void AddMsg2(const char *pHead1, int nLen1, const char *pHead2, int nLen2);
	bool PopMsg(std::vector<char> &vec);
	int GetNumOfMsg();
	int GetNumBytes();
protected:
	std::timed_mutex locker;
	std::shared_ptr<AdaptiveMsgQ> queue;
};

struct SharedInfo {
	bool m_bValid;

	std::string m_moduleName;
	std::string m_localServerName;

	std::atomic<int> m_connStat;
	bool m_bRSConnected;
	int m_nOpMode; // 1 - monitor Op.; 2 - client Op.
	int m_nMonitors;
	int m_nClients;

	std::timed_mutex m_chLock;
	_oneAVChannel2Push *m_PushChs;
	_oneAVChannel2Pull *m_pPullCh;
	int m_peerTalkerType; // 0 - none; 1 - MONITOR; 2 - CLIENT

	std::shared_ptr<SafeMsgQueue> m_cmdRcv;
	std::shared_ptr<SafeMsgQueue> m_cmdSnd;
	std::shared_ptr<SafeMsgQueue> m_cmdRespRcv;
	std::shared_ptr<SafeMsgQueue> m_cmdRespSnd;
	std::shared_ptr<SafeMsgQueue> m_alarmRcv;
	std::shared_ptr<SafeMsgQueue> m_alarmSnd;
	std::shared_ptr<SafeMsgQueue> m_statusRcv;
	std::shared_ptr<SafeMsgQueue> m_statusSnd;
	std::shared_ptr<SafeMsgQueue> m_fileRespSnd;

	std::shared_ptr<TalkHelper> m_pTalkHelper;
	
	SharedInfo(const char *pModuleName, const char *pLSName);
	bool EnableAVStreamHandling(bool bAlwaysAnswerCalls);
	~SharedInfo();
	bool IsValid() const;
};

class LSMComp : protected DualChTCPClientBase {
public:
	LSMComp(SharedInfo *pShared);
	virtual ~LSMComp() {}

	bool IsValid() const;
	void Process();

	bool UpdateFileDir(int type, const char *pFileDirNames);

protected:
	bool m_bValid;
	SharedInfo *m_pShared;
	std::shared_ptr<NodeBCRcvHelper> m_pNodeHelper;
	std::shared_ptr<FileSource> m_pFileSrc;
	U32 m_serIP;
	unsigned short m_serPort;
	int m_prCnt;
	std::vector<char> m_tVec, m_tVec2;

	OneChAVInfo m_resInfo;
	OneChAllocation m_resAlloc;
	OneChAVInfo m_avInfo[MAX_AUDIOVIDEO_CHANNELS];
	OneChAllocation m_avAlloc[MAX_AUDIOVIDEO_CHANNELS];

	bool genMainLoginStr(std::vector<char> &vec) override;
	bool genSubLoginStr(unsigned int uid, std::vector<char> &vec) override;
	// 0 - login successful; -1 - login exception (INCORRECT PASSWORD, etc);
	// 1 - other login error
	int parseMainLoginResp(const char *pRespHead, int nRespLen, unsigned int &newUID) override;
	// 0 - okay; 1 - disconnection due to illegal strings;
	// -1 - server side notification of exception (INCORRECT PASSWORD, etc)
	int processRcv(const std::vector<char> &strVec, bool bFromMain) override;

	void _sendTalks();
	void _groupChsInfoAlloc();
	void _forwardSndQueue();
	bool _isServerAddrChanged();
};

}}

#endif // __DDRCOMMLIB_MARS_LOCALSERVICEMODULE_COMPONENT_H_INCLUDED__
