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
#include "ogl/glSmartBitmap.h"
#include "gameTypes/MapTypes.h"
#include "gameData/AnimalConsts.h"
#include "gameTypes/BuildingTypes.h"
#include "gameTypes/JobTypes.h"
#include "helpers/multiArray.h"
#include "libsiedler2/src/ArchivInfo.h"
#include <boost/array.hpp>
#include <string>
#include <vector>
#include <map>
#include <stdint.h>

///////////////////////////////////////////////////////////////////////////////

class glArchivItem_Bitmap;
class glArchivItem_BitmapBase;
class glArchivItem_Bitmap_Player;
class glArchivItem_Bitmap_Raw;
class glArchivItem_Bob;
class glArchivItem_Font;
class glArchivItem_Sound;
class glTexturePacker;
namespace libsiedler2{
    class ArchivItem_Ini;
    class ArchivItem_Palette;
}

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

        Loader();
        /// Desktruktor von @p Loader.
        ~Loader() override;

        /// Lädt alle allgemeinen Dateien.
        bool LoadFilesAtStart();
        /// Lädt die Spieldateien.
        bool LoadFilesAtGame(unsigned char gfxset, bool* nations);
        /// Lädt Dateien von Addons.
        bool LoadFilesFromAddon(const AddonId id);
        void fillCaches();
        /// Deletes all loaded terrain textures
        void ClearTerrainTextures();
        /// Lädt das Terrain.
        bool CreateTerrainTextures();

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
        libsiedler2::ArchivInfo* ExtractAnimatedTexture(const Rect& rect, unsigned char color_count, unsigned char start_index, uint32_t colorShift = 0);

        bool LoadFilesFromArray(const unsigned int files_count, const unsigned int* files, bool isOriginal);
        bool LoadLsts(unsigned int dir);
        bool LoadFileOrDir(const std::string& file, const unsigned int file_id, bool isOriginal);

        static bool SortFilesHelper(const std::string& lhs, const std::string& rhs);
        static std::vector<std::string> ExplodeString(std::string const& line, const char delim, const unsigned int max = 0xFFFFFFFF);

    public:
        glArchivItem_Bitmap* GetImageN(const std::string& file, unsigned int nr);
        glArchivItem_Bitmap* GetImage(const std::string& file, const std::string& name);
        glArchivItem_Bitmap_Player* GetPlayerImage(const std::string& file, unsigned int nr);
        glArchivItem_Font* GetFontN(const std::string& file, unsigned int nr);
        libsiedler2::ArchivItem_Palette* GetPaletteN(const std::string& file, unsigned int nr = 0);
        glArchivItem_Sound* GetSoundN(const std::string& file, unsigned int nr);
        std::string GetTextN(const std::string& file, unsigned int nr);
        libsiedler2::ArchivInfo* GetInfoN(const std::string& file);
        glArchivItem_Bob* GetBobN(const std::string& file);
        glArchivItem_BitmapBase* GetNationImageN(unsigned int nation, unsigned int nr);
        glArchivItem_Bitmap* GetNationImage(unsigned int nation, unsigned int nr);
        glArchivItem_Bitmap_Player* GetNationPlayerImage(unsigned int nation, unsigned int nr);
        glArchivItem_Bitmap* GetMapImageN(unsigned int nr);
        glArchivItem_Bitmap_Player* GetMapPlayerImage(unsigned int nr);
        glArchivItem_Bitmap* GetTexImageN(unsigned int nr);
        libsiedler2::ArchivItem_Palette* GetTexPaletteN(unsigned int nr);
        libsiedler2::ArchivItem_Ini* GetSettingsIniN(const std::string& name);
        /// Returns the texture for the given terrain. For animated textures the given frame is returned
        glArchivItem_Bitmap& GetTerrainTexture(TerrainType t, unsigned animationFrame = 0);

    private:
        template<typename T>
        static T convertChecked(libsiedler2::ArchivItem* item){ T res = dynamic_cast<T>(item); RTTR_Assert(!item || res); return res; }
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

        glTexturePacker* stp;

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
