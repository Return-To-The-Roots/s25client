// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "MusicItem.h"
#include "libsiedler2/ArchivItem_Sound_XMidi.h"

class glArchivItem_Sound_XMidi : public libsiedler2::ArchivItem_Sound_XMidi, public MusicItem
{
public:
    RTTR_CLONEABLE(glArchivItem_Sound_XMidi)

protected:
    SoundHandle Load() override;
};
