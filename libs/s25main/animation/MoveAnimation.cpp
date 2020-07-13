// Copyright (c) 2017 - 2017 Settlers Freaks (sf-team at siedler25.org)
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
    Point<double> msPerPixel(static_cast<double>(animTime) / diff);
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
