// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "glAllocator.h"
#include "glArchivItem_Bitmap_Player.h"
#include "glArchivItem_Bitmap_RLE.h"
#include "glArchivItem_Bitmap_Raw.h"
#include "glArchivItem_Bitmap_Shadow.h"
#include "glArchivItem_Bob.h"
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
        case libsiedler2::BobType::Bitmap: // unkomprimiertes Bitmap
            return std::make_unique<glArchivItem_Bitmap_Raw>();
        default: break;
    }
    return libsiedler2::StandardAllocator::create(type, subtype);
}
