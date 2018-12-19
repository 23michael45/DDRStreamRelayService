#ifndef LocalClientDispatcher_h__
#define LocalClientDispatcher_h__


#include "../../../Shared/src/Network/BaseMessageDispatcher.h"


class LocalClientDispatcher : public DDRFramework::BaseMessageDispatcher
{
public:
	LocalClientDispatcher();
	~LocalClientDispatcher();
};


#endif // LocalClientDispatcher_h__