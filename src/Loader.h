// $Id: Loader.h 9357 2014-04-25 15:35:25Z FloSoft $
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
#ifndef LOADER_H_INCLUDED
#define LOADER_H_INCLUDED

#pragma once

#include "../libutil/src/Singleton.h"
#include "../libsiedler2/src/libsiedler2.h"
#include "const_addons.h"

///////////////////////////////////////////////////////////////////////////////

class glSmartBitmap;
class glSmartTexturePacker;

/// Loader Klasse.
class Loader : public Singleton<Loader>
{
    public:
        /// Konstruktor von @p Loader.
        Loader(void);
        /// Desktruktor von @p Loader.
        ~Loader(void);

        /// Lädt alle allgemeinen Dateien.
        bool LoadFilesAtStart(void);
        /// Lädt die Spieldateien.
        bool LoadFilesAtGame(unsigned char gfxset, bool* nations);
        /// Lädt Dateien von Addons.
        bool LoadFilesFromAddon(const AddonId id);
        void fillCaches();
        /// Lädt das Terrain.
        bool CreateTerrainTextures(void);

        /// Lädt die Settings.
        bool LoadSettings();
        /// Speichert die Settings.
        bool SaveSettings();

    protected:
        /// Lädt alle Sounds.
        inline bool LoadSounds();

    private:
        bool LoadFile(const std::string& pfad, const libsiedler2::ArchivItem_Palette* palette = NULL, bool load_always = true);
        bool LoadFile(const std::string& pfad, const libsiedler2::ArchivItem_Palette* palette, libsiedler2::ArchivInfo* archiv);
        bool LoadArchiv(const std::string& pfad, const libsiedler2::ArchivItem_Palette* palette, libsiedler2::ArchivInfo* archiv);
        void ExtractTexture(libsiedler2::ArchivInfo* destination, Rect& rect);
        void ExtractAnimatedTexture(libsiedler2::ArchivInfo* destination, Rect& rect, unsigned char color_count, unsigned char start_index);

        bool LoadFilesFromArray(const unsigned int files_count, const unsigned int* files, bool load_always = true);
        bool LoadLsts(unsigned int dir);
        bool LoadFileOrDir(const std::string& file, const unsigned int file_id, bool load_always);

        static bool SortFilesHelper(const std::string& lhs, const std::string& rhs);
        static std::vector<std::string> ExplodeString(std::string const& line, const char delim, const unsigned int max = 0xFFFFFFFF);

    public:
        inline glArchivItem_Bitmap* GetImageN(std::string file, unsigned int nr) { return dynamic_cast<glArchivItem_Bitmap*>( files[file].get(nr) ); }
        inline glArchivItem_Font* GetFontN(std::string file, unsigned int nr) { return dynamic_cast<glArchivItem_Font*>( files[file].get(nr) ); }
        inline libsiedler2::ArchivItem_Palette* GetPaletteN(std::string file, unsigned int nr = 0) { return dynamic_cast<libsiedler2::ArchivItem_Palette*>( files[file].get(nr) ); }
        inline glArchivItem_Sound* GetSoundN(std::string file, unsigned int nr) { return dynamic_cast<glArchivItem_Sound*>( files[file].get(nr) ); }
        inline const char* GetTextN(std::string file, unsigned int nr) { return dynamic_cast<libsiedler2::ArchivItem_Text*>( files[file].get(nr) ) ? dynamic_cast<libsiedler2::ArchivItem_Text*>( files[file].get(nr) )->getText() : "text missing"; }
        inline libsiedler2::ArchivInfo* GetInfoN(std::string file) { return dynamic_cast<libsiedler2::ArchivInfo*>( &files[file] ); }
        inline glArchivItem_Bob* GetBobN(std::string file) { return dynamic_cast<glArchivItem_Bob*>( files[file].get(0) ); };
        inline glArchivItem_Bitmap* GetNationImageN(unsigned int nation, unsigned int nr) { return dynamic_cast<glArchivItem_Bitmap*>(nation_gfx[nation]->get(nr)); }
        inline glArchivItem_Bitmap* GetMapImageN(unsigned int nr) { return dynamic_cast<glArchivItem_Bitmap*>(map_gfx->get(nr)); }
        inline glArchivItem_Bitmap* GetTexImageN(unsigned int nr) { return dynamic_cast<glArchivItem_Bitmap*>(tex_gfx->get(nr)); }
        inline libsiedler2::ArchivItem_Palette* GetTexPaletteN(unsigned int nr) { return dynamic_cast<libsiedler2::ArchivItem_Palette*>(tex_gfx->get(nr)); }
        inline libsiedler2::ArchivItem_Ini* GetSettingsIniN(std::string name) { return static_cast<libsiedler2::ArchivItem_Ini*>( GetInfoN(CONFIG_NAME)->find(name.c_str()) ); }

        // should not use this!
        const std::map<std::string, libsiedler2::ArchivInfo> &GetFiles(void) const { return files; }

    private:
        std::map<std::string, libsiedler2::ArchivInfo> files;
        unsigned char lastgfx;
        libsiedler2::ArchivInfo* nation_gfx[NATION_COUNT];
        libsiedler2::ArchivInfo* map_gfx;
        libsiedler2::ArchivInfo* tex_gfx;

    public:
        libsiedler2::ArchivInfo sng_lst;

        libsiedler2::ArchivInfo textures;
        libsiedler2::ArchivInfo water;
        libsiedler2::ArchivInfo lava;
        libsiedler2::ArchivInfo borders;
        libsiedler2::ArchivInfo roads;
        libsiedler2::ArchivInfo roads_points;

        glSmartTexturePacker* stp;

        static glSmartBitmap animal_cache[8][6][9];
        static glSmartBitmap building_cache[NATION_COUNT][40][2];
        static glSmartBitmap flag_cache[NATION_COUNT][3][8];
        static glSmartBitmap building_flag_cache[8];
        static glSmartBitmap tree_cache[9][15];
        static glSmartBitmap bob_jobs_cache[NATION_COUNT][33][6][8];
        static glSmartBitmap granite_cache[2][6];
        static glSmartBitmap grainfield_cache[2][4];
        static glSmartBitmap carrier_cache[35][6][8][2];
        static glSmartBitmap boundary_stone_cache[NATION_COUNT];
        static glSmartBitmap boat_cache[6][8];
        static glSmartBitmap donkey_cache[6][8];
        static glSmartBitmap gateway_cache[5];
};

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#define LOADER Loader::inst()

#endif // LOADER_H_INCLUDED
