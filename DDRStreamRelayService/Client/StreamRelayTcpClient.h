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

	void OnConnected(std::shared_ptr<TcpSocketContainer> spContainer) override;
	void OnDisconnect(std::shared_ptr<TcpSocketContainer> spContainer) override;

	virtual std::shared_ptr<TcpClientSessionBase> BindSerializerDispatcher();

	SHARED_FROM_BASE(StreamRelayTcpClient)


	void StartHeartBeat();
	void StopHeartBeat();

	void RequestStreamInfo();

private:
	void SendHeartBeatOnce(timer_id id);




	DDRFramework::Timer m_Timer;
	DDRFramework::timer_id m_HeartBeatTimerID;
};


#endif // StreamRelayTcpClient_h__
