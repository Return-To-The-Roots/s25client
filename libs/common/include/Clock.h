// Copyright (c) 2018 - 2018 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

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
