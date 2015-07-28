﻿// $Id: glArchivItem_Sound_Wave.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "defines.h"
#include "glArchivItem_Sound_Wave.h"

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
 *  Spielt den Sound ab.
 *
 *  @param[in] volume Lautstärke des Sounds.
 *  @param[in] loop   Endlosschleife ja/nein
 *
 *  @author FloSoft
 */
unsigned int glArchivItem_Sound_Wave::Play(unsigned char volume, bool loop)
{
    if(!SETTINGS.sound.effekte/* || !VIDEODRIVER.audiodriver*/)
        return 0xFFFFFFFF;

    if(!sound)
        sound = AUDIODRIVER.LoadEffect(AudioDriver::AD_WAVE, data, length);

    return AUDIODRIVER.PlayEffect(sound, volume, loop);
}
