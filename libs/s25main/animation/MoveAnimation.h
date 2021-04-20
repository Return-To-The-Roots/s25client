// Copyright (C) 2017 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "DrawPoint.h"
#include "animation/Animation.h"

/// Animations which moves an element to a new location
class MoveAnimation : public Animation
{
public:
    MoveAnimation(Window* element, DrawPoint newPos, unsigned animTime, RepeatType repeat);

    void onRescale(const ScreenResizeEvent& rs) override;

protected:
    void doUpdate(Window* element, double nextFramepartTime) override;

private:
    DrawPoint origPos_, newPos_;
};
