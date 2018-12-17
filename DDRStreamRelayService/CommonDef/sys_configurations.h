#ifndef __PLATFORM_CONFIGURATIONS_H_INCLUDED__
#define __PLATFORM_CONFIGURATIONS_H_INCLUDED__

#ifdef DEBUG
#define MY_COMPILER_ASSERT(EXPRESSION)   switch (0) {case 0: case (EXPRESSION):;}
#else
#define MY_COMPILER_ASSERT(EXPRESSION)
#endif

#define CPU_SSE42_ENABLED 1
#define CPU_NEON_ENABLED 2
#define GPU_CUDA_ENABLED 4
#define MS_WINDOWS 1
#define LINUX_UBUNTU 2
#define ENVIRONMENT_CPU_ARCH CPU_SSE42_ENABLED
#define ENVIRONMENT_CPU_MULTITHREADING_ENABLED
#define ENVIRONMENT_OS_PLATFORM MS_WINDOWS

// alias names
#define CUR_ARCH_OPTIMIZATION ENVIRONMENT_CPU_ARCH
#define OS_PLATFORM ENVIRONMENT_OS_PLATFORM

#if (CUR_ARCH_OPTIMIZATION == CPU_SSE42_ENABLED)
#include "emmintrin.h"
#include "immintrin.h"
#include "smmintrin.h"
#include "nmmintrin.h"
#elif (CUR_ARCH_OPTIMIZATION & CPU_NEON_ENABLED)
// NEON intrinsics
#endif

#ifdef ENVIRONMENT_CPU_MULTITHREADING_ENABLED
#include <thread>
#endif

#if ENVIRONMENT_OS_PLATFORM == MS_WINDOWS
#define _WINSOCKAPI_
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#elif ENVIRONMENT_OS_PLATFORM == LINUX_UBUNTU
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include "LinuxThreads.h"

#endif // ENVIRONMENT_OS_PLATFORM

#endif // __COMMON_INCLUDES_AND_MACROS_H_INCLUDED__
