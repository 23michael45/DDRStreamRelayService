#ifndef StreamRelayTcpServer_h__
#define StreamRelayTcpServer_h__

#include "../../../Shared/src/Network/TcpServerBase.h"
#include "../../../Shared/src/Utility/Singleton.h"

#include "../AudioDeviceInquiry.h"
#include "../../../Shared/src/Utility/AudioCodec.h"
#include "../../../Shared/proto/BaseCmd.pb.h"

#include "asio.hpp"
using namespace DDRFramework;
using namespace DDRCommProto;

class StreamRelayTcpSession : public TcpSessionBase
{
public:
	StreamRelayTcpSession(asio::io_context& context);
	~StreamRelayTcpSession();

	auto shared_from_base() {
		return std::static_pointer_cast<StreamRelayTcpSession>(shared_from_this());
	}

	std::mutex& GetRecvMutex()
	{
		return m_AudioRecvMutex;
	}

	asio::streambuf& GetRecvBuf()
	{
		return m_AudioRecvBuf;
	}
	virtual void OnStart() override;
	virtual void OnStop() override;
	void OnHookReceive(asio::streambuf& buf);

private:
	AudioCodec m_AudioCodec;

	std::mutex m_AudioRecvMutex;
	asio::streambuf m_AudioRecvBuf;

};


class StreamRelayTcpServer : public TcpServerBase
{
public:
	StreamRelayTcpServer(int port);
	~StreamRelayTcpServer();


	virtual std::shared_ptr<TcpSessionBase> BindSerializerDispatcher() override;

	std::shared_ptr<TcpSessionBase> GetTcpSessionBySocket(tcp::socket* pSocket);
	std::map<tcp::socket*,std::shared_ptr<TcpSessionBase>>& GetTcpSocketContainerMap();


	auto shared_from_base() {
		return std::static_pointer_cast<StreamRelayTcpServer>(shared_from_this());
	}


	virtual std::shared_ptr<TcpSessionBase> StartAccept() override;


	bool StartAudio(std::vector<AVChannelConfig>& channels);
	bool StartVideo(std::vector<AVChannelConfig>& channels);

protected:

};


#endif // StreamRelayTcpServer_h__