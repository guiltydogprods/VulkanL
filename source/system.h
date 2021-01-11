#pragma once
#include "file.h"

#ifdef RE_PLATFORM_WIN64
#include <windows.h>

typedef struct rsys_perfTimer {
	LARGE_INTEGER startTime;
	LARGE_INTEGER endTime;
	LARGE_INTEGER splitTime;
	uint64_t elapsedUsecs;
}rsys_perfTimer;

#else
#include <time.h>
typedef struct rsys_perfTimer {
	struct timespec startTime;
	struct timespec endTime;
	struct timespec splitTime;
	uint64_t elapsedUsecs;
}rsys_perfTimer;

#endif


void rsys_print(const char* fmt, ...);

void rsys_perfTimerInitialize();
void rsys_perfTimerStart(rsys_perfTimer* timer);
uint64_t rsys_perfTimerStop(rsys_perfTimer* timer);
uint64_t rsys_perfTimerGetSplit(rsys_perfTimer* timer);

rsys_file file_open(const char* filename, const char* mode);
void file_close(rsys_file fptr);
bool file_is_open(rsys_file);
size_t file_length(rsys_file fptr);
size_t file_read(void* buffer, size_t bytes, rsys_file fptr);

typedef struct rsysApi
{
	rsys_file(*file_open)(const char* filename, const char* mode);
	void(*file_close)(rsys_file file);
	bool(*file_is_open)(rsys_file file);
	size_t(*file_length)(rsys_file file);
	size_t(*file_read)(void *buffer, size_t bytes, rsys_file file);
	void(*print)(const char* fmt, ...);
	void(*perfTimerStart)(rsys_perfTimer* timer);
	uint64_t(*perfTimerStop)(rsys_perfTimer* timer);
	uint64_t(*perfTimerGetSplit)(rsys_perfTimer* timer);
}rsysApi;
