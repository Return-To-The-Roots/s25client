// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Rect.h"
#include "WineLoader.h"
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
    bool loadFilesAtStart();
    bool loadFonts();
    /// Load files required during a game
    bool loadFilesAtGame(const std::string& mapGfxPath, bool isWinterGFX, const std::vector<Nation>& nations,
                         const std::vector<AddonId>& enabledAddons);
    /// Load all given files with the default palette
    bool loadFiles(const std::vector<std::string>& files);
    bool loadResources(const std::vector<ResourceId>& resources);

    /// Creates archives with empty files for the GUI (for testing purposes)
    void loadDummyGUIFiles();
    void loadDummyMapFiles();
    void loadDummySoundFiles();
    /// Load a file and save it into the loader repo
    bool Load(const boost::filesystem::path& path, const libsiedler2::ArchivItem_Palette* palette = nullptr);
    bool Load(const ResourceId& resId, const libsiedler2::ArchivItem_Palette* palette = nullptr);
    /// Load a file or directory and its overrides into the archive
    bool Load(libsiedler2::Archiv& archive, const boost::filesystem::path& path,
              const libsiedler2::ArchivItem_Palette* palette = nullptr);
    bool Load(libsiedler2::Archiv& archive, const ResourceId& resId,
              const libsiedler2::ArchivItem_Palette* palette = nullptr);

    void fillCaches();
    static std::unique_ptr<glArchivItem_Bitmap> extractTexture(const glArchivItem_Bitmap& srcImg, const Rect& rect);
    static std::unique_ptr<libsiedler2::Archiv> extractAnimatedTexture(const glArchivItem_Bitmap& srcImg,
                                                                       const Rect& rect, uint8_t start_index,
                                                                       uint8_t color_count);

    glArchivItem_Bitmap* getImageN(const ResourceId& file, unsigned nr);
    /// Same as GetImageN but returns a ITexture. Note glArchivItem_Bitmap is a ITexture
    ITexture* getTextureN(const ResourceId& file, unsigned nr);
    glArchivItem_Bitmap* getImage(const ResourceId& file, const std::string& name);
    glArchivItem_Bitmap_Player* getPlayerImage(const ResourceId& file, unsigned nr);
    glFont* getFont(FontSize);
    libsiedler2::ArchivItem_Palette* getPaletteN(const ResourceId& file, unsigned nr = 0);
    SoundEffectItem* getSoundN(const ResourceId& file, unsigned nr);
    std::string getTextN(const ResourceId& file, unsigned nr);
    libsiedler2::Archiv& getArchive(const ResourceId& file);
    glArchivItem_Bob* getBob(const ResourceId& file);
    glArchivItem_BitmapBase* getNationImageN(Nation nation, unsigned nr);
    glArchivItem_Bitmap* getNationImage(Nation nation, unsigned nr);
    glArchivItem_Bitmap* getNationIcon(Nation nation, BuildingType bld);
    ITexture* getBuildingTex(Nation nation, BuildingType bld);
    glArchivItem_Bitmap_Player* getNationPlayerImage(Nation nation, unsigned nr);
    /// Return the map texture with the given number
    ITexture* getMapTexture(unsigned nr);
    /// Return the more specialized map image. Note: Prefer GetMapTexture which also handles (pseudo) player bitmaps
    glArchivItem_Bitmap* getMapImage(unsigned nr);
    /// Get the ware symbol texture
    ITexture* getWareTex(GoodType ware);
    /// Get the ware stack texture (lying on ground)
    ITexture* getWareStackTex(GoodType ware);
    /// Get the ware texture when carried by donky
    ITexture* getWareDonkeyTex(GoodType ware);
    /// Get job symbol texture
    ITexture* getJobTex(Job job);
    glArchivItem_Bitmap_Player* getMapPlayerImage(unsigned nr);

    bool isWinterGFX() const { return isWinterGFX_; }

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
        return animal_cache[species][Direction::West][ANIMAL_MAX_ANIMATION_STEPS];
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
    bool loadSounds();

    template<typename T>
    bool loadImpl(const T& resIdOrPath, const libsiedler2::ArchivItem_Palette* palette);

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
#define SmallFont (LOADER.getFont(FontSize::Small))
#define NormalFont (LOADER.getFont(FontSize::Normal))
#define LargeFont (LOADER.getFont(FontSize::Large))
