#pragma once
#include <string>
std::string g_BuildTime = "2/14/2019 11:57:11 AM";
std::string g_Version = "1.0.0";

#ifdef _DEBUG
std::string g_DMode = "Debug";
#else

std::string g_DMode = "Release";
#endif
