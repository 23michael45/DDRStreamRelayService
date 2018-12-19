#include "LocalClientDispatcher.h"
#include "../../../Shared/proto/BaseCmd.pb.h"

#include "LoginProcessor.h"
#include "VideoStreamInfoProcessor.h"
#include "AudioStreamInfoProcessor.h"

using namespace DDRCommProto;
using namespace DDRFramework;

LocalClientDispatcher::LocalClientDispatcher()
{

	RegisterProcessor(rsp, Login)
	RegisterProcessor(rsp, VideoStreamInfo)
	RegisterProcessor(rsp, AudioStreamInfo)

}


LocalClientDispatcher::~LocalClientDispatcher()
{
}
