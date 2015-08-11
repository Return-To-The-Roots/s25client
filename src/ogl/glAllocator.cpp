// $Id: glAllocator.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "glAllocator.h"

// @Todo: Remove this includes
#include "glArchivItem_Sound.h"
#include "glArchivItem_Music.h"
#include "glArchivItem_Sound_Wave.h"
#include "glArchivItem_Sound_Midi.h"
#include "glArchivItem_Sound_XMidi.h"
#include "glArchivItem_Sound_Other.h"

#include "glArchivItem_Bitmap.h"
#include "glArchivItem_Bitmap_RLE.h"
#include "glArchivItem_Bitmap_Player.h"
#include "glArchivItem_Bitmap_Shadow.h"
#include "glArchivItem_Bitmap_Raw.h"
#include "glArchivItem_Bitmap_Direct.h"

#include "glArchivItem_Bob.h"
#include "glArchivItem_Font.h"
#include "glArchivItem_Map.h"


///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  Der GL-Item-Allocator.
 *
 *  @param[in] type    Der Typ des Items
 *  @param[in] subtype Der Subtyp des Items
 *
 *  @author FloSoft
 */
libsiedler2::ArchivItem* GlAllocator::create(libsiedler2::BOBTYPES type, libsiedler2::SOUNDTYPES subtype) const
{
    switch(type)
    {
        case libsiedler2::BOBTYPE_SOUND: // WAVs, MIDIs
            switch(subtype)
            {
                case libsiedler2::SOUNDTYPE_MIDI: // MIDI
                    return new glArchivItem_Sound_Midi();
                case libsiedler2::SOUNDTYPE_WAVE: // WAV
                    return new glArchivItem_Sound_Wave();
                case libsiedler2::SOUNDTYPE_XMIDI: // XMIDI
                    return new glArchivItem_Sound_XMidi();
                case libsiedler2::SOUNDTYPE_OTHER: // Andere
                    return new glArchivItem_Sound_Other();
                default:
                    break;
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
        default:
            break;
    }
    return libsiedler2::StandardAllocator::create(type, subtype);
}

/**
 *  Der GL-Item-Allocator.
 *
 *  @param[in] item    Das zu kopierende Item
 *
 *  @author FloSoft
 */
libsiedler2::ArchivItem* GlAllocator::clone(const libsiedler2::ArchivItem& item) const
{
    libsiedler2::BOBTYPES type = static_cast<libsiedler2::BOBTYPES>(item.getBobType());

    switch(type)
    {
        case libsiedler2::BOBTYPE_SOUND:   // WAVs, MIDIs
        {
            const libsiedler2::baseArchivItem_Sound& soundItem = dynamic_cast<const libsiedler2::baseArchivItem_Sound&>(item);
            libsiedler2::SOUNDTYPES subtype = static_cast<libsiedler2::SOUNDTYPES>(soundItem.getType());

            switch(subtype)
            {
                case libsiedler2::SOUNDTYPE_MIDI: // MIDI
                    return new glArchivItem_Sound_Midi(dynamic_cast<const glArchivItem_Sound_Midi&>(item));
                case libsiedler2::SOUNDTYPE_WAVE: // WAV
                    return new glArchivItem_Sound_Wave(dynamic_cast<const glArchivItem_Sound_Wave&>(item));
                case libsiedler2::SOUNDTYPE_XMIDI: // XMIDI
                    return new glArchivItem_Sound_XMidi(dynamic_cast<const glArchivItem_Sound_XMidi&>(item));
                case libsiedler2::SOUNDTYPE_OTHER: // Andere
                    return new glArchivItem_Sound_Other(dynamic_cast<const glArchivItem_Sound_Other&>(item));
                default:
                    break;
            }
            break;
        }
        case libsiedler2::BOBTYPE_BOB: // Bob-File
            return new glArchivItem_Bob(dynamic_cast<const glArchivItem_Bob&>(item));
        case libsiedler2::BOBTYPE_BITMAP_RLE: // RLE komprimiertes Bitmap
            return new glArchivItem_Bitmap_RLE(dynamic_cast<const glArchivItem_Bitmap_RLE&>(item));
        case libsiedler2::BOBTYPE_FONT: // Font
            return new glArchivItem_Font(dynamic_cast<const glArchivItem_Font&>(item));
        case libsiedler2::BOBTYPE_BITMAP_PLAYER: // Bitmap mit spezifischer Spielerfarbe
            return new glArchivItem_Bitmap_Player(dynamic_cast<const glArchivItem_Bitmap_Player&>(item));
        case libsiedler2::BOBTYPE_BITMAP_SHADOW: // Schatten
            return new glArchivItem_Bitmap_Shadow(dynamic_cast<const glArchivItem_Bitmap_Shadow&>(item));
        case libsiedler2::BOBTYPE_MAP: // Map
            return new glArchivItem_Map(dynamic_cast<const glArchivItem_Map&>(item));
        case libsiedler2::BOBTYPE_BITMAP_RAW: // unkomprimiertes Bitmap
            return new glArchivItem_Bitmap_Raw(dynamic_cast<const glArchivItem_Bitmap_Raw&>(item));
        default:
            break;
    }
    return libsiedler2::StandardAllocator::clone(item);
}
