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

#ifndef Clock_h__
#define Clock_h__

#include <libutil/unique_ptr.h>
#include <boost/chrono.hpp>

class BaseClock
{
public:
    typedef boost::chrono::nanoseconds duration;
    typedef duration::rep rep;
    typedef duration::period period;

    virtual ~BaseClock() {}
    virtual duration time_since_epoch()
    {
        return boost::chrono::duration_cast<duration>(boost::chrono::steady_clock::now().time_since_epoch());
    }
};

/// Global clock instance for all timing purposes
/// Satisfies TrivialClock requirement
class Clock
{
    static libutil::unique_ptr<BaseClock>& instance()
    {
        static libutil::unique_ptr<BaseClock> clock(new BaseClock);
        return clock;
    }

public:
    typedef BaseClock::rep rep;
    typedef BaseClock::duration duration;
    typedef boost::chrono::time_point<Clock> time_point;
    static time_point now() { return time_point(instance()->time_since_epoch()); }
    // Set to a different clock
    static void setClock(libutil::unique_ptr<BaseClock> newClock) { instance() = boost::move(newClock); }
};

#endif // Clock_h__