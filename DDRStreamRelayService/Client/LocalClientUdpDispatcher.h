#ifndef LocalClientUdpDispatcher_h__
#define LocalClientUdpDispatcher_h__


#include "../../../Shared/src/Network/BaseMessageDispatcher.h"


class LocalClientUdpDispatcher : public DDRFramework::BaseMessageDispatcher
{
public:
	LocalClientUdpDispatcher();
	~LocalClientUdpDispatcher();
};


#endif // LocalClientUdpDispatcher_h__