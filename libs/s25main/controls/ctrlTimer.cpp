// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "ctrlTimer.h"

#include "drivers/VideoDriverWrapper.h"

ctrlTimer::ctrlTimer(Window* parent, unsigned id, std::chrono::milliseconds timeout)
    : Window(parent, id, DrawPoint(0, 0)), timeout_(timeout), timer_(true)
{}

/**
 *  start the timer
 */
void ctrlTimer::Start(std::chrono::milliseconds timeout)
{
    timeout_ = timeout;
    Start();
}

void ctrlTimer::Start()
{
    timer_.restart();
}

/**
 *  stop the timer
 */
void ctrlTimer::Stop()
{
    timer_.stop();
}

void ctrlTimer::Msg_PaintBefore()
{
    Window::Msg_PaintBefore();

    if(!timer_.isRunning())
        return;

    if(timer_.getElapsed() >= timeout_)
    {
        timer_.restart(); // Do this first so parent can stop or change duration
        GetParent()->Msg_Timer(GetID());
    }
}
