#ifndef AudioTalkProcessor_h__
#define AudioTalkProcessor_h__


#include "../../../Shared/src/Network/MessageSerializer.h"
#include "../../../Shared/src/Network/BaseProcessor.h"


using namespace DDRFramework;
class AudioTalkProcessor : public BaseProcessor
{
public:
	AudioTalkProcessor(BaseMessageDispatcher& dispatcher);
	~AudioTalkProcessor();

	virtual void Process(std::shared_ptr<BaseSocketContainer> spSockContainer, std::shared_ptr<DDRCommProto::CommonHeader> spHeader, std::shared_ptr<google::protobuf::Message> spMsg) override;
	virtual void AsyncProcess(std::shared_ptr<BaseSocketContainer> spSockContainer, std::shared_ptr<DDRCommProto::CommonHeader> spHeader, std::shared_ptr<google::protobuf::Message> spMsg) override;
private:
};

#endif // AudioTalkProcessor_h__
