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
#ifndef GLARCHIVITEM_SOUND_INCLUDED
#define GLARCHIVITEM_SOUND_INCLUDED

#pragma once

#include "driver/src/SoundHandle.h"
#include "driver/src/EffectPlayId.h"
#include "libsiedler2/src/ArchivItem_Sound.h"

class glArchivItem_Sound
{
public:
    glArchivItem_Sound();
    virtual ~glArchivItem_Sound();

    /// Spielt die Musik ab.
    virtual EffectPlayId Play(uint8_t volume, bool loop) = 0;

protected:
    /// Handle to the sound, managed by driver, hence safe to copy
    SoundHandle sound;
};

#endif // !GLARCHIVITEM_SOUND_INCLUDED
