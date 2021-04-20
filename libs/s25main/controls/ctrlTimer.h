// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Timer.h"
#include "Window.h"

class ctrlTimer : public Window
{
public:
    ctrlTimer(Window* parent, unsigned id, std::chrono::milliseconds timeout);

    void Start();
    void Start(std::chrono::milliseconds timeout);
    void Stop();
    bool isRunning() const { return timer_.isRunning(); }
    Timer::duration getElapsed() const { return timer_.getElapsed(); }

    void Msg_PaintBefore() override;

protected:
    void Draw_() override{};

private:
    std::chrono::milliseconds timeout_;
    Timer timer_;
};
