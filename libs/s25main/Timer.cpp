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
