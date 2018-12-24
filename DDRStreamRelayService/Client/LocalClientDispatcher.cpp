#include "LocalClientDispatcher.h"
#include "../../../Shared/proto/BaseCmd.pb.h"

#include "LoginProcessor.h"
#include "StreamServiceInfoProcessor.h"
#include "StreamServiceInfoChangedProcessor.h"
#include "FileStatusProcessor.h"

using namespace DDRCommProto;
using namespace DDRFramework;

LocalClientDispatcher::LocalClientDispatcher()
{

	RegisterProcessor(rsp, Login)
	RegisterProcessor(rsp, StreamServiceInfo)
	RegisterProcessor(notify, StreamServiceInfoChanged)
	RegisterProcessor(chk, FileStatus)

}


LocalClientDispatcher::~LocalClientDispatcher()
{
}
