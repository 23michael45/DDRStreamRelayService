#ifndef StreamRelayTcpServer_h__
#define StreamRelayTcpServer_h__

#include "../../../Shared/src/Network/TcpServerBase.h"
#include "../../../Shared/src/Utility/Singleton.h"

#include "../../../Shared/src/Utility/AudioCodec.h"
#include "../../../Shared/proto/BaseCmd.pb.h"

#include "../../../Shared/src/Utility/Timer.hpp"
#include "../RobotChat/DDVoiceInteraction.h"
#include <map>
#include "asio.hpp"

using namespace DDRFramework;
using namespace DDRCommProto;

class StreamRelayTcpServer;

class StreamRelayTcpSession : public HookTcpSession
{
public:
	StreamRelayTcpSession(asio::io_context& context);
	~StreamRelayTcpSession();

	auto shared_from_base() {
		return std::static_pointer_cast<StreamRelayTcpSession>(shared_from_this());
	}


	virtual void OnStart() override;
	virtual void OnStop() override;


	virtual void OnHookReceive(asio::streambuf& buf) override;

	void SetParentServer(std::shared_ptr<StreamRelayTcpServer> sp)
	{
		m_spParentServer = sp;
	}


private:
	std::shared_ptr<StreamRelayTcpServer> m_spParentServer;
};


class StreamRelayTcpServer : public HookTcpServer
{

public:
	StreamRelayTcpServer(rspStreamServiceInfo& info);
	~StreamRelayTcpServer();

	auto shared_from_base() {
		return std::static_pointer_cast<StreamRelayTcpServer>(shared_from_this());
	}


	virtual std::shared_ptr<TcpSessionBase> BindSerializerDispatcher() override;

	bool StartAudioDevice();
	void StopAudioDevice();

	bool StartRemoteAudio(std::vector<AVChannelConfig>& channels);
	bool StartRemoteVideo(std::vector<AVChannelConfig>& channels);


	void StopRemoteAudio() {};
	void StopRemoteVideo() {};

	AudioCodec& GetAudioCodec()
	{
		return m_AudioCodec;
	};

protected:

	AudioCodec m_AudioCodec;

	DDRFramework::Timer m_Timer;


};


#endif // StreamRelayTcpServer_h__