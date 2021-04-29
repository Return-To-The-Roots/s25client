// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
                    RepeatType repeat = Animation::RepeatType::Repeat);

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
    setSkipType(Animation::SkipType::Time);
}

template<class T>
void ToggleAnimation<T>::doUpdate(Window* element, double nextFramepartTime)
{
    // Use this to avoid problems if users increase the frame count
    bool curValue = (getCurLinearInterpolationFactor(nextFramepartTime) < 0.5) ? startValue_ : !startValue_;
    T& actualElement = dynamic_cast<T&>(*element);
    CALL_MEMBER_FN(actualElement, animFunc_)(curValue);
}
