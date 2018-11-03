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
#ifndef SOUNDSDL_MUSIC_H_INCLUDED
#define SOUNDSDL_MUSIC_H_INCLUDED

#include "SoundHandle.h"
#include <SDL_mixer.h>
#include <vector>

class SoundSDL_Music : public SoundDesc
{
public:
    explicit SoundSDL_Music(Mix_Music* music) : SoundDesc(SD_MUSIC), music(music) {}
    explicit SoundSDL_Music(const std::vector<char>& data) : SoundDesc(SD_MUSIC), music(NULL), data(data) {}

    void setInvalid() { isValid_ = false; }
    /// Handle to the sound. Managed and freed by the driver
    Mix_Music* music; /// Das Music-Handle.
    const std::vector<char> data;
};

#endif // !SOUNDSDL_MUSIC_H_INCLUDED
