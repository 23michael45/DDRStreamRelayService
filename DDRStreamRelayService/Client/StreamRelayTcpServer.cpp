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
	}
}
void StreamRelayTcpSession::OnStop()
{
	if (m_spParentServer)
	{
		m_spParentServer->GetAudioCodec().SetTcpSession(nullptr);
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
	StartAudioDevice();
}
StreamRelayTcpServer::~StreamRelayTcpServer()
{
	StopAudioDevice();
	DebugLog("StreamRelayTcpServer Destroy")
}


std::shared_ptr<TcpSessionBase> StreamRelayTcpServer::BindSerializerDispatcher()
{
	BIND_IOCONTEXT_SERIALIZER_DISPATCHER(m_IOContext, StreamRelayTcpSession, MessageSerializer, BaseMessageDispatcher, BaseHeadRuleRouter)
		
	spStreamRelayTcpSession->SetParentServer(shared_from_base());
	return spStreamRelayTcpSession;
}

bool StreamRelayTcpServer::StartAudioDevice()
{

	DDVoiceInteraction::GetInstance()->Init();

	//do not use shared_from_base ,member don't give shared_ptr otherwisse it wont destruct correctly
	if (m_AudioCodec.Init())
	{
		m_AudioCodec.StartRecord();
		m_AudioCodec.StartPlay();
		return true;
	}
	else
	{
		return false;
	}
}
void StreamRelayTcpServer::StopAudioDevice()
{

	m_AudioCodec.StopRecord();
	m_AudioCodec.StopPlay();
	m_AudioCodec.Deinit();

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