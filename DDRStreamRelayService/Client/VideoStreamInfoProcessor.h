#ifndef VideoStreamInfoProcessor_h__
#define VideoStreamInfoProcessor_h__


#include "../../../Shared/src/Network/MessageSerializer.h"
#include "../../../Shared/src/Network/BaseProcessor.h"


using namespace DDRFramework;
class VideoStreamInfoProcessor : public BaseProcessor
{
public:
	VideoStreamInfoProcessor(BaseMessageDispatcher& dispatcher);
	~VideoStreamInfoProcessor();

	virtual void Process(std::shared_ptr<BaseSocketContainer> spSockContainer, std::shared_ptr<DDRCommProto::CommonHeader> spHeader, std::shared_ptr<google::protobuf::Message> spMsg) override;
	virtual void AsyncProcess(std::shared_ptr<BaseSocketContainer> spSockContainer, std::shared_ptr<DDRCommProto::CommonHeader> spHeader, std::shared_ptr<google::protobuf::Message> spMsg) override;
private:
};

#endif // VideoStreamInfoProcessor_h__
