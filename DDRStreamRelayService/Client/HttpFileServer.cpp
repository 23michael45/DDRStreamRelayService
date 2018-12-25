#include "HttpFileServer.h"
#include "../../../Shared/thirdparty/cppfs/include/cppfs/FilePath.h"
#include "GlobalManager.h"

HttpFileServer::HttpFileServer()
{

}
HttpFileServer::~HttpFileServer()
{


}

std::string HttpFileServer::GetHttpFullPath(std::string file)
{
	cppfs::FilePath root(m_RootPath);
	cppfs::FilePath full(file);
	
	auto fullresolved = full.resolved();
	auto rootresolved = root.resolved();
	std::string relativeFilePath = fullresolved.replace(fullresolved.begin(), fullresolved.begin() + rootresolved.length() , "/");
	std::string ret = "http://" + m_IPAddress + ":" + m_Port + "/" + relativeFilePath;
	return ret;
}
