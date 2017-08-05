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

#include "defines.h" // IWYU pragma: keep
#include "glArchivItem_Sound_XMidi.h"
#include "libsiedler2/src/ArchivItem_Sound_Midi.h"
#include "drivers/AudioDriverWrapper.h"
#include "Settings.h"

/**
 *  Spielt die Musik ab.
 *
 *  @param[in] volume Lautstärke der Musik.
 *  @param[in] loop   Endlosschleife ja/nein
 */
void glArchivItem_Sound_XMidi::Play(const unsigned repeats)
{
    if(!SETTINGS.sound.musik)
        return;

    if(!sound)
    {
        const libsiedler2::MIDI_Track* midiTrack = getMidiTrack(0);
        if(!midiTrack)
            return;
        libsiedler2::ArchivItem_Sound_Midi soundArchiv;
        soundArchiv.addTrack(*midiTrack);
        sound = AUDIODRIVER.LoadMusic(soundArchiv, ".midi");
    }

    if(sound)
        AUDIODRIVER.PlayMusic(sound, repeats);
}
