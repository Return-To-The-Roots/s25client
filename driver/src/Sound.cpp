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
#include "main.h" // IWYU pragma: keep
#include "Sound.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

///////////////////////////////////////////////////////////////////////////////
/** @class Sound
 *
 *  Basisklasse für einen Sound.
 *
 *  @author FloSoft
 */

///////////////////////////////////////////////////////////////////////////////
/** @var Sound::nr
 *
 *  Interne Nummer des Sounds.
 *
 *  @author FloSoft
 */

///////////////////////////////////////////////////////////////////////////////
/** @var Sound::CallBack
 *
 *  Callback für Spielbenachrichtigung.
 *
 *  @author FloSoft
 */

Sound::Sound() :  nr(-1), type(SD_UNKNOWN)
{
}

Sound::~Sound()
{
}
