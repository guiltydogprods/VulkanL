#include "stdafx.h"
#include "system.h"

#ifdef RE_PLATFORM_WIN64
#include <windows.h>
#else
#include <time.h>
#endif

#ifdef RE_PLATFORM_WIN64

//static rsys_perfTimer s_perfTimer;
static LARGE_INTEGER s_timerFreq = { .QuadPart = 0 };
#else
static uint64_t s_timerFreq = 0;
#endif

void rsys_perfTimerInitialize()
{
#ifdef RE_PLATFORM_WIN64
	QueryPerformanceFrequency(&s_timerFreq);
#else

#endif
}

void rsys_perfTimerStart(rsys_perfTimer* timer)
{
	assert(timer != NULL);
#ifdef RE_PLATFORM_WIN64
	QueryPerformanceCounter(&timer->startTime);
	timer->splitTime = timer->startTime;
#else
	clock_gettime(CLOCK_MONOTONIC, &timer->startTime);
	timer->splitTime = timer->startTime;
#endif
}

uint64_t rsys_perfTimerGetSplit(rsys_perfTimer* timer)
{
#ifdef RE_PLATFORM_WIN64
	assert(s_timerFreq.QuadPart != 0);
	LARGE_INTEGER currentTime;
	QueryPerformanceCounter(&currentTime);

	LONGLONG elapsedUsecs = currentTime.QuadPart - timer->splitTime.QuadPart;
	timer->splitTime = currentTime;
	return (elapsedUsecs * 1000000) / s_timerFreq.QuadPart;
#else
	struct timespec currentTime;
	clock_gettime(CLOCK_MONOTONIC, &currentTime);
	uint64_t elapsedUsecs = (currentTime.tv_nsec - timer->splitTime.tv_nsec) / 1000;
	timer->splitTime = currentTime;

	return elapsedUsecs;
#endif
}

uint64_t rsys_perfTimerStop(rsys_perfTimer* timer)
{
	assert(timer != NULL);
#ifdef RE_PLATFORM_WIN64
	assert(s_timerFreq.QuadPart != 0);
	QueryPerformanceCounter(&timer->endTime);

	LONGLONG elapsedUsecs = timer->endTime.QuadPart - timer->splitTime.QuadPart;
	timer->elapsedUsecs = ((timer->endTime.QuadPart - timer->startTime.QuadPart) * 1000000) / s_timerFreq.QuadPart;
	return (elapsedUsecs * 1000000) / s_timerFreq.QuadPart;
#else

	clock_gettime(CLOCK_MONOTONIC, &timer->endTime);
	uint64_t elapsedUsecs = (timer->endTime.tv_nsec - timer->splitTime.tv_nsec) / 1000;
	timer->elapsedUsecs = (timer->endTime.tv_nsec - timer->startTime.tv_nsec) / 1000;

	return elapsedUsecs;
#endif
}

void rsys_print(const char* fmt, ...)
{
	char debugString[4096];				//CLR - Should really calculate the size of the output string instead.

	va_list args;
	va_start(args, fmt);
#ifdef RE_PLATFORM_WIN64
	vsprintf_s(debugString, sizeof(debugString), fmt, args);
#else
	vsprintf(debugString, fmt, args);
#endif
	va_end(args);
#ifdef RE_PLATFORM_WIN64
	if (IsDebuggerPresent())
	{
		OutputDebugStringA(debugString);
		fputs(debugString, stdout);
	}
	else
		fputs(debugString, stdout);
#elif RE_PLATFORM_MACOS
    fputs(debugString, stdout);
#else // RE_PLATFORM_ANDROID
	__android_log_print(ANDROID_LOG_DEBUG, "RealityEngine", "%s", debugString);
#endif
}
