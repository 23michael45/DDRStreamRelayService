#ifndef __DDR_COMMON_FUNCTIONS_H_INCLUDED__
#define __DDR_COMMON_FUNCTIONS_H_INCLUDED__

#include "CommonDef.h"

#ifdef DDR_COMMON_FUNCS_LIB
#define __API__DDRCF__ __declspec(dllexport)   
#else
#define __API__DDRCF__ __declspec(dllimport)
#ifdef _DEBUG
#ifndef _WIN64
#pragma comment (lib, "CommonDef/CommonDef_d.lib")
#else
#pragma comment (lib, "CommonDef/CommonDef_X64d.lib")
#endif
#else
#ifndef _WIN64
#pragma comment (lib, "CommonDef/CommonDef_r.lib")
#else
#pragma comment (lib, "CommonDef/CommonDef_X64r.lib")
#endif
#endif
#endif

namespace DDRCommonDef
{
	// sys. defined key functions
	__API__DDRCF__ void ClearKeyboardInput();
	__API__DDRCF__ bool KeyStatusChange(int &vCode, bool &bDown);
	__API__DDRCF__ int KeyTest();
	__API__DDRCF__ void WaitOrKey(unsigned int maxWaitTimeInMS);
	// time stamp, IP / integer to string conversions
	__API__DDRCF__ void GetFormattedTime(char *str, int *pLen = nullptr);
	__API__DDRCF__ void GetUNIXTimeStamp(unsigned int &UTC_Sec, unsigned short &milSec);
	// pStr points to an array holding at least 11 elements to be safe
	__API__DDRCF__ void ConvertInt2Str(int x, char *pStr, int *pEndingPos = nullptr);

	__API__DDRCF__ float pi2pif(float angInRad);
	__API__DDRCF__ float WeightedAngleAddition_f(float ang1, float ang2, float alpha);
	__API__DDRCF__ double pi2pi(double angInRad);
	__API__DDRCF__ double WeightedAngleAddition(double ang1, double ang2, double alpha);
	__API__DDRCF__ void GetRelative2DPose_f(float x0, float y0, float th0,
		                                    float x1, float y1, float th1,
											float &x1_0, float &y1_0, float &th1_0);
	__API__DDRCF__ void GetRelative2DPose(double x0, double y0, double th0,
		                                  double x1, double y1, double th1,
										  double &x1_0, double &y1_0, double &th1_0);
	__API__DDRCF__ void Get2DPoseFromBaseAndRelative(double x0, double y0, double th0,
		                                             double x1_0, double y1_0, double th1_0,
													 double &x1, double &y1, double &th1);
	__API__DDRCF__ void Get2DPoseFromBaseAndRelative_f(float x0, float y0, float th0,
		                                               float x1_0, float y1_0, float th1_0,
													   float &x1, float &y1, float &th1);
}

#endif // __DDR_COMMON_FUNCTIONS_H_INCLUDED__
