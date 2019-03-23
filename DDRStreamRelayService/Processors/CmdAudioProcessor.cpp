

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
		auto sprsp = std::make_shared<rspCmdAudio>();

		auto spServer = GlobalManager::Instance()->GetTcpServer();
		if (spServer)
		{



			if (pRaw->audiop() == reqCmdAudio_eAudioOperational_eStart)
			{
				std::string content = pRaw->audiostr();
				int priority = pRaw->level();

				if (pRaw->type() == reqCmdAudio_eAudioMode_eFile)
				{

					spServer->StartPlayFile(content, priority);
				}
				else if (pRaw->type() == reqCmdAudio_eAudioMode_eTTS)
				{

					spServer->StartPlayTxt(content, priority);

				}

			}
			else if (pRaw->audiop() == reqCmdAudio_eAudioOperational_eStop)
			{

				spServer->GetAudioCodec().StopPlayBuf();
			}

			sprsp->set_type(eCmdRspType::eSuccess);
		}
		else
		{
			sprsp->set_type(eCmdRspType::eCmdFailed);

		}

		spSockContainer->Send(sprsp);
	}

}

void CmdAudioProcessor::AsyncProcess(std::shared_ptr<BaseSocketContainer> spSockContainer, std::shared_ptr<DDRCommProto::CommonHeader> spHeader, std::shared_ptr<google::protobuf::Message> spMsg)
{


}

