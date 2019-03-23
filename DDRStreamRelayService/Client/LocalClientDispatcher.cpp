#include "LocalClientDispatcher.h"
#include "../../../Shared/proto/BaseCmd.pb.h"

#include "../Processors/LoginProcessor.h"
#include "../Processors/StreamServiceInfoProcessor.h"
#include "../Processors/StreamServiceInfoChangedProcessor.h"
#include "../Processors/FileStatusProcessor.h"
#include "../Processors/FileAddressProcessor.h"
#include "../Processors/CmdAudioProcessor.h"
#include "../Processors/AudioTalkProcessor.h"
#include "../Processors/UploadFileProcessor.h"
#include "../Processors/CmdMoveProcessor.h"

using namespace DDRCommProto;
using namespace DDRFramework;

LocalClientDispatcher::LocalClientDispatcher()
{

	RegisterProcessor(rsp, Login)
	RegisterProcessor(rsp, StreamServiceInfo)
	RegisterProcessor(notify, StreamServiceInfoChanged)
	RegisterProcessor(chk, FileStatus)
		//RegisterProcessor(req, FileAddress)
		RegisterProcessor(req, CmdAudio)

		RegisterProcessor(req, CmdMove)
		RegisterProcessor(req, AudioTalk)
		RegisterProcessor(notify, UploadFile)

}


LocalClientDispatcher::~LocalClientDispatcher()
{
}
