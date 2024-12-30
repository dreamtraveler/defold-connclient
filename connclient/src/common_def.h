#pragma once

#include <memory.h>

#include <cstdint>
#include <iostream>
#include <sstream>

#include "time_api.h"


#if ((defined __APPLE__) && (defined __MACH__))
#if (defined(iOS))
#define OS_IOS
#elif (defined(MAC))
#define OS_MAC
#endif
#elif (defined(ANDROID))
#define OS_ANDROID
#elif (defined(__linux__) || defined(unix))
#define OS_LINUX
#elif (defined(__WIN32__) || defined(_WIN32) || defined(WIN32))
#define OS_WIN32
#else
#error Please define your platform.
#endif

#ifndef OS_WIN32
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#else
#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#endif

#ifndef OS_WIN32
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif