

#include <memory>
#include "../../../Shared/proto/BaseCmd.pb.h"
#include "AudioTalkProcessor.h"
#include "../../../Shared/src/Utility/DDRMacro.h"
#include "../../../Shared/src/Utility/Logger.h"
#include "../Client/GlobalManager.h"

using namespace DDRFramework;
using namespace DDRCommProto;

AudioTalkProcessor::AudioTalkProcessor(BaseMessageDispatcher& dispatcher) :BaseProcessor(dispatcher)
{
}


AudioTalkProcessor::~AudioTalkProcessor()
{
}

void AudioTalkProcessor::Process(std::shared_ptr<BaseSocketContainer> spSockContainer, std::shared_ptr<CommonHeader> spHeader, std::shared_ptr<google::protobuf::Message> spMsg)
{

	reqAudioTalk* pRaw = reinterpret_cast<reqAudioTalk*>(spMsg.get());

	if (pRaw)
	{
		if (pRaw->optype() == reqAudioTalk_eOpMode_eStart)
		{
			if (pRaw->nettype() == reqAudioTalk_eNetType_eLocal)
			{

				GlobalManager::Instance()->GetTcpServer()->GetAudioCodec().SetTcpReceiveSession(spSockContainer->GetTcp());
			}
			else if (pRaw->nettype() == reqAudioTalk_eNetType_eRemote)
			{

			}

		}
		else if (pRaw->optype() == reqAudioTalk_eOpMode_eStop)
		{

			GlobalManager::Instance()->GetTcpServer()->GetAudioCodec().SetTcpReceiveSession(nullptr);
		}
	}
}

void AudioTalkProcessor::AsyncProcess(std::shared_ptr<BaseSocketContainer> spSockContainer, std::shared_ptr<DDRCommProto::CommonHeader> spHeader, std::shared_ptr<google::protobuf::Message> spMsg)
{


}
