#ifndef StreamRelayTcpServer_h__
#define StreamRelayTcpServer_h__

#include "../../../Shared/src/Network/TcpServerBase.h"
#include "../../../Shared/src/Utility/Singleton.h"

#include "../../../Shared/src/Utility/AudioCodec.h"
#include "../../../Shared/proto/BaseCmd.pb.h"

#include "asio.hpp"
using namespace DDRFramework;
using namespace DDRCommProto;

class StreamRelayTcpSession : public HookTcpSession
{
public:
	StreamRelayTcpSession(asio::io_context& context);
	~StreamRelayTcpSession();

	auto shared_from_base() {
		return std::static_pointer_cast<StreamRelayTcpSession>(shared_from_this());
	}


	void on_recv_frames(mal_device* pDevice, mal_uint32 frameCount, const void* pSamples);
	mal_uint32 on_send_frames(mal_device* pDevice, mal_uint32 frameCount, void* pSamples);


	virtual void OnStart() override;
	virtual void OnStop() override;


	virtual void OnHookReceive(asio::streambuf& buf) override;
	std::mutex& GetAudioRecvMutex()
	{
		return m_AudioRecvMutex;
	}
private:
	AudioCodec m_AudioCodec;



	std::mutex m_AudioRecvMutex;
	asio::streambuf m_AudioRecvBuf;

};


class StreamRelayTcpServer : public HookTcpServer
{
public:
	StreamRelayTcpServer(int port);
	~StreamRelayTcpServer();

	auto shared_from_base() {
		return std::static_pointer_cast<StreamRelayTcpServer>(shared_from_this());
	}

	virtual std::shared_ptr<TcpSessionBase> BindSerializerDispatcher() override;


	bool StartAudio(std::vector<AVChannelConfig>& channels);
	bool StartVideo(std::vector<AVChannelConfig>& channels);

protected:

};


#endif // StreamRelayTcpServer_h__