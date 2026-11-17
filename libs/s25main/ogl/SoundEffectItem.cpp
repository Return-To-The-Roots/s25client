// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "SoundEffectItem.h"
#include "Settings.h"
#include "drivers/AudioDriverWrapper.h"

EffectPlayId SoundEffectItem::play(uint8_t volume, bool loop)
{
    if(!SETTINGS.sound.effectsEnabled)
        return EffectPlayId::Invalid;
    return AUDIODRIVER.playEffect(getSoundHandle(), volume, loop);
}
