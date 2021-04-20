// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "SoundEffectItem.h"
#include "Settings.h"
#include "drivers/AudioDriverWrapper.h"

EffectPlayId SoundEffectItem::Play(uint8_t volume, bool loop)
{
    if(!SETTINGS.sound.effectsEnabled)
        return EffectPlayId::Invalid;
    return AUDIODRIVER.PlayEffect(GetSoundHandle(), volume, loop);
}
