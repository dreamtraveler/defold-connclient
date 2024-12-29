#pragma once

#include <cstdint>

class TimeExpire final
{
public:
    TimeExpire(int expire_long) : expire_long_(expire_long) {}
    bool TryExpire(int64_t now_ms);
    void After(int ms, int64_t now_ms);
    void Reset(int64_t now_ms);

private:
    int64_t last_time_ = {0};
    int expire_long_ = {0};
};
