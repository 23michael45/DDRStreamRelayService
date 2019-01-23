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

	SHARED_FROM_BASE(StreamRelayTcpSession)

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

	SHARED_FROM_BASE(StreamRelayTcpServer)

	virtual std::shared_ptr<TcpSessionBase> BindSerializerDispatcher() override;

	bool StartAudioDevice();
	void StopAudioDevice();

	bool StartRemoteAudio(std::vector<AVChannelConfig>& channels);
	bool StartRemoteVideo(std::vector<AVChannelConfig>& channels);


	void StopRemoteAudio() {};
	void StopRemoteVideo() {};


	void StartPlayTxt(std::string& content,int priority);
	void StartPlayFile(std::string& filename, int prioritye);




	bool PlayAudio(std::shared_ptr<asio::streambuf>, int priority);

	void PushQueue(std::shared_ptr<WavBufInfo> spinfo, int priority);
	void PopQueue(std::shared_ptr<WavBufInfo> spinfo);
	std::shared_ptr<WavBufInfo> GetQueueNextAudio();

	void OnWaveFinish(std::shared_ptr<WavBufInfo> spInfo);


	AudioCodec& GetAudioCodec()
	{
		return m_AudioCodec;
	};

protected:

	AudioCodec m_AudioCodec;

	DDRFramework::Timer m_Timer;


	struct greater
	{
		bool operator()(const int& _Left, const int& _Right) const
		{
			return (_Left > _Right);
		}
	};

	std::map<int, std::shared_ptr<std::queue<std::shared_ptr<WavBufInfo>>>, greater> m_AudioQueueMap;
	std::mutex m_AudioQueueMutex;
};


#endif // StreamRelayTcpServer_h__