// Copyright (C) 2017 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "MoveAnimation.h"
#include "RescaleWindowProp.h"
#include "Window.h"
#include "drivers/ScreenResizeEvent.h"
#include <cmath>

MoveAnimation::MoveAnimation(Window* element, DrawPoint newPos, unsigned animTime, RepeatType repeat)
    : Animation(element, 2, animTime, repeat), origPos_(element->GetPos()), newPos_(newPos)
{
    Point<double> diff(newPos_ - origPos_);
    diff = elMax(Point<double>::all(1), diff); // Avoid division by zero
    Point<double> msPerPixel(Point<double>::all(animTime) / diff);
    double frameRate = std::max(1., std::floor(std::min(msPerPixel.x, msPerPixel.y)));
    setFrameRate(static_cast<unsigned>(frameRate));
    setNumFrames(static_cast<unsigned>(std::ceil(animTime / frameRate)) + 1u);
}

void MoveAnimation::onRescale(const ScreenResizeEvent& rs)
{
    RescaleWindowProp rescale(rs.oldSize, rs.newSize);
    origPos_ = rescale(origPos_);
    newPos_ = rescale(newPos_);
}

void MoveAnimation::doUpdate(Window* element, double nextFramepartTime)
{
    DrawPoint totalDiff = newPos_ - origPos_;
    Point<double> curDiff = Point<double>(totalDiff) * getCurLinearInterpolationFactor(nextFramepartTime);
    DrawPoint curDiffInt;
    curDiffInt.x = std::lround(curDiff.x);
    curDiffInt.y = std::lround(curDiff.y);
    element->SetPos(curDiffInt + origPos_);
}
