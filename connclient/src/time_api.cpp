#include "time_api.h"
#include "base_macro.h"

#include <iomanip>
#include <sstream>

#ifndef OS_WIN32
#include <sys/time.h>
#endif

namespace TimeAPI
{

#ifdef OS_WIN32
// Based on:
// http://www.google.com/codesearch/p?hl=en#dR3YEbitojA/os_win32.c&q=GetSystemTimeAsFileTime%20license:bsd
// See COPYING for copyright information.
static int gettimeofday(struct timeval* tv, void* tz)
{
#define EPOCHFILETIME (116444736000000000ULL)
    FILETIME ft;
    LARGE_INTEGER li;
    uint64_t tt;

    GetSystemTimeAsFileTime(&ft);
    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;
    tt = (li.QuadPart - EPOCHFILETIME) / 10;
    tv->tv_sec = tt / 1000000;
    tv->tv_usec = tt % 1000000;

    return 0;
}
#endif

int64_t GetTimeMs()
{
    timeval tv;
    gettimeofday(&tv, nullptr);
    return (int64_t)tv.tv_sec * 1000 + (int64_t)tv.tv_usec / 1000;
}

std::string GetCurTimeStr()
{
    timeval tv;
    gettimeofday(&tv, nullptr);
    int64_t t = tv.tv_sec;
    int64_t ms = tv.tv_usec / 1000;
    struct ::tm tm_time;
#ifdef OS_WIN32
    localtime_s(&tm_time, &t);
#else
    localtime_r((const time_t*)&t, &tm_time);
#endif

    std::ostringstream time_oss;
    time_oss.fill('0');
    time_oss << 1900 + tm_time.tm_year << std::setw(2) << 1 + tm_time.tm_mon << std::setw(2)
             << tm_time.tm_mday << '-' << std::setw(2) << tm_time.tm_hour << ":" << std::setw(2)
             << tm_time.tm_min << ":" << std::setw(2) << tm_time.tm_sec << "." << std::setw(3)
             << ms;

    return time_oss.str();
}

void SleepMs(int ms)
{
#ifdef OS_WIN32
    ::Sleep(ms);
#else
    struct timeval tm;
    tm.tv_sec = 0;
    tm.tv_usec = ms * 1000;
    select(0, nullptr, nullptr, nullptr, &tm);
#endif
}

}  // namespace TimeAPI