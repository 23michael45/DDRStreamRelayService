#include "StreamRelayTcpServer.h"
#include "../../../Shared/src/Network/TcpSocketContainer.h"
#include "../../../Shared/src/Network/MessageSerializer.h"
#include "../../../Shared/src/Network/BaseMessageDispatcher.h"

#include "../../../Shared/src/Utility/XmlLoader.h"
#include "StreamRelayService.h"
#include "DDRAudioTranscode.h"
#include "DDRStreamPlay.h"


StreamRelayTcpSession* p_gAudioTcpSession;

void on_recv_frames(mal_device* pDevice, mal_uint32 frameCount, const void* pSamples)
{
	mal_uint32 sampleCount = frameCount * pDevice->channels;

	std::shared_ptr<asio::streambuf> buf = std::make_shared<asio::streambuf>();

	std::ostream oshold(buf.get());
	oshold.write((const char*)pSamples, sampleCount * sizeof(mal_int16));
	oshold.flush();

	if (p_gAudioTcpSession)
	{
		p_gAudioTcpSession->Send(buf);
	}
}

mal_uint32 on_send_frames(mal_device* pDevice, mal_uint32 frameCount, void* pSamples)
{
	if (p_gAudioTcpSession)
	{
		std::lock_guard<std::mutex> lock(p_gAudioTcpSession->GetRecvMutex());
		mal_uint32 samplesToRead = frameCount * pDevice->channels;

		asio::streambuf& buf = p_gAudioTcpSession->GetRecvBuf();

		size_t len = buf.size();
		if (len < samplesToRead * sizeof(mal_int16))
		{

			memcpy(pSamples, buf.data().data(), len);
			buf.consume(len);

			return len / pDevice->channels;
		}
		else
		{

			memcpy(pSamples, buf.data().data(), samplesToRead * sizeof(mal_int16));
			buf.consume(samplesToRead * sizeof(mal_int16));
			return samplesToRead / pDevice->channels;
		}


	}



}


StreamRelayTcpSession::StreamRelayTcpSession(asio::io_context& context) : DDRFramework::TcpSessionBase(context)
{
}
StreamRelayTcpSession::~StreamRelayTcpSession()
{
	DebugLog("\nStreamRelayTcpSession Destroy")
}

void StreamRelayTcpSession::OnHookReceive(asio::streambuf& buf)
{
	std::lock_guard<std::mutex> lock(m_AudioRecvMutex);

	static int i = 0;
	i += buf.size();
	DebugLog("\n--------------------------------------------------------------------------------%i", i);

	std::ostream oshold(&m_AudioRecvBuf);
	oshold.write((const char*)buf.data().data(), buf.size());
	oshold.flush();
}

void StreamRelayTcpSession::OnStart()
{

	p_gAudioTcpSession = this;

	m_AudioCodec.Init(1, 16000, on_recv_frames, on_send_frames);
	m_AudioCodec.StartRecord();
	m_AudioCodec.StartPlay();
}
void StreamRelayTcpSession::OnStop()
{

	m_AudioCodec.StopRecord();
	m_AudioCodec.StopPlay();
}



StreamRelayTcpServer::StreamRelayTcpServer(int port):TcpServerBase(port)
{
}


StreamRelayTcpServer::~StreamRelayTcpServer()
{
	DebugLog("\nStreamRelayTcpServer Destroy")
}


std::shared_ptr<TcpSessionBase> StreamRelayTcpServer::GetTcpSessionBySocket(tcp::socket* pSocket)
{
	if (m_SessionMap.find(pSocket) != m_SessionMap.end())
	{
		return m_SessionMap[pSocket];
	}
	return nullptr;
}

std::map<tcp::socket*, std::shared_ptr<TcpSessionBase>>& StreamRelayTcpServer::GetTcpSocketContainerMap()
{
	return m_SessionMap;
}
std::shared_ptr<TcpSessionBase> StreamRelayTcpServer::BindSerializerDispatcher()
{
	BIND_IOCONTEXT_SERIALIZER_DISPATCHER(m_IOContext, StreamRelayTcpSession, MessageSerializer, BaseMessageDispatcher, BaseHeadRuleRouter)
		return spStreamRelayTcpSession;
}
std::shared_ptr<TcpSessionBase> StreamRelayTcpServer::StartAccept()
{
	auto spSession = TcpServerBase::StartAccept();

	auto spStreamRelayTcpSession = dynamic_pointer_cast<StreamRelayTcpSession>(spSession);
	spSession->BindOnHookReceive(std::bind(&StreamRelayTcpSession::OnHookReceive, spStreamRelayTcpSession,std::placeholders::_1));

	return spSession;
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
		DebugLog("\n初始化失败，输入输出名称有误，请检查！\n");
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