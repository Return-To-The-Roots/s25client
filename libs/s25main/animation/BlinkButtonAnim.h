// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "animation/ToggleAnimation.h"

class ctrlButton;

/// Animation which makes a button blink (toggle illuminated)
class BlinkButtonAnim : public ToggleAnimation<ctrlButton>
{
public:
    BlinkButtonAnim(ctrlButton* element, bool startValue = true, unsigned frameRate = 500,
                    RepeatType repeat = Animation::RepeatType::Repeat);
};
