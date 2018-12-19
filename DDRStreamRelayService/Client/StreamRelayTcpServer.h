#ifndef StreamRelayTcpServer_h__
#define StreamRelayTcpServer_h__

#include "../../../Shared/src/Network/TcpServerBase.h"
#include "../../../Shared/src/Utility/Singleton.h"
using namespace DDRFramework;




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

protected:

	void OnHookReceive(asio::streambuf& buf);

};


#endif // StreamRelayTcpServer_h__