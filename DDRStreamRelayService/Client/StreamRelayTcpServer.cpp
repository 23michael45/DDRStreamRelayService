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

	/*mal_uint32 newCapturedSampleCount = capturedSampleCount + sampleCount;
	mal_int16* pNewCapturedSamples = (mal_int16*)realloc(pCapturedSamples, newCapturedSampleCount * sizeof(mal_int16));
	if (pNewCapturedSamples == NULL) {
		return;
	}

	memcpy(pNewCapturedSamples + capturedSampleCount, pSamples, sampleCount * sizeof(mal_int16));

	pCapturedSamples = pNewCapturedSamples;
	capturedSampleCount = newCapturedSampleCount;*/
}

mal_uint32 on_send_frames(mal_device* pDevice, mal_uint32 frameCount, void* pSamples)
{
	if (p_gAudioTcpSession)
	{
		std::lock_guard<std::mutex> lock(p_gAudioTcpSession->GetRecvMutex());
		mal_uint32 samplesToRead = frameCount * pDevice->channels;

		asio::streambuf& buf = p_gAudioTcpSession->GetRecvBuf();

		size_t len = buf.size();
		if (len < samplesToRead)
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

}

void StreamRelayTcpSession::OnHookReceive(asio::streambuf& buf)
{
	std::lock_guard<std::mutex> lock(m_AudioRecvMutex);

	std::ostream oshold(&m_AudioRecvBuf);
	oshold.write((const char*)buf.data().data(), buf.size());
	oshold.flush();
}

void StreamRelayTcpSession::OnStart()
{

	p_gAudioTcpSession = this;

	m_AudioCodec.Init(2, 48000, on_recv_frames, on_send_frames);
	//m_AudioCodec.StartRecord();
	m_AudioCodec.StartPlay();
}
void StreamRelayTcpSession::OnStop()
{

	//m_AudioCodec.StopRecord();
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


bool StreamRelayTcpServer::StartVideo(std::vector<AVChannel>& channels)
{
	std::vector<std::string> inputIPs;
	std::vector<std::string> outputIPs;
	for (auto channel : channels)
	{
		inputIPs.push_back(channel.srcip());
		outputIPs.push_back(channel.dstip());

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
bool StreamRelayTcpServer::StartAudio(std::vector<AVChannel>& channels)
{
	
		m_AudioDeviceInquiry.Process();
		//Sleep(1000);
		auto audioDeivceIDs = m_AudioDeviceInquiry.GetAudioDeviceID();
		if (audioDeivceIDs.size() < 1)
		{
			DebugLog("\n无音频推流设备，请检查！\n");
			return false;
		}
		else
		{
			auto audioDeivceID = audioDeivceIDs[0];


			//AudioTranscode at;

			//string srcIP;
			//string remotedstIP;
			//string localdstIP;



			//for (auto channel : channels)
			//{
			//	if (channel.type() == AVChannel_ChannelType_Local)
			//	{
			//		srcIP = channel.srcip();
			//		localdstIP = channel.dstip();
			//	}
			//	else if (channel.type() == AVChannel_ChannelType_Remote)
			//	{

			//		srcIP = channel.srcip();
			//		remotedstIP = channel.dstip();
			//	}
			//}

			//if (at.Init(srcIP.c_str(), remotedstIP.c_str(), localdstIP.c_str()))
			//{
			//	//printf("音频设备初始化失败，请检查！\n");
			//	//continue;
			//	at.Launch();
			//	at.Respond();


			//	StreamPlay sp;
			//	bool spInit = false;
			//	if (!localdstIP.empty())
			//	{

			//		spInit = sp.Init(localdstIP.c_str());
			//	}
			//	else if (!remotedstIP.empty())
			//	{


			//		spInit = sp.Init(remotedstIP.c_str());
			//	}

			//	if (spInit)
			//	{
			//		sp.Launch();
			//	}


			//}

		}

	
}