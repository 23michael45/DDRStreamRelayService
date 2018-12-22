#ifndef StreamRelayTcpClient_h__
#define StreamRelayTcpClient_h__

#include "../../../Shared/src/Network/TcpClientBase.h"
#include "../../../Shared/src/Utility/Singleton.h"
#include "../../../Shared/src/Utility/Timer.hpp"

using namespace DDRFramework;
class StreamRelayTcpClient : public TcpClientBase 
{
public:
	StreamRelayTcpClient();
	~StreamRelayTcpClient();

	void OnConnected(TcpSocketContainer& container) override;
	void OnDisconnect(TcpSocketContainer& container) override;

	virtual std::shared_ptr<TcpClientSessionBase> BindSerializerDispatcher();

	auto shared_from_base() {
		return std::static_pointer_cast<StreamRelayTcpClient>(shared_from_this());
	}


	void StartHeartBeat();
	void StopHeartBeat();

	void RequestStreamInfo();

private:
	void SendHeartBeatOnce(timer_id id);




	DDRFramework::Timer m_Timer;
	DDRFramework::timer_id m_HeartBeatTimerID;
};


#endif // StreamRelayTcpClient_h__
