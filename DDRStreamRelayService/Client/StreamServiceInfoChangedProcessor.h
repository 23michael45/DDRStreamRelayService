#ifndef StreamServiceInfoChangedProcessor_h__
#define StreamServiceInfoChangedProcessor_h__





#include "../../../Shared/src/Network/MessageSerializer.h"
#include "../../../Shared/src/Network/BaseProcessor.h"


using namespace DDRFramework;
class StreamServiceInfoChangedProcessor : public BaseProcessor
{
public:
	StreamServiceInfoChangedProcessor(BaseMessageDispatcher& dispatcher);
	~StreamServiceInfoChangedProcessor();

	virtual void Process(std::shared_ptr<BaseSocketContainer> spSockContainer, std::shared_ptr<DDRCommProto::CommonHeader> spHeader, std::shared_ptr<google::protobuf::Message> spMsg) override;
	virtual void AsyncProcess(std::shared_ptr<BaseSocketContainer> spSockContainer, std::shared_ptr<DDRCommProto::CommonHeader> spHeader, std::shared_ptr<google::protobuf::Message> spMsg) override;
private:
};

#endif // StreamServiceInfoChangedProcessor_h__

