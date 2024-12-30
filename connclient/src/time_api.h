#pragma once

#include <string>

namespace TimeAPI
{
int64_t GetTimeMs();
std::string GetCurTimeStr();
void SleepMs(int ms);
};  // namespace TimeAPI
