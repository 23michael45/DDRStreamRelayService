#ifndef AudioStreamInfoProcessor_h__
#define AudioStreamInfoProcessor_h__


#include "../../../Shared/src/Network/MessageSerializer.h"
#include "../../../Shared/src/Network/BaseProcessor.h"


using namespace DDRFramework;
class AudioStreamInfoProcessor : public BaseProcessor
{
public:
	AudioStreamInfoProcessor(BaseMessageDispatcher& dispatcher);
	~AudioStreamInfoProcessor();

	virtual void Process(std::shared_ptr<BaseSocketContainer> spSockContainer, std::shared_ptr<DDRCommProto::CommonHeader> spHeader, std::shared_ptr<google::protobuf::Message> spMsg) override;

private:
};

#endif // AudioStreamInfoProcessor_h__
