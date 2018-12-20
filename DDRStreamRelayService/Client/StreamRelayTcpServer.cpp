#include "StreamRelayTcpServer.h"
#include "../../../Shared/src/Network/TcpSocketContainer.h"
#include "../../../Shared/src/Network/MessageSerializer.h"
#include "../../../Shared/src/Network/BaseMessageDispatcher.h"

#include "../../../Shared/src/Utility/XmlLoader.h"
#include "StreamRelayService.h"
#include "DDRAudioTranscode.h"
#include "DDRStreamPlay.h"
StreamRelayTcpSession::StreamRelayTcpSession(asio::io_context& context) : DDRFramework::TcpSessionBase(context)
{

}
StreamRelayTcpSession::~StreamRelayTcpSession()
{

}


void StreamRelayTcpSession::CheckWrite()
{

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


			AudioTranscode at;

			string srcIP;
			string remotedstIP;
			string localdstIP;



			for (auto channel : channels)
			{
				if (channel.type() == AVChannel_ChannelType_Local)
				{
					srcIP = channel.srcip();
					localdstIP = channel.dstip();
				}
				else if (channel.type() == AVChannel_ChannelType_Remote)
				{

					srcIP = channel.srcip();
					remotedstIP = channel.dstip();
				}
			}

			if (at.Init(srcIP.c_str(), remotedstIP.c_str(), localdstIP.c_str()))
			{
				//printf("音频设备初始化失败，请检查！\n");
				//continue;
				at.Launch();
				at.Respond();


				StreamPlay sp;
				bool spInit = false;
				if (!localdstIP.empty())
				{

					spInit = sp.Init(localdstIP.c_str());
				}
				else if (!remotedstIP.empty())
				{


					spInit = sp.Init(remotedstIP.c_str());
				}

				if (spInit)
				{
					sp.Launch();
				}


			}

		}

	
}