#pragma once
#include <string>
std::string g_BuildTime = "3/2/2019 4:55:50 PM";
std::string g_Version = "1.0.0";

#ifdef _DEBUG
std::string g_DMode = "Debug";
#else

std::string g_DMode = "Release";
#endif
