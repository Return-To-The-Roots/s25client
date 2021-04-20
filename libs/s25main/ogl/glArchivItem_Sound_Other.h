// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "MusicItem.h"
#include "libsiedler2/ArchivItem_Sound_Other.h"

class glArchivItem_Sound_Other : public libsiedler2::ArchivItem_Sound_Other, public MusicItem
{
public:
    glArchivItem_Sound_Other(libsiedler2::SoundType soundType) : ArchivItem_Sound_Other(soundType) {}
    RTTR_CLONEABLE(glArchivItem_Sound_Other)

protected:
    SoundHandle Load() override;
};
