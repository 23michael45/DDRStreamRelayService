#pragma once
#include <string>
std::string g_BuildTime = "4/24/2019 10:58:53 AM";
std::string g_Version = "1.0.0";

#ifdef _DEBUG
std::string g_DMode = "Debug";
#else

std::string g_DMode = "Release";
#endif
