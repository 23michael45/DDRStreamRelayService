#pragma once
#include <string>
std::string g_BuildTime = "3/25/2019 8:36:22 PM";
std::string g_Version = "1.0.0";

#ifdef _DEBUG
std::string g_DMode = "Debug";
#else

std::string g_DMode = "Release";
#endif
