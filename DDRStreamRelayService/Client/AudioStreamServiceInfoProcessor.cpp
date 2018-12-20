#include "AudioStreamServiceInfoProcessor.h"
#include <memory>
#include "../../../Shared/proto/BaseCmd.pb.h"
#include "../../../Shared/src/Utility/DDRMacro.h"

#include "../../../Shared/src/Utility/Logger.h"
#include "GlobalManager.h"
using namespace DDRFramework;
using namespace DDRCommProto;

AudioStreamServiceInfoProcessor::AudioStreamServiceInfoProcessor(BaseMessageDispatcher& dispatcher) :BaseProcessor(dispatcher)
{ 
}


AudioStreamServiceInfoProcessor::~AudioStreamServiceInfoProcessor()
{
}

void AudioStreamServiceInfoProcessor::Process(std::shared_ptr<BaseSocketContainer> spSockContainer, std::shared_ptr<CommonHeader> spHeader, std::shared_ptr<google::protobuf::Message> spMsg)
{

	rspAudioStreamServiceInfo* pRaw = reinterpret_cast<rspAudioStreamServiceInfo*>(spMsg.get());


	//std::vector<AVChannel> channels;
	//for (auto channel : pRaw->channels())
	//{
	//	channels.push_back(channel);
	//}

	//GlobalManager::Instance()->GetTcpServer()->StartAudio(channels);

}