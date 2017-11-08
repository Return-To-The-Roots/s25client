// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef SOUNDSDL_EFFECT_H_INCLUDED
#define SOUNDSDL_EFFECT_H_INCLUDED

#include "SoundHandle.h"

struct Mix_Chunk;

/// SDL sound descriptor. Internal use only
class SoundSDL_Effect : public SoundDesc
{
public:
    explicit SoundSDL_Effect(Mix_Chunk* sound) : SoundDesc(SD_EFFECT), sound(sound) {}

    void setInvalid() { isValid_ = false; }
    /// Handle to the sound. Managed and freed by the driver
    Mix_Chunk* const sound;
};

#endif // !SOUNDSDL_EFFECT_H_INCLUDED
