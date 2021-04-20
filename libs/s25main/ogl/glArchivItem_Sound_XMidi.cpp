// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "glArchivItem_Sound_XMidi.h"
#include "drivers/AudioDriverWrapper.h"
#include "libsiedler2/ArchivItem_Sound_Midi.h"

SoundHandle glArchivItem_Sound_XMidi::Load()
{
    const libsiedler2::MIDI_Track& midiTrack = getMidiTrack(0);
    libsiedler2::ArchivItem_Sound_Midi soundArchiv;
    soundArchiv.addTrack(midiTrack);
    return AUDIODRIVER.LoadMusic(soundArchiv, ".midi");
}
