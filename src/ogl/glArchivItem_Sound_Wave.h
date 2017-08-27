// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.
#ifndef GLARCHIVITEM_SOUND_WAVE_H_INCLUDED
#define GLARCHIVITEM_SOUND_WAVE_H_INCLUDED

#pragma once

#include "glArchivItem_Sound.h"
#include "libsiedler2/src/ArchivItem_Sound_Wave.h"

class glArchivItem_Sound_Wave : public libsiedler2::ArchivItem_Sound_Wave, public glArchivItem_Sound
{
public:
    RTTR_CLONEABLE(glArchivItem_Sound_Wave)

    /// Spielt den Sound ab.
        EffectPlayId Play(uint8_t volume, bool loop) override;
};

#endif // !GLARCHIVITEM_SOUND_WAVE_H_INCLUDED
