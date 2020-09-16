// Copyright (c) 2017 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#pragma once

#include "animation/Animation.h"
#include "commonDefines.h"

/// Animations which toggles a boolean state of an object
template<class T>
class ToggleAnimation : public Animation
{
public:
    using BoolFunc = void (T::*)(bool);

    ToggleAnimation(T* element, BoolFunc animFunc, bool startValue, unsigned frameRate,
                    RepeatType repeat = Animation::RPT_Repeat);

protected:
    void doUpdate(Window* element, double nextFramepartTime) override;

private:
    BoolFunc animFunc_;
    bool startValue_;
};

template<class T>
ToggleAnimation<T>::ToggleAnimation(T* element, BoolFunc animFunc, bool startValue, unsigned frameRate,
                                    RepeatType repeat)
    : Animation(element, 2, frameRate, repeat), animFunc_(animFunc), startValue_(startValue)
{
    setSkipType(Animation::SKIP_TIME);
}

template<class T>
void ToggleAnimation<T>::doUpdate(Window* element, double nextFramepartTime)
{
    // Use this to avoid problems if users increase the frame count
    bool curValue = (getCurLinearInterpolationFactor(nextFramepartTime) < 0.5) ? startValue_ : !startValue_;
    T& actualElement = dynamic_cast<T&>(*element);
    CALL_MEMBER_FN(actualElement, animFunc_)(curValue);
}
