

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
		auto sprsp = std::make_shared<rspAudioTalk>();
		if (pRaw->optype() == reqAudioTalk_eOpMode_eStart)
		{
			if (pRaw->nettype() == reqAudioTalk_eNetType_eLocal)
			{


				auto& codec = GlobalManager::Instance()->GetTcpServer()->GetAudioCodec();

				if (codec.HasTcpReceiveSession())
				{
					sprsp->set_status(eTalkStatus::ETS_USE_BY_OTHER);


				}
				else
				{

					auto spServer = GlobalManager::Instance()->GetTcpServer();
					if (spServer)
					{
						if (spHeader->passnodearray().size() > 0)
						{
							auto& HeadPassnode = spHeader->passnodearray(0);
							std::string fromIP = HeadPassnode.fromip();
							codec.SetTcpReceiveSessionIP(fromIP);
							sprsp->set_status(eTalkStatus::ETS_START_OK);
						}
						else
						{
							sprsp->set_status(eTalkStatus::ETS_NO_USER_CONNECTED_WITH_IP);

						}

					}


				}


			}
			else if (pRaw->nettype() == reqAudioTalk_eNetType_eRemote)
			{
				//to do 
				//sprsp->set_status(eTalkStatus::ETS_OK);
			}

		}
		else if (pRaw->optype() == reqAudioTalk_eOpMode_eStop)
		{

			GlobalManager::Instance()->GetTcpServer()->GetAudioCodec().SetTcpReceiveSessionIP();
			sprsp->set_status(eTalkStatus::ETS_STOP_OK);
		}

		spSockContainer->SendBack(spHeader, sprsp);
	}
}

void AudioTalkProcessor::AsyncProcess(std::shared_ptr<BaseSocketContainer> spSockContainer, std::shared_ptr<DDRCommProto::CommonHeader> spHeader, std::shared_ptr<google::protobuf::Message> spMsg)
{


}
