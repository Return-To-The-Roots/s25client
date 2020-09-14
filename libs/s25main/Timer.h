// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "Clock.h"
#include <boost/optional.hpp>

/// Timer class for measuring time periods
class Timer
{
public:
    using duration = Clock::duration;

    Timer(bool startNow = false)
    {
        if(startNow)
            start();
    }
    /// Start timer, throws when already running
    void start();
    /// Stop timer even when not running
    void stop();
    /// Restart timer even when not running
    void restart();
    /// Return elapsed time, throws when not running
    duration getElapsed() const;
    bool isRunning() const { return !!startTime; }

private:
    boost::optional<Clock::time_point> startTime;
};
