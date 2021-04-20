// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
