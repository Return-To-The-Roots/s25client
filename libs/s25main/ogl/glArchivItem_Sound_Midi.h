// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "MusicItem.h"
#include "libsiedler2/ArchivItem_Sound_Midi.h"

class glArchivItem_Sound_Midi : public libsiedler2::ArchivItem_Sound_Midi, public MusicItem
{
public:
    RTTR_CLONEABLE(glArchivItem_Sound_Midi)
protected:
    SoundHandle Load() override;
};
