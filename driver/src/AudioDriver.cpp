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
#include "AudioDriver.h"
#include "Sound.h"
#include <cstddef>

// Include last!
#include "DebugNew.h" // IWYU pragma: keep
class AudioDriverLoaderInterface;

// Do not inline! That would break DLL compatibility: http://stackoverflow.com/questions/32444520/how-to-handle-destructors-in-dll-exported-interfaces
IAudioDriver::~IAudioDriver(){}

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

AudioDriver::AudioDriver(AudioDriverLoaderInterface* adli)
    : play_id_counter(1),  adli(adli), initialized(false)
{
}

AudioDriver::~AudioDriver()
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
const char* AudioDriver::GetName() const
{
    return NULL;
}

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
