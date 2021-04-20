// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Timer.h"
#include <stdexcept>

void Timer::start()
{
    if(isRunning())
        throw std::runtime_error("Timer is already running");
    restart();
}

void Timer::stop()
{
    startTime = boost::none;
}

void Timer::restart()
{
    startTime = Clock::now();
}

Timer::duration Timer::getElapsed() const
{
    if(!isRunning())
        throw std::runtime_error("Timer is not running");
    return Clock::now() - *startTime;
}
