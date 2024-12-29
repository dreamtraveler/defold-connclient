#include "time_expire.h"

bool TimeExpire::TryExpire(int64_t now_ms)
{
    if (now_ms > last_time_ + expire_long_) {
        last_time_ = now_ms;
        return true;
    } else {
        return false;
    }
}

void TimeExpire::After(int ms, int64_t now_ms)
{
    if (ms < 0) return;
    if (ms == 0) {
        last_time_ = 0;
    } else {
        last_time_ = now_ms;
        expire_long_ = ms;
    }
}

void TimeExpire::Reset(int64_t now_ms)
{
    last_time_ = now_ms;
}