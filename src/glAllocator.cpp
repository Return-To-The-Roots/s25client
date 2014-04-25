// $Id: glAllocator.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "glAllocator.h"

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
 *  @param[in] item    Das zu kopierende Item
 *
 *  @author FloSoft
 */
libsiedler2::ArchivItem* glAllocator(unsigned short type, unsigned short subtype, const libsiedler2::ArchivItem* item)
{
    if(item)
        type = item->getBobType();

    switch(type)
    {
        case libsiedler2::BOBTYPE_SOUND: // WAVs, MIDIs
        {
            const libsiedler2::baseArchivItem_Sound* i = dynamic_cast<const libsiedler2::baseArchivItem_Sound*>(item);
            if(item)
                subtype = i->getType();

            switch(subtype)
            {
                case libsiedler2::SOUNDTYPE_MIDI: // MIDI
                {
                    if(!item)
                        return new glArchivItem_Sound_Midi();
                    else
                        return new glArchivItem_Sound_Midi( dynamic_cast<const glArchivItem_Sound_Midi*>(item) );
                } break;
                case libsiedler2::SOUNDTYPE_WAVE: // WAV
                {
                    if(!item)
                        return new glArchivItem_Sound_Wave();
                    else
                        return new glArchivItem_Sound_Wave( dynamic_cast<const glArchivItem_Sound_Wave*>(item) );
                } break;
                case libsiedler2::SOUNDTYPE_XMIDI: // XMIDI
                {
                    if(!item)
                        return new glArchivItem_Sound_XMidi();
                    else
                        return new glArchivItem_Sound_XMidi( dynamic_cast<const glArchivItem_Sound_XMidi*>(item) );
                } break;
                case libsiedler2::SOUNDTYPE_OTHER: // Andere
                {
                    if(!item)
                        return new glArchivItem_Sound_Other();
                    else
                        return new glArchivItem_Sound_Other( dynamic_cast<const glArchivItem_Sound_Other*>(item) );
                } break;
                default:
                {
                    return libsiedler2::StandardAllocator(type, subtype, item);
                } break;
            }
        } break;
        case libsiedler2::BOBTYPE_BOB: // Bob-File
        {
            if(!item)
                return new glArchivItem_Bob();
            else
                return new glArchivItem_Bob( dynamic_cast<const glArchivItem_Bob*>(item) );
        } break;
        case libsiedler2::BOBTYPE_BITMAP_RLE: // RLE komprimiertes Bitmap
        {
            if(!item)
                return new glArchivItem_Bitmap_RLE();
            else
                return new glArchivItem_Bitmap_RLE( dynamic_cast<const glArchivItem_Bitmap_RLE*>(item) );
        } break;
        case libsiedler2::BOBTYPE_FONT: // Font
        {
            if(!item)
                return new glArchivItem_Font();
            else
                return new glArchivItem_Font( dynamic_cast<const glArchivItem_Font*>(item) );
        } break;
        case libsiedler2::BOBTYPE_BITMAP_PLAYER: // Bitmap mit spezifischer Spielerfarbe
        {
            if(!item)
                return new glArchivItem_Bitmap_Player();
            else
                return new glArchivItem_Bitmap_Player( dynamic_cast<const glArchivItem_Bitmap_Player*>(item) );
        } break;
        case libsiedler2::BOBTYPE_BITMAP_SHADOW: // Schatten
        {
            if(!item)
                return new glArchivItem_Bitmap_Shadow();
            else
                return new glArchivItem_Bitmap_Shadow( dynamic_cast<const glArchivItem_Bitmap_Shadow*>(item) );
        } break;
        case libsiedler2::BOBTYPE_MAP: // Map
        {
            if(!item)
                return new glArchivItem_Map();
            else
                return new glArchivItem_Map( dynamic_cast<const glArchivItem_Map*>(item) );
        } break;
        case libsiedler2::BOBTYPE_BITMAP_RAW: // unkomprimiertes Bitmap
        {
            if(!item)
                return new glArchivItem_Bitmap_Raw();
            else
                return new glArchivItem_Bitmap_Raw( dynamic_cast<const glArchivItem_Bitmap_Raw*>(item) );
        } break;
        default:
            break;
    }
    return libsiedler2::StandardAllocator(type, subtype, item);
}
