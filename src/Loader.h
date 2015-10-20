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
#ifndef LOADER_H_INCLUDED
#define LOADER_H_INCLUDED

#pragma once

#include "../libutil/src/Singleton.h"
#include "addons/const_addons.h"
#include "gameData/NationConsts.h"
#include "Rect.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glArchivItem_Font.h"
#include "ogl/glArchivItem_Sound.h"
#include "ogl/glArchivItem_Bob.h"
#include "ogl/glSmartBitmap.h" // Todo: Remove and use fwd decl
#include "gameTypes/MapTypes.h"
#include "gameData/AnimalConsts.h"
#include "gameTypes/BuildingTypes.h"
#include "gameTypes/JobTypes.h"
#include "helpers/multiArray.h"
#include "../libsiedler2/src/ArchivInfo.h"
#include "../libsiedler2/src/ArchivItem_Text.h"
#include "../libsiedler2/src/ArchivItem_Ini.h"
#include <boost/array.hpp>
#include <string>
#include <vector>
#include <map>

namespace libsiedler2{
    class ArchivItem_Palette;
    class ArchivItem_Ini;
}

///////////////////////////////////////////////////////////////////////////////

class glSmartBitmap;
class glSmartTexturePacker;
class glArchivItem_Bitmap_Raw;

const std::string CONFIG_NAME = "config";

/// Loader Klasse.
class Loader : public Singleton<Loader, SingletonPolicies::WithLongevity>
{
    /// Struct for storing loaded file entries
    struct FileEntry{
        libsiedler2::ArchivInfo archiv;
        /// True if the archiv was modified by overrides
        bool hasOverrides;
        FileEntry(): hasOverrides(false){}
    };
    public:
        BOOST_STATIC_CONSTEXPR unsigned Longevity = 19;

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
        /// Deletes all loaded terrain textures
        void ClearTerrainTextures();
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
        bool LoadFile(const std::string& pfad, const libsiedler2::ArchivItem_Palette* palette, bool isOriginal);
        bool LoadFile(const std::string& pfad, const libsiedler2::ArchivItem_Palette* palette, libsiedler2::ArchivInfo& archiv);
        bool LoadArchiv(const std::string& pfad, const libsiedler2::ArchivItem_Palette* palette, libsiedler2::ArchivInfo& archiv);
        glArchivItem_Bitmap_Raw* ExtractTexture(const Rect& rect);
        libsiedler2::ArchivInfo* ExtractAnimatedTexture(const Rect& rect, unsigned char color_count, unsigned char start_index);

        bool LoadFilesFromArray(const unsigned int files_count, const unsigned int* files, bool isOriginal);
        bool LoadLsts(unsigned int dir);
        bool LoadFileOrDir(const std::string& file, const unsigned int file_id, bool isOriginal);

        static bool SortFilesHelper(const std::string& lhs, const std::string& rhs);
        static std::vector<std::string> ExplodeString(std::string const& line, const char delim, const unsigned int max = 0xFFFFFFFF);

    public:
        inline glArchivItem_Bitmap* GetImageN(const std::string& file, unsigned int nr) { return dynamic_cast<glArchivItem_Bitmap*>( files_[file].archiv.get(nr) ); }
        inline glArchivItem_Font* GetFontN(const std::string& file, unsigned int nr) { return dynamic_cast<glArchivItem_Font*>( files_[file].archiv.get(nr) ); }
        inline libsiedler2::ArchivItem_Palette* GetPaletteN(const std::string& file, unsigned int nr = 0) { return dynamic_cast<libsiedler2::ArchivItem_Palette*>( files_[file].archiv.get(nr) ); }
        inline glArchivItem_Sound* GetSoundN(const std::string& file, unsigned int nr) { return dynamic_cast<glArchivItem_Sound*>( files_[file].archiv.get(nr) ); }
        inline std::string GetTextN(const std::string& file, unsigned int nr) { libsiedler2::ArchivItem_Text* archiv = dynamic_cast<libsiedler2::ArchivItem_Text*>( files_[file].archiv.get(nr) ); return archiv ? archiv->getText() : "text missing"; }
        inline libsiedler2::ArchivInfo* GetInfoN(const std::string& file) { return &files_[file].archiv; }
        inline glArchivItem_Bob* GetBobN(const std::string& file) { return dynamic_cast<glArchivItem_Bob*>( files_[file].archiv.get(0) ); };
        inline glArchivItem_Bitmap* GetNationImageN(unsigned int nation, unsigned int nr) { return dynamic_cast<glArchivItem_Bitmap*>(nation_gfx[nation]->get(nr)); }
        inline glArchivItem_Bitmap* GetMapImageN(unsigned int nr) { return dynamic_cast<glArchivItem_Bitmap*>(map_gfx->get(nr)); }
        inline glArchivItem_Bitmap* GetTexImageN(unsigned int nr) { return dynamic_cast<glArchivItem_Bitmap*>(tex_gfx->get(nr)); }
        inline libsiedler2::ArchivItem_Palette* GetTexPaletteN(unsigned int nr) { return dynamic_cast<libsiedler2::ArchivItem_Palette*>(tex_gfx->get(nr)); }
        inline libsiedler2::ArchivItem_Ini* GetSettingsIniN(const std::string& name) { return static_cast<libsiedler2::ArchivItem_Ini*>( GetInfoN(CONFIG_NAME)->find(name) ); }
        /// Returns the texture for the given terrain. For animated textures the given frame is returned
        glArchivItem_Bitmap& GetTerrainTexture(TerrainType t, unsigned animationFrame = 0);

    private:
        std::map<std::string, FileEntry> files_;
        /// Terraintextures (unanimated)
        std::map<TerrainType, glArchivItem_Bitmap*> terrainTextures;
        /// Terraintextures (animated) (currently only water and lava)
        std::map<TerrainType, libsiedler2::ArchivInfo*> terrainTexturesAnim;

        unsigned char lastgfx;
        boost::array<libsiedler2::ArchivInfo*, NAT_COUNT> nation_gfx;
        libsiedler2::ArchivInfo* map_gfx;
        libsiedler2::ArchivInfo* tex_gfx;

    public:
        libsiedler2::ArchivInfo sng_lst;

        libsiedler2::ArchivInfo borders;
        libsiedler2::ArchivInfo roads;
        libsiedler2::ArchivInfo roads_points;

        glSmartTexturePacker* stp;

        helpers::MultiArray<glSmartBitmap, 8, 6, ANIMAL_MAX_ANIMATION_STEPS + 1> animal_cache;
        helpers::MultiArray<glSmartBitmap, NAT_COUNT, BUILDING_TYPES_COUNT, 2> building_cache;
        helpers::MultiArray<glSmartBitmap, NAT_COUNT, 3, 8> flag_cache;
        helpers::MultiArray<glSmartBitmap, 8> building_flag_cache;
        helpers::MultiArray<glSmartBitmap, 9, 15> tree_cache;
        helpers::MultiArray<glSmartBitmap, NAT_COUNT, JOB_TYPES_COUNT + 1, 6, 8> bob_jobs_cache;
        helpers::MultiArray<glSmartBitmap, 2, 6> granite_cache;
        helpers::MultiArray<glSmartBitmap, 2, 4> grainfield_cache;
        helpers::MultiArray<glSmartBitmap, WARE_TYPES_COUNT, 6, 8, 2> carrier_cache;
        helpers::MultiArray<glSmartBitmap, NAT_COUNT> boundary_stone_cache;
        helpers::MultiArray<glSmartBitmap, 6, 8> boat_cache;
        helpers::MultiArray<glSmartBitmap, 6, 8> donkey_cache;
        helpers::MultiArray<glSmartBitmap, 5> gateway_cache;
};

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#define LOADER Loader::inst()

#endif // LOADER_H_INCLUDED
