#ifndef StreamServiceInfoProcessor_h__
#define StreamServiceInfoProcessor_h__




#include "../../../Shared/src/Network/MessageSerializer.h"
#include "../../../Shared/src/Network/BaseProcessor.h"


using namespace DDRFramework;
class StreamServiceInfoProcessor : public BaseProcessor
{
public:
	StreamServiceInfoProcessor(BaseMessageDispatcher& dispatcher);
	~StreamServiceInfoProcessor();

	virtual void Process(std::shared_ptr<BaseSocketContainer> spSockContainer, std::shared_ptr<DDRCommProto::CommonHeader> spHeader, std::shared_ptr<google::protobuf::Message> spMsg) override;
	virtual void AsyncProcess(std::shared_ptr<BaseSocketContainer> spSockContainer, std::shared_ptr<DDRCommProto::CommonHeader> spHeader, std::shared_ptr<google::protobuf::Message> spMsg) override;
private:
};

#endif // StreamServiceInfoProcessor_h__
