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
#ifndef LOADER_H_INCLUDED
#define LOADER_H_INCLUDED

#pragma once

#include "Rect.h"
#include "helpers/multiArray.h"
#include "ogl/glSmartBitmap.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/JobTypes.h"
#include "gameTypes/MapTypes.h"
#include "gameData/AnimalConsts.h"
#include "gameData/NationConsts.h"
#include "libsiedler2/Archiv.h"
#include "libutil/Singleton.h"
#include "libutil/unique_ptr.h"
#include <boost/array.hpp>
#include <map>
#include <string>
#include <vector>

struct AddonId;
class ITexture;
class glArchivItem_Bitmap;
class glArchivItem_BitmapBase;
class glArchivItem_Bitmap_Player;
class glArchivItem_Bitmap_Raw;
class glArchivItem_Bob;
class glArchivItem_Font;
class SoundEffectItem;
class glTexturePacker;
namespace libsiedler2 {
class ArchivItem_Ini;
class ArchivItem_Palette;
} // namespace libsiedler2

/// Loader Klasse.
class Loader : public Singleton<Loader, SingletonPolicies::WithLongevity>
{
    /// Struct for storing loaded file entries
    struct FileEntry
    {
        libsiedler2::Archiv archiv;
        /// List of files used to build this archiv
        std::vector<std::string> filesUsed;
        bool loadedAfterOverrideChange;
    };
    struct OverrideFolder
    {
        /// Path to the folder
        std::string path;
        /// Filenames in the folder
        std::vector<std::string> files;
    };

public:
    BOOST_STATIC_CONSTEXPR unsigned Longevity = 19;

    Loader();
    ~Loader() override;

    /// Add a folder to the list of folders containing overrides. Files in folders added last will override prior ones
    /// Paths with macros will be resolved
    void AddOverrideFolder(std::string path, bool atBack = true);
    /// Add the folder form an addon to the override folders
    void AddAddonFolder(AddonId id);
    void ClearOverrideFolders();

    /// Lädt alle allgemeinen Dateien.
    bool LoadFilesAtStart();
    /// Lädt die Spieldateien.
    bool LoadFilesAtGame(const std::string& mapGfxPath, bool isWinterGFX, const std::vector<bool>& nations);
    /// Load all files from the override folders that have not been use yet
    bool LoadOverrideFiles();
    /// Load all given files with the default palette
    bool LoadFiles(const std::vector<std::string>& files);

    /// Creates archives with empty files for the GUI (for testing purposes)
    void LoadDummyGUIFiles();
    /// Load a file and save it into the loader repo
    bool LoadFile(const std::string& pfad, const libsiedler2::ArchivItem_Palette* palette = NULL, bool isFromOverrideDir = false);
    /// Load a file into the archiv
    bool LoadFile(libsiedler2::Archiv& archiv, const std::string& pfad, const libsiedler2::ArchivItem_Palette* palette = NULL);

    void fillCaches();
    static libutil::unique_ptr<glArchivItem_Bitmap> ExtractTexture(const glArchivItem_Bitmap& srcImg, const Rect& rect);
    static libutil::unique_ptr<libsiedler2::Archiv> ExtractAnimatedTexture(const glArchivItem_Bitmap& srcImg, const Rect& rect,
                                                                           uint8_t start_index, uint8_t color_count);

    glArchivItem_Bitmap* GetImageN(const std::string& file, unsigned nr);
    /// Same as GetImageN but returns a ITexture. Note glArchivItem_Bitmap is a ITexture
    ITexture* GetTextureN(const std::string& file, unsigned nr);
    glArchivItem_Bitmap* GetImage(const std::string& file, const std::string& name);
    glArchivItem_Bitmap_Player* GetPlayerImage(const std::string& file, unsigned nr);
    glArchivItem_Font* GetFontN(const std::string& file, unsigned nr);
    libsiedler2::ArchivItem_Palette* GetPaletteN(const std::string& file, unsigned nr = 0);
    SoundEffectItem* GetSoundN(const std::string& file, unsigned nr);
    std::string GetTextN(const std::string& file, unsigned nr);
    libsiedler2::Archiv& GetInfoN(const std::string& file);
    glArchivItem_Bob* GetBobN(const std::string& file);
    glArchivItem_BitmapBase* GetNationImageN(unsigned nation, unsigned nr);
    glArchivItem_Bitmap* GetNationImage(unsigned nation, unsigned nr);
    /// Same as GetNationImage but returns a ITexture. Note glArchivItem_Bitmap is a ITexture
    ITexture* GetNationTex(unsigned nation, unsigned nr);
    glArchivItem_Bitmap_Player* GetNationPlayerImage(unsigned nation, unsigned nr);
    glArchivItem_Bitmap* GetMapImageN(unsigned nr);
    /// Same as GetMapImageN but returns a ITexture. Note glArchivItem_Bitmap is a ITexture
    ITexture* GetMapTexN(unsigned nr);
    glArchivItem_Bitmap_Player* GetMapPlayerImage(unsigned nr);

    bool IsWinterGFX() const { return isWinterGFX_; }

    libsiedler2::Archiv sng_lst;

    /// Animals: Species, Direction, AnimationFrame(Last = Dead)
    helpers::MultiArray<glSmartBitmap, NUM_SPECS, 6, ANIMAL_MAX_ANIMATION_STEPS + 1> animal_cache;
    /// Buildings: Nation, Type, Building/Skeleton
    helpers::MultiArray<glSmartBitmap, NUM_NATS, NUM_BUILDING_TYPES, 2> building_cache;
    /// Flags: Nation, Type, AnimationFrame
    helpers::MultiArray<glSmartBitmap, NUM_NATS, 3, 8> flag_cache;
    /// Military Flags: AnimationFrame
    // helpers::MultiArray<glSmartBitmap, 8> building_flag_cache;
    /// Trees: Type, AnimationFrame
    helpers::MultiArray<glSmartBitmap, 9, 15> tree_cache;
    /// Jobs: Nation, Job (last is fat carrier), Direction, AnimationFrame
    helpers::MultiArray<glSmartBitmap, NUM_NATS, NUM_JOB_TYPES + 1, 6, 8> bob_jobs_cache;
    /// Stone: Type, Size
    helpers::MultiArray<glSmartBitmap, 2, 6> granite_cache;
    /// Grainfield: Type, Size
    helpers::MultiArray<glSmartBitmap, 2, 4> grainfield_cache;
    /// Carrier w/ ware: Ware, Direction, Animation, NormalOrFat
    helpers::MultiArray<glSmartBitmap, NUM_WARE_TYPES, 6, 8, 2> carrier_cache;
    /// Boundary stones: Nation
    helpers::MultiArray<glSmartBitmap, NUM_NATS> boundary_stone_cache;
    /// BoatCarrier: Direction, AnimationFrame
    helpers::MultiArray<glSmartBitmap, 6, 8> boat_cache;
    /// Donkey: Direction, AnimationFrame
    helpers::MultiArray<glSmartBitmap, 6, 8> donkey_cache;
    /// Gateway: AnimationFrame
    helpers::MultiArray<glSmartBitmap, 5> gateway_cache;

private:
    /// Get all files to load for a request of loading filepath
    std::vector<std::string> GetFilesToLoad(const std::string& filepath);
    static bool MergeArchives(libsiedler2::Archiv& targetArchiv, libsiedler2::Archiv& otherArchiv);

    /// Lädt alle Sounds.
    bool LoadSounds();

    /// Load a file into the archiv
    bool LoadSingleFile(libsiedler2::Archiv& archiv, const std::string& pfad, const libsiedler2::ArchivItem_Palette* palette = NULL);
    bool LoadArchiv(libsiedler2::Archiv& archiv, const std::string& pfad, const libsiedler2::ArchivItem_Palette* palette = NULL);
    bool LoadOverrideDirectory(const std::string& path);
    bool LoadFilesFromArray(const std::vector<unsigned>& files);

    template<typename T>
    static T convertChecked(libsiedler2::ArchivItem* item)
    {
        T res = dynamic_cast<T>(item);
        RTTR_Assert(!item || res);
        return res;
    }
    std::vector<OverrideFolder> overrideFolders_;
    std::map<std::string, FileEntry> files_;

    bool isWinterGFX_;
    boost::array<libsiedler2::Archiv*, NUM_NATS> nation_gfx;
    libsiedler2::Archiv* map_gfx;
    glTexturePacker* stp;
};

///////////////////////////////////////////////////////////////////////////////
#define LOADER Loader::inst()

// Helper macros for easy access to fonts
#define SmallFont (LOADER.GetFontN("outline_fonts", 0))
#define NormalFont (LOADER.GetFontN("outline_fonts", 1))
#define LargeFont (LOADER.GetFontN("outline_fonts", 2))

#endif // LOADER_H_INCLUDED
