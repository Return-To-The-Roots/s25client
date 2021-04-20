// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "SoundEffectItem.h"
#include "libsiedler2/ArchivItem_Sound_Wave.h"

class glArchivItem_Sound_Wave : public libsiedler2::ArchivItem_Sound_Wave, public SoundEffectItem
{
public:
    RTTR_CLONEABLE(glArchivItem_Sound_Wave)

protected:
    SoundHandle Load() override;
};
