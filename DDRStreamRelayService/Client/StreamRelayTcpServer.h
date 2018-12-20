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

	AudioCodec m_AudioCodec;
};


class StreamRelayTcpServer : public TcpServerBase
{
public:
	StreamRelayTcpServer(int port);
	~StreamRelayTcpServer();


	virtual std::shared_ptr<TcpSessionBase> BindSerializerDispatcher();

	std::shared_ptr<TcpSessionBase> GetTcpSessionBySocket(tcp::socket* pSocket);
	std::map<tcp::socket*,std::shared_ptr<TcpSessionBase>>& GetTcpSocketContainerMap();


	auto shared_from_base() {
		return std::static_pointer_cast<StreamRelayTcpServer>(shared_from_this());
	}


	virtual std::shared_ptr<TcpSessionBase> StartAccept() override;


	bool StartAudio(std::vector<AVChannel>& channels);
	bool StartVideo(std::vector<AVChannel>& channels);

protected:

	void OnHookReceive(asio::streambuf& buf);



	AudioDeviceInquiry m_AudioDeviceInquiry;
	//zWalleAudio m_zWalleAudioAudio;
};


#endif // StreamRelayTcpServer_h__