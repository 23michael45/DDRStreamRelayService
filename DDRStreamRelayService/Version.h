#pragma once
#include <string>
std::string g_BuildTime = "3/5/2019 5:29:57 PM";
std::string g_Version = "1.0.0";

#ifdef _DEBUG
std::string g_DMode = "Debug";
#else

std::string g_DMode = "Release";
#endif
