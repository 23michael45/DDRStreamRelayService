#include "StreamRelayTcpServer.h"
#include "../../../Shared/src/Network/TcpSocketContainer.h"
#include "../../../Shared/src/Network/MessageSerializer.h"
#include "../../../Shared/src/Network/BaseMessageDispatcher.h"

#include "../../../Shared/src/Utility/XmlLoader.h"
#include "StreamRelayService.h"
#include "DDRAudioTranscode.h"
#include "DDRStreamPlay.h"



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
	mal_uint32 samplesToRead = frameCount * pDevice->channels;
	/*if (samplesToRead > capturedSampleCount - playbackSample) {
		samplesToRead = capturedSampleCount - playbackSample;
	}

	if (samplesToRead == 0) {
		return 0;
	}

	memcpy(pSamples, pCapturedSamples + playbackSample, samplesToRead * sizeof(mal_int16));
	playbackSample += samplesToRead;*/

	return samplesToRead / pDevice->channels;
}


StreamRelayTcpSession::StreamRelayTcpSession(asio::io_context& context) : DDRFramework::TcpSessionBase(context)
{
	m_AudioCodec.Init(2, 48000, on_recv_frames,on_send_frames);
	//m_AudioCodec.StartRecord();
	m_AudioCodec.StartPlay();
}
StreamRelayTcpSession::~StreamRelayTcpSession()
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


std::shared_ptr<TcpSessionBase> StreamRelayTcpServer::BindSerializerDispatcher()
{
	BIND_IOCONTEXT_SERIALIZER_DISPATCHER(m_IOContext, TcpSessionBase, MessageSerializer, BaseMessageDispatcher, BaseHeadRuleRouter)
		return spTcpSessionBase;
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

std::shared_ptr<TcpSessionBase> StreamRelayTcpServer::StartAccept()
{
	auto spSession = TcpServerBase::StartAccept();
	spSession->BindOnHookReceive(std::bind(&StreamRelayTcpServer::OnHookReceive, shared_from_base(),std::placeholders::_1));
	return spSession;
}

void StreamRelayTcpServer::OnHookReceive(asio::streambuf& buf)
{
	static int i = 0;
	i += buf.size();
	DebugLog("\n%i",i );
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