#pragma once

#include <atomic>
#include <algorithm>

class ClockManager
{
public:
    ClockManager() : local_clock(0) {}

    void increment()
    {
        local_clock++;
    }

    void updateOnReceive(int received_timestamp)
    {
        local_clock = std::max(local_clock.load(), received_timestamp) + 1;
    }

    int getTime() const
    {
        return local_clock.load();
    }

private:
    std::atomic<int> local_clock;
};