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
#include "helpers/MaxEnumValue.h"
#include "helpers/MultiArray.h"
#include "ogl/glSmartBitmap.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/JobTypes.h"
#include "gameTypes/Nation.h"
#include "gameData/AnimalConsts.h"
#include "libsiedler2/Archiv.h"
#include <boost/filesystem/path.hpp>
#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

enum class AddonId;
class ITexture;
class glArchivItem_Bitmap;
class glArchivItem_BitmapBase;
class glArchivItem_Bitmap_Player;
class glArchivItem_Bob;
class glFont;
class SoundEffectItem;
class glTexturePacker;
class MusicItem;
class Log;
class RttrConfig;
namespace libsiedler2 {
class ArchivItem_Ini;
class ArchivItem_Palette;
} // namespace libsiedler2

/// Font sizes for which we have glyph sets
enum class FontSize
{
    Small,
    Normal,
    Large
};
DEFINE_MAX_ENUM_VALUE(FontSize, FontSize::Large)

/// Identifier for resources
using ResourceId = std::string;

class Loader
{
    /// Struct for storing loaded file entries
    struct FileEntry
    {
        libsiedler2::Archiv archiv;
        /// List of files used to build this archiv
        std::vector<boost::filesystem::path> filesUsed;
        bool loadedAfterOverrideChange;
    };
    struct OverrideFolder
    {
        /// Path to the folder
        boost::filesystem::path path;
        /// Filenames in the folder
        std::vector<boost::filesystem::path> files;
    };

public:
    Loader(Log&, const RttrConfig&);
    ~Loader();

    /// Add a folder to the list of folders containing overrides. Files in folders added last will override prior ones
    /// Paths with macros will be resolved
    void AddOverrideFolder(const std::string& path, bool atBack = true);
    void AddOverrideFolder(const char* path, bool atBack = true) { AddOverrideFolder(std::string(path), atBack); }
    /// Add a folder to the list of folders containing overrides. Files in folders added last will override prior ones
    void AddOverrideFolder(const boost::filesystem::path& path, bool atBack = true);
    /// Add the folder form an addon to the override folders
    void AddAddonFolder(AddonId id);
    void ClearOverrideFolders();

    /// Load general files required also outside of games
    bool LoadFilesAtStart();
    bool LoadFonts();
    /// Load files required during a game
    bool LoadFilesAtGame(const std::string& mapGfxPath, bool isWinterGFX, const std::vector<Nation>& nations);
    /// Load all files from the override folders that have not been loaded yet
    bool LoadOverrideFiles();
    /// Load all given files with the default palette
    bool LoadFiles(const std::vector<std::string>& files);

    /// Creates archives with empty files for the GUI (for testing purposes)
    void LoadDummyGUIFiles();
    /// Load a file and save it into the loader repo
    bool Load(const boost::filesystem::path& path, const libsiedler2::ArchivItem_Palette* palette = nullptr,
              bool isFromOverrideDir = false);
    /// Load a file or directory and its overrides into the archiv
    bool Load(libsiedler2::Archiv& archiv, const boost::filesystem::path& path, const libsiedler2::ArchivItem_Palette* palette = nullptr);

    void fillCaches();
    static std::unique_ptr<glArchivItem_Bitmap> ExtractTexture(const glArchivItem_Bitmap& srcImg, const Rect& rect);
    static std::unique_ptr<libsiedler2::Archiv> ExtractAnimatedTexture(const glArchivItem_Bitmap& srcImg, const Rect& rect,
                                                                       uint8_t start_index, uint8_t color_count);

    glArchivItem_Bitmap* GetImageN(const ResourceId& file, unsigned nr);
    /// Same as GetImageN but returns a ITexture. Note glArchivItem_Bitmap is a ITexture
    ITexture* GetTextureN(const ResourceId& file, unsigned nr);
    glArchivItem_Bitmap* GetImage(const ResourceId& file, const std::string& name);
    glArchivItem_Bitmap_Player* GetPlayerImage(const ResourceId& file, unsigned nr);
    glFont* GetFont(FontSize);
    libsiedler2::ArchivItem_Palette* GetPaletteN(const ResourceId& file, unsigned nr = 0);
    SoundEffectItem* GetSoundN(const ResourceId& file, unsigned nr);
    std::string GetTextN(const ResourceId& file, unsigned nr);
    libsiedler2::Archiv& GetArchive(const ResourceId& file);
    glArchivItem_Bob* GetBob(const ResourceId& file);
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

    std::vector<std::unique_ptr<MusicItem>> sng_lst;

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
    std::array<glSmartBitmap, NUM_NATS> boundary_stone_cache;
    /// BoatCarrier: Direction, AnimationFrame
    helpers::MultiArray<glSmartBitmap, 6, 8> boat_cache;
    /// Donkey: Direction, AnimationFrame
    helpers::MultiArray<glSmartBitmap, 6, 8> donkey_cache;
    /// Gateway: AnimationFrame
    std::array<glSmartBitmap, 5> gateway_cache;

private:
    static ResourceId MakeResourceId(const boost::filesystem::path& filepath);

    /// Get all files to load for a request of loading filepath
    std::vector<boost::filesystem::path> GetFilesToLoad(const boost::filesystem::path& filepath);
    bool MergeArchives(libsiedler2::Archiv& targetArchiv, libsiedler2::Archiv& otherArchiv);

    /// Load all sounds
    bool LoadSounds();

    /// Load a file or directory into the archive
    libsiedler2::Archiv DoLoadFileOrDirectory(const boost::filesystem::path& filePath,
                                              const libsiedler2::ArchivItem_Palette* palette = nullptr);
    /// Load the file into the archive
    libsiedler2::Archiv DoLoadFile(const boost::filesystem::path& filePath, const libsiedler2::ArchivItem_Palette* palette = nullptr);
    bool LoadOverrideDirectory(const boost::filesystem::path& path);

    template<typename T>
    static T convertChecked(libsiedler2::ArchivItem* item)
    {
        T res = dynamic_cast<T>(item);
        RTTR_Assert(!item || res);
        return res;
    }
    Log& logger_;
    const RttrConfig& config_;
    std::vector<OverrideFolder> overrideFolders_;
    std::map<ResourceId, FileEntry> files_;
    std::vector<glFont> fonts;

    bool isWinterGFX_;
    std::array<libsiedler2::Archiv*, NUM_NATS> nation_gfx;
    libsiedler2::Archiv* map_gfx;
    std::unique_ptr<glTexturePacker> stp;
};

///////////////////////////////////////////////////////////////////////////////
Loader& getGlobalLoader();
#define LOADER getGlobalLoader()

// Helper macros for easy access to fonts
#define SmallFont (LOADER.GetFont(FontSize::Small))
#define NormalFont (LOADER.GetFont(FontSize::Normal))
#define LargeFont (LOADER.GetFont(FontSize::Large))

#endif // LOADER_H_INCLUDED
