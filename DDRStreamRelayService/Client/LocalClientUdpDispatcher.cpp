#include "LocalClientUdpDispatcher.h"
#include "../Processors/LocalClientUdpProcessor.h"
#include "../../../Shared/proto/BaseCmd.pb.h"
#include "../Processors/LoginProcessor.h"


using namespace DDRCommProto;
LocalClientUdpDispatcher::LocalClientUdpDispatcher()
{

	bcLSAddr bcClientInfo;
	m_ProcessorMap[bcClientInfo.GetTypeName()] = std::make_shared<LocalClientUdpProcessor>(*this);
}


LocalClientUdpDispatcher::~LocalClientUdpDispatcher()
{
	DebugLog("LocalClientUdpDispatcher Destroy");
}
