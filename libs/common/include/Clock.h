// Copyright (C) 2018 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <chrono>
#include <memory>

class BaseClock
{
public:
    using duration = std::chrono::nanoseconds;
    using rep = duration::rep;
    using period = duration::period;

    virtual ~BaseClock() = default;
    virtual duration time_since_epoch()
    {
        return std::chrono::duration_cast<duration>(std::chrono::steady_clock::now().time_since_epoch());
    }
};

/// Global clock instance for all timing purposes
/// Satisfies TrivialClock requirement
class Clock
{
    static std::unique_ptr<BaseClock>& instance()
    {
        static std::unique_ptr<BaseClock> clock(new BaseClock);
        return clock;
    }

public:
    using rep = BaseClock::rep;
    using duration = BaseClock::duration;
    using time_point = std::chrono::time_point<Clock>;
    static time_point now() { return time_point(instance()->time_since_epoch()); }
    // Set to a different clock
    static void setClock(std::unique_ptr<BaseClock> newClock) { instance() = std::move(newClock); }
};
