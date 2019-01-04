#include <memory>
#include "../../../Shared/proto/BaseCmd.pb.h"
#include "FileAddressProcessor.h"
#include "../../../Shared/src/Utility/DDRMacro.h"
#include "../../../Shared/src/Utility/Logger.h"
#include "GlobalManager.h"
#include "FileManager.h"
#include <regex>
#include "HttpFileServer.h"
#include "../../../Shared/src/Utility/CommonFunc.h"

using namespace DDRFramework;
using namespace DDRCommProto;

FileAddressProcessor::FileAddressProcessor(BaseMessageDispatcher& dispatcher) :BaseProcessor(dispatcher)
{
}


FileAddressProcessor::~FileAddressProcessor()
{
}

void FileAddressProcessor::Process(std::shared_ptr<BaseSocketContainer> spSockContainer, std::shared_ptr<CommonHeader> spHeader, std::shared_ptr<google::protobuf::Message> spMsg)
{


}

void FileAddressProcessor::AsyncProcess(std::shared_ptr<BaseSocketContainer> spSockContainer, std::shared_ptr<DDRCommProto::CommonHeader> spHeader, std::shared_ptr<google::protobuf::Message> spMsg)
{
	reqFileAddress* pRaw = reinterpret_cast<reqFileAddress*>(spMsg.get());


	auto sprsp = std::make_shared<rspFileAddress>();

	if (pRaw->filetype() == eFileTypes::FileHttpAddress)
	{

		auto files = FileManager::Instance()->CheckFiles();

		for (auto fmt : pRaw->filenames())
		{
			std::string filefmt = DDRFramework::getStartWildRegex(fmt);

			for (auto file : files)
			{
				if (std::regex_match(file, std::regex(filefmt)))
				{
					sprsp->add_fileaddrlist(HttpFileServer::Instance()->GetHttpFullPath(file));
				}
			}
		}

		spHeader->set_toclttype(0,eAllClient);
		spHeader->set_flowdirection(0,CommonHeader_eFlowDir_Forward);

		spSockContainer->Send(spHeader,sprsp);
	}
}
