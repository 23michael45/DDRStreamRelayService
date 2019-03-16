#pragma once
#include <string>
std::string g_BuildTime = "3/16/2019 11:23:15 AM";
std::string g_Version = "1.0.0";

#ifdef _DEBUG
std::string g_DMode = "Debug";
#else

std::string g_DMode = "Release";
#endif
