#ifndef __DDRCOMMLIB_INFOSTRING_PROCESSOR_H_INCLUDED__
#define __DDRCOMMLIB_INFOSTRING_PROCESSOR_H_INCLUDED__

#include <vector>
#include <string>
#include "CommLib/MessageQueue.h"

#ifndef U32
typedef unsigned int U32;
#endif
#ifndef U16
typedef unsigned short U16;
#endif

namespace DDRCommLib { namespace MARS {

const int MAX_AVPASS_LENGTH = 15;
const int MAX_AUDIOVIDEO_CHANNELS = 15;
const int MAX_CHANNEL_NAME_LENGTH = 31;
const int EXTRA_BYTES_FOR_ENCRYPTION = 10;

bool PackVecs(const char *pHead, int nLen,
	std::vector<char> &dstVec,
	bool bAppend = false);

bool PackVecs(const std::vector<char> &srcVec,
	std::vector<char> &dstVec,
	bool bAppend = false);

bool AddHeaderAndPack(const char *pHeader, int nHeaderLen,
	const char *pContent, int nContentLen,
	std::vector<char> &dstVec,
	bool bAppend = false);

// return value:
// 0 - successful
// 1 - pending message
// -1 - illegal
int DecodeNextMessage(ByteQ *pByteQ, std::vector<char> &tmpVec,
	                  std::vector<char> &dstVec, void*);

enum DDR_IOSTR_TYPE {
	ILLEGAL,
	INSUFFICIENT,
	EMPTY,
	LOGIN,
	NOTICE,
	STATUS,
	FILE,
	COMMAND,
	CMD_RESP,
	ALARM,
	CHATTING
};

DDR_IOSTR_TYPE DecodeInfoStr(const char *pBufHead, int nBufLen,
	                         const char **pNextBufPos,
							 std::vector<char> &strVec,
							 int *pStrContPos // the position of actual params
							 );

DDR_IOSTR_TYPE AnalyzeInfoStr(const std::vector<char> &strVec,
	                          int *pStrContPos // the position of actual params
							  );

struct OneChAVInfo {
	int devType;
	int avType; // 0 - N.A.; 1 - audio; 2 - video; 3 - audio+video
	int bwReq;
	std::string accessStr; // local access string
	char chName[MAX_CHANNEL_NAME_LENGTH + 1];
	OneChAVInfo() : avType(0) {}
	OneChAVInfo& operator= (const OneChAVInfo &);
};

struct OneChAllocation {
	bool bAllocated;
	int sessionID;
	char avpass[MAX_AVPASS_LENGTH + 1];
	U32 uip_BE;
	U16 port_BE;
	OneChAllocation() : bAllocated(false) {}
	OneChAllocation& operator= (const OneChAllocation&);
};

bool ReadChAVInfo(const char *pFileName,
	              OneChAVInfo *pChAVInfo // MAX_AVPASS_LENGTH channels
				  );

/* ROBOT-LOGIN
login\0Role=ROBOT;RID=DDR0024;PWD=Abcd;OpLv=3;OpTime=[milliseconds since epoch, 8 byte long long];
AV={[AVType('0'-N.A; '1'-audio; '2'-vidio; '3'-AV]+[chName(ending with '\0')]+[bandwidth required(4 bytes integer)]}
+ ... + {...}; (15 channels) + UID=[4 bytes unique ID];
*/
bool Gen_RobotLogin(const char *pRID, const char *pPWD,
		            char cOpLv, long long cOpTime,
					const OneChAVInfo *pAVChannels, // MAX_AUDIOVIDEO_CHANNELS
					unsigned int UID,
					std::vector<char> &vec // output
					);

/* MONITOR-LOGIN
login\0Role=MONITOR;RID=DDR0024;PWD=aBCD;OpLv=3;Nickname=John;UID=[4 bytes non-zero unique ID];
*/
bool Gen_MonitorLogin(const char *pRID, const char *pPWD,
	                  int mOpLv, const char *pNickname,
					  unsigned int UID,
					  std::vector<char> &vec);

/* CLIENT-LOGIN
login\0Role=CLIENT;RID=DDR0024;PWD=aBCD;OpLv=3;Nickname=John;UID=[4 bytes non-zero unique ID];
*/
bool Gen_ClientLogin(const char *pRID, const char *pPWD,
	                 int mOpLv, const char *pNickname,
					 unsigned int UID,
					 std::vector<char> &vec);

/* LSM-LOGIN
login\0Role=LOCALSERVICEMODULE;Nickname=MainService;UID=[4 bytes non-zero unique ID];
*/
bool Gen_LSMLogin(const char *pModuleName,
				  unsigned int UID,
	              std::vector<char> &vec);

/* SubChannel-LOGIN
login\0Role=SUBCHANNEL;RID=DDR0024;PWD=aBCD;UID=[4 bytes non-zero unique ID];
*/
bool Gen_SubChLogin(const char *pRID, const char *pPWD, unsigned int UID,
	                std::vector<char> &vec);

struct LoginInfo {
	int role; // 0 - robot; 1 - monitor
	const char *pID;
	int nIDLen;
	const char *pPWD;
	int nPWDLen;
	char OpAccessLevel; // level 0~9
	long long OpTime;
	const char *pNickName;
	int nNickNameLen;
	unsigned int UID; // non-zero unique ID for a dual-channel connection
	char avType[MAX_AUDIOVIDEO_CHANNELS];
	char chName[MAX_AUDIOVIDEO_CHANNELS][MAX_CHANNEL_NAME_LENGTH + 1];
	OneChAVInfo chs[MAX_AUDIOVIDEO_CHANNELS];
};

/* VERY IMPORTANT!!!
pStr starts after the type marks! For instance, the info string:
login\0Role=ROBOT;.......
has its pStr starting at "Role=...".
*/
bool Parse_LoginStr(const char *pStr, int nStrLen, LoginInfo &li);

/* LOGIN-RESPONSE
-SUCCESSFUL: login\0YourRole=[Role("ROBOT", "MONITOR", "CLIENT" or "LOCALSERVICEMODULE")];RID=DDR0024(robot ID);
-NOT SUCCESSFUL: login\0ret=3(login return code + '0');

login return code
0 - SUCCESSFUL,
1 - UNABLE_TO_ADD (limit reached)
2 - BUSY
3 - NO_PROTO_FILE (expired)
4 - INVALID_PROTO (expired)
5 - INCORRECT_PASSWORD (monitor password denied)
6 - FUNC_CALL_ERROR */
bool Gen_LoginResp(int nRet, // login return code
	               int role, // 0 - robot; 1 - monitor; 2 - client; 3 - LSM
				   const char *pRID,
				   unsigned int UID,
				   std::vector<char> &vec);

bool Parse_LoginResp(const char *pStr, int nStrLen,
	                 int *pRetCode, // login return code
					 int *pRole, // 0 - robot; 1 - monitor; 2 - client; 3 - LSM
					 const char **pRIDPtr, int *pRIDLen,
					 unsigned int *pUID
					 );

/* AV allocation info from RemoteServer to RemoteRobot telling where to PUSH these av streams (dynamic channels)
and to PULL the reserved av stream
notice\0type=RS2RR_AVPU;
Res=[UIP(big-endian)]+[port(big-endian)]+[vPass(ending with '\0')]; (optional)
AV={[AVType('0'-N.A; '1'-audio; '2'-vidio; '3'-AV]+[UIP(big-endian)]+[port(big-endian)]+[vPass(ending with '\0')]} + ... + {...}; (15 channels)
*/
bool Gen_RS2RR_AVPU(const OneChAVInfo *pReservedInfo, // only one channel
	                const OneChAllocation *pReservedAlloc, // only one channel
					const OneChAVInfo *pDynamicAVInfo, // MAX_AUDIOVIDEO_CHANNELS
					const OneChAllocation *pDynamicAlloc, // MAX_AUDIOVIDEO_CHANNELS
					std::vector<char> &vec);

bool Parse_RS2RR_AVPU(const char *pStr, int nStrLen,
	                  OneChAllocation *pRes, // reserved allocation, only one channel
					  OneChAllocation *pDynamicAlloc // MAX_AUDIOVIDEO_CHANNELS
					  );

/* Access and monitor count info from RemoteServer to RemoteRobot
notice\0
type=RS2RR_ACCPU;
nMonitors=[#monitors(4 bytes)];
mAccLv=2; ('0'~'9', remote monitors' highest access level)
mOpTime=[opTime(8 bytes)]; (M-side op's conn. time)
*/
bool Gen_RS2RR_ACCPU(int nMonitors, int mAccLv, long long mOpTime,
	                 std::vector<char> &vec);

bool Parse_RS2RR_ACCPU(const char *pStr, int nStrLen,
	                   int *pnMonitors, int *pmAccLv, long long *pmOpTime);

/*
notice\0
type=RR2RS_ACCPU;
nClients=[#clients(4 bytes)];
cAccLv=2; ('0'~'9', local clients' highest access level)
cOpTime=[opTime(8 bytes)]; (C-side op's conn. time)
*/
bool Gen_RR2RS_ACCPU(int nClients, int cAccLv, long long cOpTime,
	                 std::vector<char> &vec);

bool Parse_RR2RS_ACCPU(const char *pStr, int nStrLen,
	                   int *pnClients, int *pCAccLv, long long *pCOpTime);


/* AV allocation info from RemoteServer to Monitors telling where to PULL these av streams (dynamic channels)
and to PUSH to the reserved av stream
notice\0type=RS2RR_AVPU;
Res=[bandwidth allocated]+[UIP(big-endian)]+[port(big-endian)]+[vPass(ending with '\0')]; (optional)
AV={[AVType('0'-N.A; '1'-audio; '2'-vidio; '3'-AV]+[UIP(big-endian)]+[port(big-endian)]
+[vPass(ending with '\0')]+[chName(ending with '\0')]} + ... + {...}; (15 channels)
*/
// pReservedInfo and pReservedAlloc SHOULD BE NULL for non-op monitors!
bool Gen_RS2M_AVPU(const OneChAVInfo *pReservedInfo, // only one channel
	               const OneChAllocation *pReservedAlloc, // only one channel
				   const OneChAVInfo *pDynamicAVInfo, // MAX_AUDIOVIDEO_CHANNELS
				   const OneChAllocation *pDynamicAlloc, // MAX_AUDIOVIDEO_CHANNELS
				   const bool *pbDynamicDownstreamAllocated, // MAX_AUDIOVIDEO_CHANNELS
				   std::vector<char> &vec);

bool Parse_RS2M_AVPU(const char *pStr, int nStrLen,
	                 OneChAVInfo *pResInfo, // one channel
					 OneChAllocation *pResAlloc,
					 OneChAVInfo *pDynamicInfo, // MAX_AUDIOVIDEO_CHANNELS
					 OneChAllocation *pDynamicAlloc);

/* Access and monitor/client count info from RemoteServer to Monitors
notice\0
type=RS2M_ACCPU;
bRobotOnline=1; ('0'-offline, '1'-online)
bOp=1; ('0'-not Op; '1' - Op)
nMonitors=[#monitors(4 bytes)];
nClients=[#clients(4 bytes)];
*/
bool Gen_RS2M_ACCPU(bool bRobotOnline, bool bOp,
	                int nMonitors, int nClients,
					std::vector<char> &vec);

bool Parse_RS2M_ACCPU(const char *pStr, int nStrLen,
	                  bool *pbRobotOnline, bool *pbOp,
					  int *pnMonitors, int *pnClients);

/* Notify monitors of incorrect password (and following disconnection)
notice\0type = INCORRECT_PASSWORD;
*/
bool Gen_RS2M_PWDDenial(std::vector<char> &vec);

bool Parse_RS2M_PWDDenail(const char *pStr, int nStrLen);

/* 2-way talk (may be of audio/video type) message STARTED BY MONITOR (M->RS->(RR/LS)->LSM)
notice\0
type=MTalk;
msgID=[unique message ID(16 bytes, TWO long longs)];
avType=1; ('0'-N.A.; '1' - audio; '2' - video; '3' - audio + video)
*/
bool Gen_MTalk(long long msgID1, long long msgID2,
	           int avType, std::vector<char> &vec);

bool Check_MTalk(const char *pStr, int nStrLen);

bool Parse_MTalk(const char *pStr, int nStrLen,
	             long long *pMsgID1, long long *pMsgID2,
				 int *pAVType);

/* MTalk response (LSM->(LS/RR)->RS->M)
notice\0
type=MTalkResp;
msgID=[unique message ID(16 bytes, TWO long longs)];
*/
bool Gen_MTalkResp(long long msgID1, long long msgID2,
	               std::vector<char> &vec);

bool Check_MTalkResp(const char *pStr, int nStrLen);

bool Parse_MTalkResp(const char *pStr, int nStrLen,
	                 long long *pMsgID1, long long *pMsgID2);

/* 2-way talk (may be of audio/video type) message STARTED BY CLIENT (C->LS->LSM)
notice\0
type=CTalk;
msgID=[unique message ID(16 bytes, TWO long long)];
avType=1; ('0'-N.A.; '1' - audio; '2' - video; '3' - audio + video)
*/
bool Gen_CTalk(long long msgID1, long long msgID2,
	           int avType, std::vector<char> &vec);

bool Check_CTalk(const char *pStr, int nStrLen);

bool Parse_CTalk(const char *pStr, int nStrLen,
	             long long *pMsgID1, long long *pMsgID2,
				 int *pAVType);

/* CTalk response (LSM->LS->C)
notice\0
type=CTalkResp;
msgID=[unique message ID(8 bytes long long)];
*/
bool Gen_CTalkResp(long long msgID1, long long msgID2,
	               std::vector<char> &vec);

bool Check_CTalkResp(const char *pStr, int nStrLen);

bool Parse_CTalkResp(const char *pStr, int nStrLen,
	                 long long *pMsgID1, long long *pMsgID2);

/* 2-way talk (may be of audio/video type) message STARTED BY ROBOT (LSM->(LS/RR)->RS->M(op.) or LSM->LS->C(op.))
notice\0
type=RTalk;
msgID=[unique message ID(16 bytes, TWO long long)];
*/
bool Gen_RTalk(long long msgID1, long long msgID2,
	           std::vector<char> &vec);

bool Check_RTalk(const char *pStr, int nStrLen);

bool Parse_RTalk(const char *pStr, int nStrLen,
	             long long *pMsgID1, long long *pMsgID2);

/* RTalk response (M(op.)->RS->(RR/LS)->LSM or C(op.)->LS->LSM)
notice\0
type=RTalkResp;
msgID=[unique message ID(8 bytes long long)];
avType=1; ('0'-N.A.; '1' - audio; '2' - video; '3' - audio + video)
src=1; ('1'- remote MONITOR (op.); '2' - local CLIENT (op.))
*/
bool Gen_RTalkResp(long long msgID1, long long msgID2,
	               int avType, int srcMode,
				   std::vector<char> &vec);

bool Check_RTalkResp(const char *pStr, int nStrLen);

bool Parse_RTalkResp(const char *pStr, int nStrLen,
	                 long long *pMsgID1, long long *pMsgID2,
					 int *pAVType, int *pSrc);

/* COMMAND string (M(op.)->RS->(RR/LS)->LSM or C(op.)->LS->LSM or LSM-LS->LSM)
cmdO\0
[NPR, # of pre-routing(1 byte)]
{[UIP(big-endian)]+[port(big-endian)]} + ... + {...} (NPR IPs, in the FILO fashion where an earlier IP comes later)
[cmdID(8 bytes)]
[Keyword] + '\0' + [arguments(optional)]
*/
bool Gen_Command(const char *pCmdKeyword,
	             const char *pArgs, int nArgsLenInBytes,
				 long long cmdID,
				 std::vector<char> &vec);

bool Pass_Command(const char *pCmdHead, int nCmdLen,
	              U32 srcIP_BE, U16 srcPort_BE,
				  std::vector<char> &vec);

bool Parse_Command(const char *pCmdHead, int nCmdLen,
	               const char **ppKeyword,
				   const char **ppArgs,
				   int *pArgLen);

/* COMMAND RESPONSE string (LSM->(LS/RR)->RS->M(op.) or LSM->LS->C(op.) or LSM-LS->LSM)
cmdR\0
[NPR, # of pre-routing(1 byte)]
{[UIP(big-endian)]+[port(big-endian)]} + ... + {...} (NPR IPs, in the FILO fashion where an earlier IP comes later)
[cmdID(8 bytes)]
[Response] (may be null)
*/
bool Gen_CmdResp(const char *pCmdHead, int nCmdLen, const char *pKeyword,
				 const char *pResponse, int nRespLen,
				 std::vector<char> &vec);

bool Pass_CmdResp(const char *pCmdRespHead, int nCmdRespLen,
	              U32 *pTarIP_BE, U16 *pTarPort_BE,
	              std::vector<char> &vec);

bool Parse_CmdResp(const char *pCmdRespHead, int nCmdRespLen,
	               long long *pCmdID,
				   const char **ppResp, int *pRespLen);

/* AV allocation info from LocalServer to Client telling where to PULL these av streams (dynamic channels)
(optional) and to PUSH the reserved av stream (Res., for talks)
NOTE that local access to av streams is represented as a string of two types:
~ IP streams. The string then is the URL.
~ LNN data streams. The string is like "LNN:UDP:[PassStr]" where UDP/TCP is the trans. prot. and [PassStr] is the name
of the emitting node generated randomly by LocalServer.

notice\0type=LS2C_AVPU;
LocalRes=[LocalAccessString(ending with '\0')]; (optional)
AV={[AVType('0'-N.A; '1'-audio; '2'-vidio; '3'-AV]+[LocalAccessString(ending with '\0')]
+[chName(ending with '\0')]} + ... + {...}; (15 channels. Null strings indicate unavaible source)
*/
bool Gen_LS2C_AVPU(const OneChAVInfo *pLocalRes, // one channel
	               const OneChAVInfo *pLocalDynamicChs, // MAX_AUDIOVIDEO_CHANNELS
	               std::vector<char> &vec);

bool Parse_LS2C_AVPU(const char *pStr, int nStrLen,
	                 OneChAVInfo *pLocalRes, // one channel
					 OneChAVInfo *pLocalDynamicChs // MAX_AUDIOVIDEO_CHANNELS
					 );

/* AV src/allocation info from LocalServer to LSMs telling where to PULL these av streams (dynamic channels)
(optional) and to PUSH the reserved av stream (Res., for talks)
NOTE that local access to av streams is represented as a string of two types:
~ IP streams. The string then is the URL.
~ LNN data streams. The string is like "LNN:UDP:[PassStr]" where UDP/TCP is the trans. prot. and [PassStr] is the name
of the emitting node generated randomly by LocalServer.

notice\0type=LS2LSM_AVPU;
RemoteRes=[UIP(big-endian)]+[port(big-endian)]+[vPass(ending with '\0')]; (optional)
LocalRes=[LocalAccessString(ending with '\0')];
AV={[AVType('0'-N.A; '1'-audio; '2'-vidio; '3'-AV]
+[Device Type('1' - IP device; '2' - USB/AUX microphone]
+[LocalAccessString(ending with '\0')]+[bandwidth req.(4 bytes)]
+[UIP(big-endian)]+[port(big-endian)]+[vPass(ending with '\0')]} + ... + {...}; (15 channels)
*/
bool Gen_LS2LSM_AVPU(const OneChAllocation *pRemoteRes, // single channel researved for talks (CAN BE NULL)
	                 const OneChAVInfo *pLocalRes, // single channel researved for talks
	                 const OneChAVInfo *pAccessInfoChannels, // MAX_AUDIOVIDEO_CHANNELS
      	             const OneChAllocation *pRemoteAllocChannels, // MAX_AUDIOVIDEO_CHANNELS
					 std::vector<char> &vec);

bool Parse_LS2LSM_AVPU(const char *pStr, int nStrLen,
	                   OneChAllocation *pRemoteRes,
					   OneChAVInfo *pLocalRes,
	                   OneChAVInfo *pAccessInfoChannels,
					   OneChAllocation *pRemoteAllocChannels);

/* Access and monitor/client count info from LocalServer to Monitors
notice\0
type=LS2C_ACCPU;
bConnected2RS=1; ('0' - not connect to remote server; '1' - connected)
bOp=1; ('0' - not Op; '1' - Op)
nMonitors=[#monitors(4 bytes)];
nClients=[#clients(4 bytes)];
*/
bool Gen_LS2C_ACCPU(bool bOp, bool bRSConnected,
	                int nMonitors, int nClients,
					std::vector<char> &vec);

bool Parse_LS2C_ACCPU(const char *pStr, int nStrLen,
	                  bool *pbOp, bool *pbRSConnected,
					  int *pnMonitors, int *pnClients);

/* Access and monitor/client count info from LocalServer to LSMs
notice\0
type=LS2LSM_ACCPU;
bConnected2RS=1; ('0' - not connect to remote server; '1' - connected)
opMode=2; ('0' - not known; '1' - monitor Op.; '2' - client Op.)
nMonitors=[#monitors(4 bytes)];
nClients=[#clients(4 bytes)];
*/
bool Gen_LS2LSM_ACCPU(bool bRSConnected, 
				      int opMode, // 0 - unknown; 1 - monitor Op.; 2 - client Op.
					  int nMonitors, int nClients,
					  std::vector<char> &vec);

bool Parse_LS2LSM_ACCPU(const char *pStr, int nStrLen,
	                    bool *pbRSConnected, int *pOpMode,
						int *pnMonitors, int *pnClients);

}}

#endif // __DDRCOMMLIB_TALKS_INFOSTRING_PROCESSOR_H_INCLUDED__
