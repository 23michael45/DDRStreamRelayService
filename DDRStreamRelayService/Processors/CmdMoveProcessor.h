#ifndef CmdMoveProcessor_h__
#define CmdMoveProcessor_h__


#include "../../../Shared/src/Network/MessageSerializer.h"
#include "../../../Shared/src/Network/BaseProcessor.h"


using namespace DDRFramework;
class CmdMoveProcessor : public BaseProcessor
{
public:
	CmdMoveProcessor(BaseMessageDispatcher& dispatcher);
	~CmdMoveProcessor();

	virtual void Process(std::shared_ptr<BaseSocketContainer> spSockContainer, std::shared_ptr<DDRCommProto::CommonHeader> spHeader, std::shared_ptr<google::protobuf::Message> spMsg) override;
private:
};


#endif // CmdMoveProcessor_h__

