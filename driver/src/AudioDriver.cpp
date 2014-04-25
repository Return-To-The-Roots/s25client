// $Id: AudioDriver.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "main.h"
#include "AudioDriver.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/** @class AudioDriver
 *
 *  Basisklasse für einen Audiotreiber.
 *
 *  @author FloSoft
 */

///////////////////////////////////////////////////////////////////////////////
/** @var AudioDriver::initialized
 *
 *  Initialisierungsstatus.
 *
 *  @author FloSoft
 */

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p AudioDriver.
 *
 *  @author FloSoft
 */
AudioDriver::AudioDriver(AudioDriverLoaderInterface* adli)
    : play_id_counter(1),  adli(adli), initialized(false)
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Destruktor von @p AudioDriver.
 *
 *  @author FloSoft
 */
AudioDriver::~AudioDriver(void)
{
    for(std::vector<Sound*>::iterator it = sounds.begin(); it != sounds.end(); ++it)
    {
        // Sounddeskriptoren aufräumen
        delete (*it);
    }
    sounds.clear();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Funktion zum Auslesen des Treibernamens.
 *
 *  @return liefert den Treibernamen zurück
 *
 *  @author FloSoft
 */
const char* AudioDriver::GetName(void) const
{
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
unsigned AudioDriver::GeneratePlayID()
{
    // Ende erreicht?
    if(play_id_counter == 0xFFFF)
    {
        // dann wieder bei 0 anfangen (0xFFFF als Fehlermeldung!)
        play_id_counter = 0;
    }

    return play_id_counter++;
}
