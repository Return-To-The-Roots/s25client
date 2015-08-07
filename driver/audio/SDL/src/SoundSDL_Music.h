// $Id: SoundSDL_Music.h 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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

#include <Sound.h>

class AudioSDL;

class SoundSDL_Music : public Sound
{
        friend class AudioSDL;
    public:
        /// Konstruktor von @p SoundSDL_Music.
        SoundSDL_Music();

        /// Destruktor von @p SoundSDL_Music.
        ~SoundSDL_Music(void);

    private:

        static SoundSDL_Music* mthis;
        Mix_Music* music;  ///< Das Music-Handle.

};

#endif // !SOUNDSDL_MUSIC_H_INCLUDED
