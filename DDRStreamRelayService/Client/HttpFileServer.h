#ifndef HttpFileServer_h__
#define HttpFileServer_h__



#include <filesystem>
#include <vector>
#include <string>
#include "../../../Shared/src/Utility/Singleton.h"
#include "../../../Shared/src/Network/HttpServer.h"

using namespace DDRFramework;

class HttpFileServer :public HttpServer, public CSingleton<HttpFileServer>
{
public:	 
	HttpFileServer();
	~HttpFileServer();

	std::string GetHttpFullPath(std::string file);
};

#endif // HttpFileServer_h__
