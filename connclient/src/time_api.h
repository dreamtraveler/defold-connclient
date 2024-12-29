#pragma once

#include <string>

#include "common_def.h"

namespace TimeAPI
{
int64_t GetTimeMs();
std::string GetCurTimeStr();
void SleepMs(int ms);
};  // namespace TimeAPI
