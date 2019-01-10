

#include <memory>
#include "../../../Shared/proto/BaseCmd.pb.h"
#include "CmdAudioProcessor.h"
#include "../../../Shared/src/Utility/DDRMacro.h"
#include "../../../Shared/src/Utility/Logger.h"
#include "../Client/GlobalManager.h"

using namespace DDRFramework;
using namespace DDRCommProto;

CmdAudioProcessor::CmdAudioProcessor(BaseMessageDispatcher& dispatcher) :BaseProcessor(dispatcher)
{
}


CmdAudioProcessor::~CmdAudioProcessor()
{
}

void CmdAudioProcessor::Process(std::shared_ptr<BaseSocketContainer> spSockContainer, std::shared_ptr<CommonHeader> spHeader, std::shared_ptr<google::protobuf::Message> spMsg)
{

	reqCmdAudio* pRaw = reinterpret_cast<reqCmdAudio*>(spMsg.get());
	if (pRaw)
	{
		if (pRaw->audiop() == reqCmdAudio_eAudioOperational_eStart)
		{
			std::string content = pRaw->audiostr();
			int priority = pRaw->level();

			if (pRaw->type() == reqCmdAudio_eAudioMode_eFile)
			{

				GlobalManager::Instance()->GetTcpServer()->StartPlayTxt(content, priority);
			}
			else if (pRaw->type() == reqCmdAudio_eAudioMode_eTTS)
			{

				GlobalManager::Instance()->GetTcpServer()->StartPlayFile(content, priority);

			}

		}
		else if (pRaw->audiop() == reqCmdAudio_eAudioOperational_eStop)
		{

			GlobalManager::Instance()->GetTcpServer()->GetAudioCodec().StopPlayBuf();
		}
	}

}

void CmdAudioProcessor::AsyncProcess(std::shared_ptr<BaseSocketContainer> spSockContainer, std::shared_ptr<DDRCommProto::CommonHeader> spHeader, std::shared_ptr<google::protobuf::Message> spMsg)
{


}

