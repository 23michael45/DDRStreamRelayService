#include "StreamRelayTcpServer.h"
#include "../../../Shared/src/Network/TcpSocketContainer.h"
#include "../../../Shared/src/Network/MessageSerializer.h"
#include "../../../Shared/src/Network/BaseMessageDispatcher.h"

#include "../../../Shared/src/Utility/XmlLoader.h"
#include "StreamRelayService.h"
#include "DDRAudioTranscode.h"
#include "DDRStreamPlay.h"
#include "RobotChat/DDVoiceInteraction.h"


StreamRelayTcpSession::StreamRelayTcpSession(asio::io_context& context) : DDRFramework::HookTcpSession(context)
{
	DebugLog("StreamRelayTcpSession Create")
}
StreamRelayTcpSession::~StreamRelayTcpSession()
{
	DebugLog("StreamRelayTcpSession Destroy")
}



void StreamRelayTcpSession::OnHookReceive(asio::streambuf& buf)
{
	if (m_spParentServer)
	{
		m_spParentServer->GetAudioCodec().PushAudioRecvBuf(buf);

	}
}

void StreamRelayTcpSession::OnStart()
{
	if(m_spParentServer)
	{
		m_spParentServer->GetAudioCodec().SetTcpSession(shared_from_base());


		m_spParentServer->StartAudioDevice();
	}

}
void StreamRelayTcpSession::OnStop()
{
	if (m_spParentServer)
	{
		m_spParentServer->GetAudioCodec().SetTcpSession(nullptr);


		m_spParentServer->StopAudioDevice();
	}

}





StreamRelayTcpServer::StreamRelayTcpServer(rspStreamServiceInfo& info) : HookTcpServer(info.tcpport())
{
	std::vector<AVChannelConfig> Local_VideoChannels;
	std::vector<AVChannelConfig> Remote_VideoChannels;
	std::vector<AVChannelConfig> Local_AudioChannels;
	std::vector<AVChannelConfig> Remote_AudioChannels;
	std::vector<AVChannelConfig> Local_AVChannels;
	std::vector<AVChannelConfig> Remote_AVChannels;

	for (auto channel : info.channels())
	{
		if (channel.networktype() == ChannelNetworkType::Local)
		{
			if (channel.streamtype() == ChannelStreamType::Video)
			{
				Local_VideoChannels.push_back(channel);
			}
			if (channel.streamtype() == ChannelStreamType::Audio)
			{
				Local_AudioChannels.push_back(channel);
			}
			if (channel.streamtype() == ChannelStreamType::VideoAudio)
			{
				Local_AVChannels.push_back(channel);
			}

		}
		else if (channel.networktype() == ChannelNetworkType::Remote)
		{
			if (channel.streamtype() == ChannelStreamType::Video)
			{
				Remote_VideoChannels.push_back(channel);
			}
			if (channel.streamtype() == ChannelStreamType::Audio)
			{
				Remote_AudioChannels.push_back(channel);
			}
			if (channel.streamtype() == ChannelStreamType::VideoAudio)
			{
				Remote_AVChannels.push_back(channel);
			}
		}
		//StartRemoteVideo(Remote_VideoChannels);
		//StartRemoteAudio(Remote_AudioChannels);

	}

	m_AudioCodec.BindOnFinishPlayWave(std::bind(&StreamRelayTcpServer::OnWaveFinish,this,std::placeholders::_1));
}
StreamRelayTcpServer::~StreamRelayTcpServer()
{
	while (m_AudioCodec.IsPlayingWave())
	{

		std::this_thread::sleep_for(chrono::milliseconds(1));
	}
	StopAudioDevice();
	DebugLog("StreamRelayTcpServer Destroy")
}

bool StreamRelayTcpServer::StartRemoteVideo(std::vector<AVChannelConfig>& channels)
{
	std::vector<std::string> inputIPs;
	std::vector<std::string> outputIPs;


	for (auto channel : channels)
	{
		inputIPs.push_back(channel.src());
		outputIPs.push_back(channel.dst());

	}

	StreamRelayService src(channels.size());
	int ret = src.Init(inputIPs, outputIPs);

	if (ret == STREAM_INIT_ERROR_INPUT_OUTPUT_SIZE
		|| ret == STREAM_INIT_ERROR_OUTPUT_NAME_DUPLICATED)
	{
		DebugLog("Stream Rtmp Service Init Failed");
		return false;
	}
	else
	{
		src.Launch();
		src.Respond();

	}
}
bool StreamRelayTcpServer::StartRemoteAudio(std::vector<AVChannelConfig>& channels)
{
	AudioTranscode at;

	for (auto channel : channels)
	{
		if (!at.Init(channel.src().c_str(), channel.dst().c_str()))
		{
			DebugLog("Audio Device Init Failed!");
			return false;
		}
		at.Launch();
		at.Respond();

		break;;
	}

	return true;

}

std::shared_ptr<TcpSessionBase> StreamRelayTcpServer::BindSerializerDispatcher()
{
	BIND_IOCONTEXT_SERIALIZER_DISPATCHER(m_IOContext, StreamRelayTcpSession, MessageSerializer, BaseMessageDispatcher, BaseHeadRuleRouter)
		
	spStreamRelayTcpSession->SetParentServer(shared_from_base());
	return spStreamRelayTcpSession;
}

bool StreamRelayTcpServer::StartAudioDevice()
{

	if (m_AudioCodec.Init())
	{
		m_AudioCodec.StartDeviceRecord();
		m_AudioCodec.StartDevicePlay();
		return true;
	}
	else
	{
		return false;
	}
}
void StreamRelayTcpServer::StopAudioDevice()
{

	m_AudioCodec.StopDeviceRecord();
	m_AudioCodec.StopDevicePlay();
	m_AudioCodec.Deinit();

}


void StreamRelayTcpServer::StartPlayTxt(std::string& content, int priority)
{
	auto spBuf = DDVoiceInteraction::Instance()->GetVoiceBuf(content);
	if (spBuf && PlayAudio(spBuf,priority))
	{

	}
}

void StreamRelayTcpServer::StartPlayFile(std::string& filename, int priority)
{
	std::shared_ptr<asio::streambuf> spBuf = std::make_shared<asio::streambuf>();
	std::ostream oshold(spBuf.get());


	std::ifstream ifs;
	ifs.open(filename.c_str(), std::ifstream::in);

	ifs.seekg(0, ifs.end);
	long size = ifs.tellg();
	ifs.seekg(0);
	char* buffer = new char[size];
	ifs.read(buffer, size);

	oshold.write(buffer, size);
	oshold.flush();

	delete[] buffer;
	ifs.close();


	if (PlayAudio(spBuf,priority))
	{

	}
}






bool StreamRelayTcpServer::PlayAudio(std::shared_ptr<asio::streambuf> spbuf, int priority)
{

	auto spInfo = std::make_shared<WavBufInfo>();
	spInfo->m_spBuf = spbuf;
	spInfo->m_CurrentFrame = 0;
	spInfo->m_Priority = priority;
	PushQueue(spInfo, priority);

	auto spNextInfo = GetQueueNextAudio();

	if (m_AudioCodec.IsPlayingWave())
	{
		auto spPreInfo = m_AudioCodec.StopPlayBuf();
	}

	if (m_AudioCodec.StartPlayBuf(spNextInfo))
	{
		return true;
	}
	return false;
}

void StreamRelayTcpServer::PushQueue(std::shared_ptr<WavBufInfo> spinfo,int priority)
{
	std::lock_guard<std::mutex> lock(m_AudioQueueMutex);

	if (m_AudioQueueMap.find(priority) == m_AudioQueueMap.end())
	{
		auto spQueue = std::make_shared<std::queue<std::shared_ptr<WavBufInfo>>>();
		m_AudioQueueMap.insert(make_pair(priority, spQueue));
	}
	if (spinfo)
	{
		auto spQueue = m_AudioQueueMap[priority];
		spQueue->push(spinfo);
	}


}

void StreamRelayTcpServer::PopQueue(std::shared_ptr<WavBufInfo> spinfo)
{
	std::lock_guard<std::mutex> lock(m_AudioQueueMutex);
	if (m_AudioQueueMap.find(spinfo->m_Priority) != m_AudioQueueMap.end())
	{
		m_AudioQueueMap[spinfo->m_Priority]->pop();
	}
}

std::shared_ptr<WavBufInfo> StreamRelayTcpServer::GetQueueNextAudio()
{
	std::lock_guard<std::mutex> lock(m_AudioQueueMutex);
	std::shared_ptr< WavBufInfo> spInfo;
	for (auto spQueuePair : m_AudioQueueMap)
	{
		auto spQueue = spQueuePair.second;
		if (spQueue->size() > 0)
		{
			spInfo = spQueue->front();
			return  spInfo;
		}
	}
	return  nullptr;
}

void StreamRelayTcpServer::OnWaveFinish(std::shared_ptr<WavBufInfo> spInfo)
{
	auto spFront = GetQueueNextAudio();
	if (spInfo == spFront)
	{
		PopQueue(spInfo);
	}
	m_AudioCodec.StopPlayBuf();

	auto spNextAudio = GetQueueNextAudio();
	if (spNextAudio)
	{
		m_AudioCodec.StartPlayBuf(spNextAudio);
	}
}
