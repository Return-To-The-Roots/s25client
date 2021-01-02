// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#pragma once

#include "Rect.h"
#include "enum_cast.hpp"
#include "helpers/MultiArray.h"
#include "ogl/glSmartBitmap.h"
#include "resources/ResourceId.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/Direction.h"
#include "gameTypes/FlagType.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/JobTypes.h"
#include "gameTypes/Nation.h"
#include "gameData/AnimalConsts.h"
#include <boost/filesystem/path.hpp>
#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

class ArchiveLoader;
class ArchiveLocator;
class glArchivItem_Bitmap;
class glArchivItem_Bitmap_Player;
class glArchivItem_BitmapBase;
class glArchivItem_Bob;
class glFont;
class glTexturePacker;
class ITexture;
class Log;
class MusicItem;
class RttrConfig;
class SoundEffectItem;
enum class AddonId;

namespace libsiedler2 {
class Archiv;
class ArchivItem;
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
constexpr auto maxEnumValue(FontSize)
{
    return FontSize::Large;
}

void addDefaultResourceFolders(const RttrConfig& config, ArchiveLocator& locator,
                               const std::vector<Nation>& usedNations, const std::vector<AddonId>& enabledAddons);

class Loader
{
    /// Struct for storing loaded file entries
    struct FileEntry;

public:
    Loader(Log&, const RttrConfig&);
    ~Loader();

    void initResourceFolders() { initResourceFolders({}, {}); }
    void initResourceFolders(const std::vector<Nation>& usedNations, const std::vector<AddonId>& enabledAddons);

    /// Load general files required also outside of games
    bool LoadFilesAtStart();
    bool LoadFonts();
    /// Load files required during a game
    bool LoadFilesAtGame(const std::string& mapGfxPath, bool isWinterGFX, const std::vector<Nation>& nations,
                         const std::vector<AddonId>& enabledAddons);
    /// Load all given files with the default palette
    bool LoadFiles(const std::vector<std::string>& files);
    bool LoadResources(const std::vector<ResourceId>& resources);

    /// Creates archives with empty files for the GUI (for testing purposes)
    void LoadDummyGUIFiles();
    /// Load a file and save it into the loader repo
    bool Load(const boost::filesystem::path& path, const libsiedler2::ArchivItem_Palette* palette = nullptr);
    bool Load(const ResourceId& resId, const libsiedler2::ArchivItem_Palette* palette = nullptr);
    /// Load a file or directory and its overrides into the archive
    bool Load(libsiedler2::Archiv& archive, const boost::filesystem::path& path,
              const libsiedler2::ArchivItem_Palette* palette = nullptr);
    bool Load(libsiedler2::Archiv& archive, const ResourceId& resId,
              const libsiedler2::ArchivItem_Palette* palette = nullptr);

    void fillCaches();
    static std::unique_ptr<glArchivItem_Bitmap> ExtractTexture(const glArchivItem_Bitmap& srcImg, const Rect& rect);
    static std::unique_ptr<libsiedler2::Archiv> ExtractAnimatedTexture(const glArchivItem_Bitmap& srcImg,
                                                                       const Rect& rect, uint8_t start_index,
                                                                       uint8_t color_count);

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
    glArchivItem_BitmapBase* GetNationImageN(Nation nation, unsigned nr);
    glArchivItem_Bitmap* GetNationImage(Nation nation, unsigned nr);
    glArchivItem_Bitmap* GetNationIcon(Nation nation, BuildingType bld);
    /// Same as GetNationImage but returns a ITexture. Note glArchivItem_Bitmap is a ITexture
    ITexture* GetNationTex(Nation nation, unsigned nr);
    glArchivItem_Bitmap_Player* GetNationPlayerImage(Nation nation, unsigned nr);
    glArchivItem_Bitmap* GetMapImageN(unsigned nr);
    /// Same as GetMapImageN but returns a ITexture. Note glArchivItem_Bitmap is a ITexture
    ITexture* GetMapTexN(unsigned nr);
    /// Get the ware symbol texture
    ITexture* GetWareTex(GoodType ware) { return GetMapTexN(WARES_TEX_MAP_OFFSET + rttr::enum_cast(ware)); }
    /// Get the ware stack texture (lying on ground)
    ITexture* GetWareStackTex(GoodType ware) { return GetMapTexN(WARE_STACK_TEX_MAP_OFFSET + rttr::enum_cast(ware)); }
    /// Get the ware texture when carried by donky
    ITexture* GetWareDonkeyTex(GoodType ware)
    {
        return GetMapTexN(WARES_DONKEY_TEX_MAP_OFFSET + rttr::enum_cast(ware));
    }
    /// Get job symbol texture
    ITexture* GetJobTex(Job job)
    {
        return (job == Job::CharBurner) ? GetTextureN("io_new", 5) : GetMapTexN(2300 + rttr::enum_cast(job));
    }
    glArchivItem_Bitmap_Player* GetMapPlayerImage(unsigned nr);

    bool IsWinterGFX() const { return isWinterGFX_; }

    std::vector<std::unique_ptr<MusicItem>> sng_lst;

    /// Figure animations have 8 frames
    using AnimationSprites = std::array<glSmartBitmap, 8>;
    struct FightSprites
    {
        /// Attack animation
        AnimationSprites attacking;
        // 3 defend animations
        std::array<AnimationSprites, 3> defending;
        /// Sprite for the hit
        glSmartBitmap hit;
    };

    /// Animals: Species, Direction, AnimationFrame(Last = Dead)
    using AnimalAnimationSprites =
      helpers::EnumArray<std::array<glSmartBitmap, ANIMAL_MAX_ANIMATION_STEPS + 1>, Direction>;
    helpers::EnumArray<AnimalAnimationSprites, Species> animal_cache;
    glSmartBitmap& getAnimalSprite(Species species, Direction dir, unsigned aniFrame)
    {
        return animal_cache[species][dir][aniFrame];
    }
    glSmartBitmap& getDeadAnimalSprite(Species species)
    {
        return animal_cache[species][Direction::WEST][ANIMAL_MAX_ANIMATION_STEPS];
    }

    struct BuildingSprites
    {
        glSmartBitmap building, skeleton, door;
    };
    /// Buildings: Nation, Type, Building/Skeleton
    helpers::MultiEnumArray<BuildingSprites, Nation, BuildingType> building_cache;
    /// Flags: Nation, Type, AnimationFrame
    helpers::MultiEnumArray<AnimationSprites, Nation, FlagType> flag_cache;
    /// Military Flags: AnimationFrame
    // AnimationSprites building_flag_cache;
    /// Trees: Type, AnimationFrame
    helpers::MultiArray<glSmartBitmap, 9, 15> tree_cache;
    /// Jobs: Nation, Job, Direction, AnimationFrame
    helpers::MultiEnumArray<AnimationSprites, Nation, Job, Direction> bob_jobs_cache;
    helpers::MultiEnumArray<AnimationSprites, Nation, Direction> fat_carrier_cache;
    glSmartBitmap& getBobSprite(Nation nat, Job job, Direction dir, unsigned aniFrame)
    {
        return bob_jobs_cache[nat][job][dir][aniFrame];
    }
    glSmartBitmap& getCarrierBobSprite(Nation nat, bool fat, Direction dir, unsigned aniFrame)
    {
        return fat ? fat_carrier_cache[nat][dir][aniFrame] : bob_jobs_cache[nat][Job::Helper][dir][aniFrame];
    }
    /// Stone: Type, Size
    helpers::EnumArray<std::array<glSmartBitmap, 6>, GraniteType> granite_cache;
    /// Grainfield: Type, Size
    helpers::MultiArray<glSmartBitmap, 2, 4> grainfield_cache;
    /// Carrier w/ ware: NormalOrFat, Ware, Direction
    std::array<helpers::MultiEnumArray<AnimationSprites, GoodType, Direction>, 2> carrier_cache;
    glSmartBitmap& getCarrierSprite(GoodType ware, bool fat, Direction dir, unsigned aniFrame)
    {
        return carrier_cache[fat][ware][dir][aniFrame];
    }
    /// Boundary stones: Nation
    helpers::EnumArray<glSmartBitmap, Nation> boundary_stone_cache;
    /// BoatCarrier: Direction, AnimationFrame
    std::array<AnimationSprites, 6> boat_cache;
    glSmartBitmap& getBoatCarrierSprite(Direction dir, unsigned aniFrame)
    {
        return boat_cache[rttr::enum_cast(dir)][aniFrame];
    }
    /// Donkey: Direction, AnimationFrame
    std::array<AnimationSprites, 6> donkey_cache;
    glSmartBitmap& getDonkeySprite(Direction dir, unsigned aniFrame)
    {
        return donkey_cache[rttr::enum_cast(dir)][aniFrame];
    }
    /// Gateway: AnimationFrame
    std::array<glSmartBitmap, 5> gateway_cache;
    /// Fight animations for each nation, soldier type and left/right
    helpers::EnumArray<helpers::MultiArray<FightSprites, NUM_SOLDIER_RANKS, 2>, Nation> fight_cache;

private:
    /// Load all sounds
    bool LoadSounds();

    template<typename T>
    bool LoadImpl(const T& resIdOrPath, const libsiedler2::ArchivItem_Palette* palette);

    Log& logger_;
    const RttrConfig& config_;
    std::unique_ptr<ArchiveLocator> archiveLocator_;
    std::unique_ptr<ArchiveLoader> archiveLoader_;
    std::map<ResourceId, FileEntry> files_;
    std::vector<glFont> fonts;

    bool isWinterGFX_;
    helpers::EnumArray<libsiedler2::Archiv*, Nation> nation_gfx;
    helpers::EnumArray<libsiedler2::Archiv*, Nation> nationIcons_;
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
