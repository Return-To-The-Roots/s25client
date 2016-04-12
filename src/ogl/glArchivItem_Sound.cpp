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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "defines.h" // IWYU pragma: keep
#include "glArchivItem_Sound.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

glArchivItem_Sound::glArchivItem_Sound()
    : baseArchivItem_Sound(), sound(NULL)
{
}

glArchivItem_Sound::glArchivItem_Sound(const glArchivItem_Sound& item)
    : baseArchivItem_Sound(item), sound(item.sound)
{
}

glArchivItem_Sound& glArchivItem_Sound::operator=(const glArchivItem_Sound& item)
{
    if(this == &item)
        return *this;
    baseArchivItem_Sound::operator=(item);
    sound = item.sound;
    return *this;
}

glArchivItem_Sound::~glArchivItem_Sound()
{}
