#include "HttpFileServer.h"
#include "../../../Shared/thirdparty/cppfs/include/cppfs/FilePath.h"
#include "../Client/GlobalManager.h"
#include "../../../Shared/src/Utility/CommonFunc.h"

HttpFileServer::HttpFileServer()
{

}
HttpFileServer::~HttpFileServer()
{


}

std::string HttpFileServer::GetHttpFullPath(std::string file)
{
	std::string ret;
	cppfs::FilePath root(m_RootPath);
	cppfs::FilePath full(file);

	auto fullresolved = full.resolved();
	auto rootresolved = root.resolved();
	//std::string relativeFilePath = fullresolved.replace(fullresolved.begin(), fullresolved.begin() + rootresolved.length(), "/");

	std::string relativeFilePath = replace_all(fullresolved, rootresolved, "/");
	std::string httphead = m_IPAddress + ":" + m_Port + "/";
	ret = httphead + relativeFilePath;
	ret = replace_all(ret, "\\", "/");
	ret = replace_all(ret, "///", "/");
	ret = replace_all(ret, "//", "/");
	ret = "http://" + ret;
	return ret;
}
