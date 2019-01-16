
#include <memory>
#include "../../../Shared/proto/BaseCmd.pb.h"
#include "CmdMoveProcessor.h"
#include "../../../Shared/src/Utility/DDRMacro.h"
#include "../../../Shared/src/Utility/Logger.h"
#include "../Client/GlobalManager.h"

using namespace DDRFramework;
using namespace DDRCommProto;

CmdMoveProcessor::CmdMoveProcessor(BaseMessageDispatcher& dispatcher) :BaseProcessor(dispatcher)
{
}


CmdMoveProcessor::~CmdMoveProcessor()
{
}

void CmdMoveProcessor::Process(std::shared_ptr<BaseSocketContainer> spSockContainer, std::shared_ptr<CommonHeader> spHeader, std::shared_ptr<google::protobuf::Message> spMsg)
{

	reqCmdMove* pRaw = reinterpret_cast<reqCmdMove*>(spMsg.get());
	if (pRaw)
	{

		auto sprsp = std::make_shared<rspCmdMove>();
		sprsp->set_type(eCmdRspType::eSuccess);


		spSockContainer->SendBack(spHeader, sprsp);

	}

}
