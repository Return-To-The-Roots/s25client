// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "glAllocator.h"
#include "glArchivItem_Bitmap_Player.h"
#include "glArchivItem_Bitmap_RLE.h"
#include "glArchivItem_Bitmap_Raw.h"
#include "glArchivItem_Bitmap_Shadow.h"
#include "glArchivItem_Bob.h"
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
std::unique_ptr<libsiedler2::ArchivItem> GlAllocator::create(libsiedler2::BobType type,
                                                             libsiedler2::SoundType subtype) const
{
    switch(type)
    {
        case libsiedler2::BobType::Sound: // WAVs, MIDIs
            switch(subtype)
            {
                case libsiedler2::SoundType::None: break;
                case libsiedler2::SoundType::Midi: // MIDI
                    return std::make_unique<glArchivItem_Sound_Midi>();
                case libsiedler2::SoundType::Wave: // WAV
                    return std::make_unique<glArchivItem_Sound_Wave>();
                case libsiedler2::SoundType::XMidi: // XMIDI
                    return std::make_unique<glArchivItem_Sound_XMidi>();
                default: // Andere
                    return std::make_unique<glArchivItem_Sound_Other>(subtype);
            }
            break;
        case libsiedler2::BobType::Bob: // Bob-File
            return std::make_unique<glArchivItem_Bob>();
        case libsiedler2::BobType::BitmapRLE: // RLE komprimiertes Bitmap
            return std::make_unique<glArchivItem_Bitmap_RLE>();
        case libsiedler2::BobType::BitmapPlayer: // Bitmap mit spezifischer Spielerfarbe
            return std::make_unique<glArchivItem_Bitmap_Player>();
        case libsiedler2::BobType::BitmapShadow: // Schatten
            return std::make_unique<glArchivItem_Bitmap_Shadow>();
        case libsiedler2::BobType::Map: // Map
            return std::make_unique<glArchivItem_Map>();
        case libsiedler2::BobType::Bitmap: // unkomprimiertes Bitmap
            return std::make_unique<glArchivItem_Bitmap_Raw>();
        default: break;
    }
    return libsiedler2::StandardAllocator::create(type, subtype);
}
