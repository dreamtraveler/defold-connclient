#pragma once

#include <cstdint>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>


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

#ifdef OS_WIN32
#undef max
#undef min
#include "win32.h"
#else
#include "unix.h"
#endif

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef INOUT
#define INOUT
#endif

#define ROUNDUP(x, y) ((((x) + ((y)-1)) / (y)) * (y))
#define ROUNDDOWN(x, y) (((x) / (y)) * (y))
#define DIV_ROUND_UP(n, d) (((n) + (d)-1) / (d))

#define while_s(p)   \
    int ncount0 = 0; \
    while ((ncount0++ < 1000000) && (p))

#define MARCO_ARGS_CONS(a, b) a##b

#define SYNTHESIZE(VarType, VarName, DefautValue)        \
                                                         \
private:                                                 \
    VarType MARCO_ARGS_CONS(VarName, _) = {DefautValue}; \
                                                         \
public:                                                  \
    inline VarType VarName(void) const                   \
    {                                                    \
        return MARCO_ARGS_CONS(VarName, _);              \
    }                                                    \
                                                         \
public:                                                  \
    inline void set_##VarName(VarType VarName)           \
    {                                                    \
        MARCO_ARGS_CONS(VarName, _) = VarName;           \
    }
#define SYNTHESIZE_REF(VarType, VarName)              \
                                                      \
private:                                              \
    VarType MARCO_ARGS_CONS(VarName, _);              \
                                                      \
public:                                               \
    inline VarType* mutable_##VarName(void)           \
    {                                                 \
        return &(MARCO_ARGS_CONS(VarName, _));        \
    }                                                 \
                                                      \
public:                                               \
    inline VarType& VarName(void)                     \
    {                                                 \
        return MARCO_ARGS_CONS(VarName, _);           \
    }                                                 \
                                                      \
public:                                               \
    inline const VarType& const_##VarName(void) const \
    {                                                 \
        return MARCO_ARGS_CONS(VarName, _);           \
    }                                                 \
                                                      \
public:                                               \
    inline void set_##VarName(const VarType& VarName) \
    {                                                 \
        MARCO_ARGS_CONS(VarName, _) = VarName;        \
    }
#define SYNTHESIZE_CHAR_ARRAY(VarName, VarLength)                 \
                                                                  \
private:                                                          \
    char MARCO_ARGS_CONS(VarName, _)[VarLength];                  \
                                                                  \
public:                                                           \
    inline const char* VarName(void) const                        \
    {                                                             \
        return MARCO_ARGS_CONS(VarName, _);                       \
    }                                                             \
                                                                  \
public:                                                           \
    inline void set_##VarName(const char* VarName)                \
    {                                                             \
        strncpy(MARCO_ARGS_CONS(VarName, _), VarName, VarLength); \
    }
#define SYNTHESIZE_ARRAY(VarType, VarName, VarLength)                                 \
                                                                                      \
private:                                                                              \
    VarType MARCO_ARGS_CONS(VarName, _)[VarLength];                                   \
                                                                                      \
public:                                                                               \
    inline VarType* mutable_##VarName(int32_t index)                                  \
    {                                                                                 \
        if (index < 0 || index >= VarLength) return nullptr;                          \
        return &(MARCO_ARGS_CONS(VarName, _)[index]);                                 \
    }                                                                                 \
                                                                                      \
public:                                                                               \
    inline const VarType& VarName(int32_t index)                                      \
    {                                                                                 \
        return MARCO_ARGS_CONS(VarName, _)[index];                                    \
    }                                                                                 \
                                                                                      \
public:                                                                               \
    inline void set_##VarName(int32_t index, const VarType& VarName)                  \
    {                                                                                 \
        MARCO_ARGS_CONS(VarName, _)[index] = VarName;                                 \
    }                                                                                 \
                                                                                      \
public:                                                                               \
    inline void clear_##VarName()                                                     \
    {                                                                                 \
        memset(&MARCO_ARGS_CONS(VarName, _), 0, sizeof(MARCO_ARGS_CONS(VarName, _))); \
    }                                                                                 \
                                                                                      \
public:                                                                               \
    inline int32_t VarName##_size()                                                   \
    {                                                                                 \
        return VarLength;                                                             \
    }


#define TO_STR(x) #x

#define INT_LIMIT (std::numeric_limits<int>::max() / 2)
#define FIX_INT_LIMIT(num)                                                        \
    {                                                                             \
        if (num < 0) {                                                            \
            LOG_ERROR("FIX_INT_LIMIT num = " << num << ", fix to 0");             \
            num = 0;                                                              \
        } else if (num > INT_LIMIT) {                                             \
            LOG_ERROR("FIX_INT_LIMIT num = " << num << ", fix to " << INT_LIMIT); \
            num = INT_LIMIT;                                                      \
        }                                                                         \
    }

#define INT64_LIMIT (std::numeric_limits<int64_t>::max() / 2)
#define FIX_INT64_LIMIT(num)                                                          \
    {                                                                                 \
        if (num < 0) {                                                                \
            LOG_ERROR("FIX_INT64_LIMIT num = " << num << ", fix to 0");               \
            num = 0;                                                                  \
        } else if (num > INT64_LIMIT) {                                               \
            LOG_ERROR("FIX_INT64_LIMIT num = " << num << ", fix to " << INT64_LIMIT); \
            num = INT64_LIMIT;                                                        \
        }                                                                             \
    }

#define CHECK_RET(x, iFailValueReturned)                        \
    do {                                                        \
        if (!(x)) {                                             \
            std::cerr << "CHECK failed: " << TO_STR(x) << "\n"; \
            return (iFailValueReturned);                        \
        }                                                       \
    } while (0)

#define CHECK_VOID(x)                                           \
    do {                                                        \
        if (!(x)) {                                             \
            std::cerr << "CHECK failed: " << TO_STR(x) << "\n"; \
            return;                                             \
        }                                                       \
    } while (0)

#define CHECK_FUNC_RET(func)      \
    do {                          \
        int ret = func;           \
        if (ret != 0) return ret; \
    } while (0)


#define ASSERT(__b)           \
    do {                      \
        if (!(__b)) {         \
            int* p = nullptr; \
            *p = 0;           \
        }                     \
    } while (0)
