#include "LocalClientDispatcher.h"
#include "../../../Shared/proto/BaseCmd.pb.h"

#include "LoginProcessor.h"

using namespace DDRCommProto;
using namespace DDRFramework;

LocalClientDispatcher::LocalClientDispatcher()
{

	respLogin respLogin;
	m_ProcessorMap[respLogin.GetTypeName()] =  std::make_shared<LoginProcessor>(*this);
}


LocalClientDispatcher::~LocalClientDispatcher()
{
}
