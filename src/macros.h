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

#ifndef MACROS_H_INCLUDED
#define MACROS_H_INCLUDED

#pragma once

///////////////////////////////////////////////////////////////////////////////
// diverse Makros

#define GetBobImage(nation, nr) ( dynamic_cast<glArchivItem_Bitmap*>(LOADER.nation_bobs[nation].get(nr)) )
#define GetBobPlayerImage(nation, nr) ( dynamic_cast<glArchivItem_Bitmap_Player*>(LOADER.nation_bobs[nation].get(nr)) )
#define GetRomBob(nr) ( dynamic_cast<glArchivItem_Bitmap_Player*>(LOADER.rombobs_lst.get(nr)) )
#define GetMapBob(nr) ( GetMapImageN(nr) )

#define SmallFont ( LOADER.GetFontN("outline_fonts", 0) )
#define NormalFont ( LOADER.GetFontN("outline_fonts", 1) )
#define LargeFont ( LOADER.GetFontN("outline_fonts", 2) )

#if defined _WIN32 && defined _MSC_VER
#   define GetBobFile(file) ( dynamic_cast<glArchivItem_Bob*>(LOADER.##file##.get(0) ) )
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
#   define GetFont(type, nr)  ( dynamic_cast<glArchivItem_Font*>( LOADER.type.get(nr) ) )
#   define GetTxt(type, nr)   ( dynamic_cast<const libsiedler2::ArchivItem_Text*>( LOADER.type.get(nr) )->getText() )
#   define GetSound(type, nr) ( dynamic_cast<glArchivItem_Sound*>( LOADER.type.get(nr) ) )
#   define GetMusic(type, nr) ( dynamic_cast<glArchivItem_Music*>( LOADER.type.get(nr) ) )
#   define GetTxtItem(type, nr) ( dynamic_cast<libsiedler2::ArchivItem_Text*>( LOADER.type.get(nr) ) )
#endif // !_WIN32 || !_MSC_VER

#ifdef _MSC_VER
#   define FORCE_INLINE __forceinline
#else
#   define FORCE_INLINE inline __attribute__((__always_inline__))
#endif // _MSC_VER

// Macro that can be used to suppress unused warnings. Required e.g. for const boost::arrays defined in headers
// Don't use this if not absolutely necessary!
#ifdef __GNUC__
#   define SUPPRESS_UNUSED __attribute__((unused))
#else
#   define SUPPRESS_UNUSED
#endif

#if defined _WIN32 && defined _DEBUG && defined _MSC_VER && !defined NOCRTDBG
    // Check for heap corruption
#   define CHECK_HEAP_CORRUPTION _ASSERTE(_CrtCheckMemory());
#else
#   define CHECK_HEAP_CORRUPTION
#endif // _WIN32 && _DEBUG && !NOCRTDBG


#endif // !MACROS_H_INCLUDED
