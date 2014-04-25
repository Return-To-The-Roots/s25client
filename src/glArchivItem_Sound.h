// $Id: glArchivItem_Sound.h 9357 2014-04-25 15:35:25Z FloSoft $
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
#ifndef GLARCHIVITEM_SOUND_INCLUDED
#define GLARCHIVITEM_SOUND_INCLUDED

#pragma once

#include "../driver/src/Sound.h"

class glArchivItem_Sound : public virtual libsiedler2::baseArchivItem_Sound
{
    public:
        /// Konstruktor von @p glArchivItem_Sound.
        glArchivItem_Sound(void);
        /// Kopiekonstruktor von @p glArchivItem_Sound.
        glArchivItem_Sound(const glArchivItem_Sound* item);

        /// Destruktor von @p glArchivItem_Sound.
        virtual ~glArchivItem_Sound(void);

        /// Spielt die Musik ab.
        virtual unsigned Play(const unsigned char volume, const bool loop) = 0;

    protected:
        Sound* sound; ///< Der Sound.
};

#endif // !GLARCHIVITEM_SOUND_INCLUDED
