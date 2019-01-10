#ifndef CmdAudioProcessor_h__
#define CmdAudioProcessor_h__


#include "../../../Shared/src/Network/MessageSerializer.h"
#include "../../../Shared/src/Network/BaseProcessor.h"


using namespace DDRFramework;
class CmdAudioProcessor : public BaseProcessor
{
public:
	CmdAudioProcessor(BaseMessageDispatcher& dispatcher);
	~CmdAudioProcessor();

	virtual void Process(std::shared_ptr<BaseSocketContainer> spSockContainer, std::shared_ptr<DDRCommProto::CommonHeader> spHeader, std::shared_ptr<google::protobuf::Message> spMsg) override;
	virtual void AsyncProcess(std::shared_ptr<BaseSocketContainer> spSockContainer, std::shared_ptr<DDRCommProto::CommonHeader> spHeader, std::shared_ptr<google::protobuf::Message> spMsg) override;
private:
};

#endif // CmdAudioProcessor_h__
