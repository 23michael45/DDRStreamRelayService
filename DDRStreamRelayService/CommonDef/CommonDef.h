#ifndef __DDR_COMMON_DEFINITIONS_AND_INCLUSIONS_H_INCLUDED__
#define __DDR_COMMON_DEFINITIONS_AND_INCLUSIONS_H_INCLUDED__

#include "Types.h"
#include "sys_configurations.h"

#ifndef PI
#define PI 3.14159265359f
#define PI2 6.28318530718f
#define HALFPI 1.5707963268f
#endif
#ifndef RAD2DEG
#define RAD2DEG 57.295780f
#define DEG2RAD 0.0174533f
#endif

#define ENVIRONMENT_CPU_MULTITHREADING_ENABLED

#ifndef CLEAR_HANDLE
#define CLEAR_HANDLE(p) if (p){ \
							CloseHandle(p); \
							p = 0; }
#endif
#ifndef CLEAR_HEAP_ARRAY
#define CLEAR_HEAP_ARRAY(p) if (p){ \
							delete []p; \
							p = 0; }
#endif
#ifndef CLEAR_HEAP_PTR
#define CLEAR_HEAP_PTR(p) if (p){ \
							delete p; \
							p = 0; }

#endif

#ifndef _TIC_START
#include <chrono>
#define _TIC_START_() std::chrono::system_clock::time_point t_start = std::chrono::high_resolution_clock::now();
#define _TIC_END_() std::chrono::system_clock::time_point t_end = std::chrono::high_resolution_clock::now();\
					double timediff = std::chrono::duration_cast<std::chrono::nanoseconds>(t_end - t_start).count() / (1e6);\
					//printf("elapsed time = %.3f ms.\n", timediff);
#endif

#endif
