// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "BlinkButtonAnim.h"
#include "controls/ctrlButton.h"

BlinkButtonAnim::BlinkButtonAnim(ctrlButton* element, bool startValue, unsigned frameRate, RepeatType repeat)
    : ToggleAnimation<ctrlButton>(element, &ctrlButton::SetIlluminated, startValue, frameRate, repeat)
{}
