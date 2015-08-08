// $Id: glArchivItem_Sound_Other.cpp 9357 2014-04-25 15:35:25Z FloSoft $
//
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
#include "defines.h"
#include "glArchivItem_Sound_Other.h"

#include "drivers/AudioDriverWrapper.h"
#include "Settings.h"
#include "../driver/src/AudioDriver.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  Spielt die Musik ab.
 *
 *  @param[in] volume Lautstärke der Musik.
 *  @param[in] loop   Endlosschleife ja/nein
 *
 *  @author FloSoft
 */
void glArchivItem_Sound_Other::Play(const unsigned repeats)
{
    if(!SETTINGS.sound.musik)
        return;

    if(!sound)
        sound = AUDIODRIVER.LoadMusic(AudioDriver::AD_OTHER, data, length);

    if(sound)
        AUDIODRIVER.PlayMusic(sound, repeats);
}
