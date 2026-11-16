// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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
void ctrlTimer::start(std::chrono::milliseconds timeout)
{
    timeout_ = timeout;
    start();
}

void ctrlTimer::start()
{
    timer_.restart();
}

/**
 *  stop the timer
 */
void ctrlTimer::stop()
{
    timer_.stop();
}

void ctrlTimer::msgPaintBefore()
{
    Window::msgPaintBefore();

    if(!timer_.isRunning())
        return;

    if(timer_.getElapsed() >= timeout_)
    {
        timer_.restart(); // Do this first so parent can stop or change duration
        getParent()->msgTimer(getID());
    }
}
