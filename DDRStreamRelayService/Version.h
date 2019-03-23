#pragma once
#include <string>
std::string g_BuildTime = "3/23/2019 11:47:44 PM";
std::string g_Version = "1.0.0";

#ifdef _DEBUG
std::string g_DMode = "Debug";
#else

std::string g_DMode = "Release";
#endif
