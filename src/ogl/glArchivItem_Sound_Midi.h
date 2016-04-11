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
#ifndef GLARCHIVITEM_SOUND_MIDI_H_INCLUDED
#define GLARCHIVITEM_SOUND_MIDI_H_INCLUDED

#pragma once

#include "../libsiedler2/src/ArchivItem_Sound_Midi.h"
#include "glArchivItem_Music.h"

class glArchivItem_Sound_Midi : public libsiedler2::baseArchivItem_Sound_Midi, public glArchivItem_Music
{
    public:
        glArchivItem_Sound_Midi() : baseArchivItem_Sound(), baseArchivItem_Sound_Midi(), glArchivItem_Music() {}

        glArchivItem_Sound_Midi(const glArchivItem_Sound_Midi& item) : baseArchivItem_Sound(item), baseArchivItem_Sound_Midi(item), glArchivItem_Music(item) {}

        /// Spielt die Musik ab.
        void Play(const unsigned repeats) override;
};

#endif // !GLARCHIVITEM_SOUND_MIDI_H_INCLUDED
