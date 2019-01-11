#include <memory>
#include "../../../Shared/proto/BaseCmd.pb.h"
#include "FileAddressProcessor.h"
#include "../../../Shared/src/Utility/DDRMacro.h"
#include "../../../Shared/src/Utility/Logger.h"
#include "../Client/GlobalManager.h"
#include "../Client/FileManager.h"
#include <regex>
#include "../Client/HttpFileServer.h"
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

		FileManager::Instance()->CheckFiles();

		for (auto fmt : pRaw->filenames())
		{
			DebugLog("FileAddressProcessor %s", fmt.c_str());
			auto files = FileManager::Instance()->Match(fmt);

			for (auto file : files)
			{
				std::string httpaddr = HttpFileServer::Instance()->GetHttpFullPath(file);
				sprsp->add_fileaddrlist(httpaddr);
				DebugLog("%s", httpaddr.c_str());
			}
		}


		spSockContainer->SendBack(spHeader,sprsp);
	}
}
