#include "StreamRelayTcpServer.h"
#include "../../../Shared/src/Network/TcpSocketContainer.h"
#include "../../../Shared/src/Network/MessageSerializer.h"
#include "../../../Shared/src/Network/BaseMessageDispatcher.h"

#include "../../../Shared/src/Utility/XmlLoader.h"
#include "StreamRelayService.h"
#include "DDRAudioTranscode.h"
#include "DDRStreamPlay.h"


StreamRelayTcpSession::StreamRelayTcpSession(asio::io_context& context) : DDRFramework::HookTcpSession(context)
{
	DebugLog("StreamRelayTcpSession Create")
}
StreamRelayTcpSession::~StreamRelayTcpSession()
{
	DebugLog("StreamRelayTcpSession Destroy")
}


void StreamRelayTcpSession::on_recv_frames(mal_device* pDevice, mal_uint32 frameCount, const void* pSamples)
{
	mal_uint32 sampleCount = frameCount * pDevice->channels;

	std::shared_ptr<asio::streambuf> buf = std::make_shared<asio::streambuf>();

	std::ostream oshold(buf.get());
	oshold.write((const char*)pSamples, sampleCount * sizeof(mal_int16));
	oshold.flush();

	Send(buf);
	
}

mal_uint32 StreamRelayTcpSession::on_send_frames(mal_device* pDevice, mal_uint32 frameCount, void* pSamples)
{
	std::lock_guard<std::mutex> lock(m_AudioRecvMutex);
	mal_uint32 samplesToRead = frameCount * pDevice->channels;

	asio::streambuf& buf = m_AudioRecvBuf;

	size_t len = buf.size();
	if (len < samplesToRead * sizeof(mal_int16))
	{

		memcpy(pSamples, buf.data().data(), len);
		buf.consume(len);

		return len / sizeof(mal_int16) / pDevice->channels;
	}
	else
	{

		memcpy(pSamples, buf.data().data(), samplesToRead * sizeof(mal_int16));
		buf.consume(samplesToRead * sizeof(mal_int16));
		return samplesToRead / pDevice->channels;
	}


}

void StreamRelayTcpSession::OnHookReceive(asio::streambuf& buf)
{
	DebugLog("Buf Size %i", buf.size());

	std::lock_guard<std::mutex> lock(m_AudioRecvMutex);
	std::ostream oshold(&m_AudioRecvBuf);
	oshold.write((const char*)buf.data().data(), buf.size());
	oshold.flush();
}

void StreamRelayTcpSession::OnStart()
{
	//do not use shared_from_base ,member don't give shared_ptr otherwisse it wont destruct correctly
	m_AudioCodec.Init(1, 16000, std::bind(&StreamRelayTcpSession::on_recv_frames,this,std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), std::bind(&StreamRelayTcpSession::on_send_frames,this,std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	m_AudioCodec.StartRecord();
	m_AudioCodec.StartPlay();
}
void StreamRelayTcpSession::OnStop()
{

	m_AudioCodec.StopRecord();
	m_AudioCodec.StopPlay();
	m_AudioCodec.Deinit();
}


StreamRelayTcpServer::StreamRelayTcpServer(int port) : HookTcpServer(port)
{

}
StreamRelayTcpServer::~StreamRelayTcpServer()
{

	DebugLog("StreamRelayTcpServer Destroy")
}
std::shared_ptr<TcpSessionBase> StreamRelayTcpServer::BindSerializerDispatcher()
{
	BIND_IOCONTEXT_SERIALIZER_DISPATCHER(m_IOContext, StreamRelayTcpSession, MessageSerializer, BaseMessageDispatcher, BaseHeadRuleRouter)
		return spStreamRelayTcpSession;
}

bool StreamRelayTcpServer::StartVideo(std::vector<AVChannelConfig>& channels)
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
bool StreamRelayTcpServer::StartAudio(std::vector<AVChannelConfig>& channels)
{
	
		
		
	return false;
	
}