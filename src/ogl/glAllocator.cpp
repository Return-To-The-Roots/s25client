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
#include "glAllocator.h"
#include "glArchivItem_Bitmap_Player.h"
#include "glArchivItem_Bitmap_RLE.h"
#include "glArchivItem_Bitmap_Raw.h"
#include "glArchivItem_Bitmap_Shadow.h"
#include "glArchivItem_Bob.h"
#include "glArchivItem_Font.h"
#include "glArchivItem_Map.h"
#include "glArchivItem_Sound_Midi.h"
#include "glArchivItem_Sound_Other.h"
#include "glArchivItem_Sound_Wave.h"
#include "glArchivItem_Sound_XMidi.h"

/**
 *  Der GL-Item-Allocator.
 *
 *  @param[in] type    Der Typ des Items
 *  @param[in] subtype Der Subtyp des Items
 */
libsiedler2::ArchivItem* GlAllocator::create(libsiedler2::BobType type, libsiedler2::SoundType subtype) const
{
    switch(type)
    {
        case libsiedler2::BOBTYPE_SOUND: // WAVs, MIDIs
            switch(subtype)
            {
                case libsiedler2::SOUNDTYPE_NONE: break;
                case libsiedler2::SOUNDTYPE_MIDI: // MIDI
                    return new glArchivItem_Sound_Midi();
                case libsiedler2::SOUNDTYPE_WAVE: // WAV
                    return new glArchivItem_Sound_Wave();
                case libsiedler2::SOUNDTYPE_XMIDI: // XMIDI
                    return new glArchivItem_Sound_XMidi();
                default: // Andere
                    return new glArchivItem_Sound_Other(subtype);
            }
            break;
        case libsiedler2::BOBTYPE_BOB: // Bob-File
            return new glArchivItem_Bob();
        case libsiedler2::BOBTYPE_BITMAP_RLE: // RLE komprimiertes Bitmap
            return new glArchivItem_Bitmap_RLE();
        case libsiedler2::BOBTYPE_FONT: // Font
            return new glArchivItem_Font();
        case libsiedler2::BOBTYPE_BITMAP_PLAYER: // Bitmap mit spezifischer Spielerfarbe
            return new glArchivItem_Bitmap_Player();
        case libsiedler2::BOBTYPE_BITMAP_SHADOW: // Schatten
            return new glArchivItem_Bitmap_Shadow();
        case libsiedler2::BOBTYPE_MAP: // Map
            return new glArchivItem_Map();
        case libsiedler2::BOBTYPE_BITMAP_RAW: // unkomprimiertes Bitmap
            return new glArchivItem_Bitmap_Raw();
        default: break;
    }
    return libsiedler2::StandardAllocator::create(type, subtype);
}
