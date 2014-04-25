// $Id: macros.h 9357 2014-04-25 15:35:25Z FloSoft $
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

#ifndef MACROS_H_INCLUDED
#define MACROS_H_INCLUDED

#pragma once

///////////////////////////////////////////////////////////////////////////////
// diverse Makros

#define GetBobImage(nation, nr) ( dynamic_cast<glArchivItem_Bitmap*>(LOADER.nation_bobs[nation].get(nr)) )
#define GetBobPlayerImage(nation, nr) ( dynamic_cast<glArchivItem_Bitmap_Player*>(LOADER.nation_bobs[nation].get(nr)) )
#define GetRomBob(nr) ( dynamic_cast<glArchivItem_Bitmap_Player*>(LOADER.rombobs_lst.get(nr)) )
#define GetMapBob(nr) ( GetMapImageN(nr) )
#define GetMapPlayerImage(nr) ( dynamic_cast<glArchivItem_Bitmap_Player*>(LOADER.map_lst.get(nr)) )

#define SmallFont ( LOADER.GetFontN("outline_fonts", 0) )
#define NormalFont ( LOADER.GetFontN("outline_fonts", 1) )
#define LargeFont ( LOADER.GetFontN("outline_fonts", 2) )

#if defined _WIN32 && defined _MSC_VER
#   define GetBobFile(file) ( dynamic_cast<glArchivItem_Bob*>(LOADER.##file##.get(0) ) )
#   define GetImage(type, nr) ( dynamic_cast<glArchivItem_Bitmap*>( LOADER.##type##.get(nr) ) )
#   define GetPlayerImage(type, nr) ( dynamic_cast<glArchivItem_Bitmap_Player*>( LOADER.##type##.get(nr) ) )
#   define GetFont(type, nr)  ( dynamic_cast<glArchivItem_Font*>( LOADER.##type##.get(nr) ) )
#   define GetSound(type, nr) ( dynamic_cast<glArchivItem_Sound*>( LOADER.##type##.get(nr) ) )
#   define GetMusic(type, nr) ( dynamic_cast<glArchivItem_Music*>( LOADER.##type##.get(nr) ) )
#   define GetTxtItem(type, nr) ( dynamic_cast<libsiedler2::ArchivItem_Text*>( LOADER.##type##.get(nr) ) )

#   ifdef _DEBUG
#       define GetTxt(type, nr) ( (LOADER.##type##.get(nr) ? dynamic_cast<const libsiedler2::ArchivItem_Text*>( LOADER.##type##.get(nr) )->getText() : "String ID " #type " (" #nr ") missing"))
#   else
#       define GetTxt(type, nr) ( dynamic_cast<const libsiedler2::ArchivItem_Text*>( LOADER.##type##.get(nr) )->getText() )
#   endif
#else
#   define GetBobFile(file) ( dynamic_cast<glArchivItem_Bob*>(LOADER.file.get(0) ) )
#   define GetImage(type, nr) ( dynamic_cast<glArchivItem_Bitmap*>( LOADER.type.get(nr) ) )
#   define GetPlayerImage(type, nr) ( dynamic_cast<glArchivItem_Bitmap_Player*>( LOADER.type.get(nr) ) )
#   define GetFont(type, nr)  ( dynamic_cast<glArchivItem_Font*>( LOADER.type.get(nr) ) )
#   define GetTxt(type, nr)   ( dynamic_cast<const libsiedler2::ArchivItem_Text*>( LOADER.type.get(nr) )->getText() )
#   define GetSound(type, nr) ( dynamic_cast<glArchivItem_Sound*>( LOADER.type.get(nr) ) )
#   define GetMusic(type, nr) ( dynamic_cast<glArchivItem_Music*>( LOADER.type.get(nr) ) )
#   define GetTxtItem(type, nr) ( dynamic_cast<libsiedler2::ArchivItem_Text*>( LOADER.type.get(nr) ) )
#endif // !_WIN32 || !_MSC_VER

#endif // !MACROS_H_INCLUDED
