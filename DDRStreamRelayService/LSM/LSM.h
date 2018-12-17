#ifndef __DDRCOMMLIB_MARS_LOCAL_SERVICE_MODULE_WRAPPER_H_INCLUDED__
#define __DDRCOMMLIB_MARS_LOCAL_SERVICE_MODULE_WRAPPER_H_INCLUDED__

#include <vector>
#include <string>

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

// communication functionalities for LocalServiceModules (LSM)
class LSMComm {
public:
	LSMComm(const char *pModuleName,
		    const char *pLocalServerName);
	bool EnableCmdRcv();
	bool EnableCmdSnd();
	bool EnableCmdRespSnd();
	bool EnableCmdRespRcv();
	bool EnableAlarmRcv();
	bool EnableAlarmSnd();
	bool EnableStatusRcv();
	bool EnableStatusSnd();
	bool EnableFileInq();
	bool EnableAVStreamHandling(bool bAlwaysAnswerTalk);
	bool Start();
	~LSMComm();

	int GetOpMode() const; // 1 - monitor Op.; 2 - client Op.
	int GetNumOfMonitors() const;
	int GetNumOfClients() const;
	bool GetPushInfo(_oneAVChannel2Push *pStreamPushingInfo // 15 channels
		             );
	bool GetPullInfo(int &avType, std::string &str);

	// 0 - connected and logged in
	// 1 - not configured or server address not obtained
	// 2 - server address obtained, but socket not create
	// 3 - configured and socket created, not connecting
	// 4 - connecting
	// 5 - connected, logging in
	int GetConnStat() const;

	// 0 - silent
	// 1 - calling
	// 2 - talking
	// 3 - being called to answer
	// 4 - pending answer
	// 5 - answering
	int Talk_GetState() const;
	bool Talk_Call();
	bool Talk_PickUp();
	bool Talk_HangUp();
	
	// Send status update to monitors/clients.
	bool SendStatusUpdate(const char *pStatusBytes, int nStatusLen);
	// Send alarms to monitors/clients.
	bool SendAlarmUpdate(const char *pAlarmBytes, int nAlarmLen);
	// Send command to other LSMs, example:
	// char x[8]; *((float*)x) = 1.0f; *((float*)(x + 4)) = 0.0f;
	// SendCommand("Move", x, 8, 123456789123456789);
	bool SendCommand(const char *pCmdKeyword,
		             const char *pCmdArgs, int nCmdArgLen,
					 long long cmdID);
	// Update file contents / file|dir lists
	// to local clients and remote server
	// type: 0 - file contents; 1 - file list; 2 - dir list
	// pFileDirNames: file or directory name (wild cards allowed)
	// e.g.: OneRoute*\\Path*.txt|config*\\config*
	bool UpdateFileDir(int type, const char *pFileDirNames);

	// get next command response this LSM has received
	// response content in resp, and command ID in *pCmdId
	bool GetNextCmdResp(std::vector<char> &resp, long long *pCmdID);
	int GetNumMsg_CmdResp();
	// get next status message from other LSMs
	bool GetNextStatus(std::vector<char> &statusMsg);
	int GetNumMsg_Status();
	// get next alarm message from other LSMs
	bool GetNextAlarm(std::vector<char> &statusMsg);
	int GetNumMsg_Alarm();

	// CALLBACK_CmdProcessor: callback function to process command
	// return value: if this command is OKAY TO PROCESS
	// pCmdHead, nCmdLen: specify a command
	// respVec: return the response bytes (can be empty to indicate no response)
	// pArg: for other arguments
	// ProcessCommand() : fetch all commands from the queue and process them using CALLBACK_CmdProcessor
	/* EXAMPLE - function definition:
	bool ProcessCmd(const void *pCmdHead, int nCmdLen, void *pArg,
	                bool &bResp, std::vector<char> &respVec) {
	    respVec.resize(0);
	    DDR_V304_Object *pObj = (DDR_V304_Object*)pArg;
		if (5 + sizeof(float) * 2 == nCmdLen && 0 == strcmp(pCmdHead, "Move")) {
		    float linVel = *((float*)(pCmdHead + 5));
		    float angVel = *((float*)(pCmdHead + 5 + sizeof(float)));
			bool bSucc = pObj->RobotMove(linVel, angVel);
			bResp = true;
			if (bSucc) {
			    respVec.resize(14);
			    memcpy(&respVec[0], "MoveSuccessul!", 14);
			} else {
			    respVec.resize(11);
			    memcpy(&respVec[0], "MoveFailed!", 11);
			}
			return true;
		}
		return false;
	}
	Example - function calling:
	void DDR_V304_Object::AutoMode() {
		...
		m_LSMComm.ProcessCommand(ProcessCmd, (void*)(&this));
		...
	}
	*/
	typedef bool(*CALLBACK_CmdProcessor)(const void *pCmdHead, int nCmdLen, void *pArg,
		                                 bool &bResp, std::vector<char> &respVec);
	bool ProcessCommand(CALLBACK_CmdProcessor pCBCmdProcFunc, void *pArg);

private:
	struct __IMPL;
	__IMPL *m_pImp;
};

}}

#endif // __DDRCOMMLIB_MARS_LOCAL_SERVICE_MODULE_WRAPPER_H_INCLUDED__
