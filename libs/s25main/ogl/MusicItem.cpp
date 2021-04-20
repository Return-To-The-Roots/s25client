// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "MusicItem.h"
#include "Settings.h"
#include "drivers/AudioDriverWrapper.h"

void MusicItem::Play(int repeats)
{
    if(!SETTINGS.sound.musicEnabled)
        return;
    AUDIODRIVER.PlayMusic(GetSoundHandle(), repeats);
}
