// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "glArchivItem_Sound_Midi.h"
#include "drivers/AudioDriverWrapper.h"

SoundHandle glArchivItem_Sound_Midi::Load()
{
    return AUDIODRIVER.LoadMusic(*this, ".midi");
}
