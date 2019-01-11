

#include <memory>
#include "../../../Shared/proto/BaseCmd.pb.h"
#include "FileStatusProcessor.h"
#include "../../../Shared/src/Utility/DDRMacro.h"
#include "../../../Shared/src/Utility/Logger.h"
#include "../../../Shared/src/Utility/CommonFunc.h"
#include "../../../Shared/thirdparty/cppfs/include/cppfs/windows/LocalFileSystem.h"
#include "../Client/GlobalManager.h"
#include "../Client/FileManager.h"
#include <regex>
#include "../Client/HttpFileServer.h"

using namespace DDRFramework;
using namespace DDRCommProto;

FileStatusProcessor::FileStatusProcessor(BaseMessageDispatcher& dispatcher) :BaseProcessor(dispatcher)
{
}


FileStatusProcessor::~FileStatusProcessor()
{
}

void FileStatusProcessor::Process(std::shared_ptr<BaseSocketContainer> spSockContainer, std::shared_ptr<CommonHeader> spHeader, std::shared_ptr<google::protobuf::Message> spMsg)
{

	chkFileStatus* pRaw = reinterpret_cast<chkFileStatus*>(spMsg.get());


	auto sprsp = std::make_shared<ansFileStatus>();

	if (pRaw->filetype() == eFileTypes::FileHttpAddress)
	{

		auto files = FileManager::Instance()->CheckFiles();

		for (auto fmt : pRaw->filenames())
		{
			std::string filefmt = DDRFramework::getStarWildRegex(fmt,true);

			for (auto file : files)
			{
				if (std::regex_match(file, std::regex(filefmt)))
				{
					sprsp->add_fileaddrlist(HttpFileServer::Instance()->GetHttpFullPath(file));
				}
			}
		}

		spSockContainer->Send(sprsp);
	}
	else
	{

	}

}

void FileStatusProcessor::AsyncProcess(std::shared_ptr<BaseSocketContainer> spSockContainer, std::shared_ptr<DDRCommProto::CommonHeader> spHeader, std::shared_ptr<google::protobuf::Message> spMsg)
{


}
