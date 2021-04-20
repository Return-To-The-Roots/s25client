// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "SoundItem.h"
#include "driver/EffectPlayId.h"

/// Base class for sound effects
class SoundEffectItem : public SoundItem
{
public:
    /// Play the sound effect
    EffectPlayId Play(uint8_t volume, bool loop);
};
