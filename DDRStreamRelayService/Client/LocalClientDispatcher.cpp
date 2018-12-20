#include "LocalClientDispatcher.h"
#include "../../../Shared/proto/BaseCmd.pb.h"

#include "LoginProcessor.h"
#include "VideoStreamServiceInfoProcessor.h"
#include "AudioStreamServiceInfoProcessor.h"

using namespace DDRCommProto;
using namespace DDRFramework;

LocalClientDispatcher::LocalClientDispatcher()
{

	RegisterProcessor(rsp, Login)
	RegisterProcessor(rsp, VideoStreamServiceInfo)
	RegisterProcessor(rsp, AudioStreamServiceInfo)

}


LocalClientDispatcher::~LocalClientDispatcher()
{
}
