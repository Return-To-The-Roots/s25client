// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
