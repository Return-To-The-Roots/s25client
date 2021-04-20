// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "glArchivItem_Sound_Wave.h"
#include "drivers/AudioDriverWrapper.h"

SoundHandle glArchivItem_Sound_Wave::Load()
{
    return AUDIODRIVER.LoadEffect(*this, ".wav");
}
