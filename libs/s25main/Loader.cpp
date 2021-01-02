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

///////////////////////////////////////////////////////////////////////////////

#include "Loader.h"
#include "ListDir.h"
#include "RttrConfig.h"
#include "Settings.h"
#include "Timer.h"
#include "addons/const_addons.h"
#include "commonDefines.h"
#include "convertSounds.h"
#include "files.h"
#include "helpers/EnumRange.h"
#include "helpers/containerUtils.h"
#include "ogl/MusicItem.h"
#include "ogl/SoundEffectItem.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "ogl/glArchivItem_Bitmap_RLE.h"
#include "ogl/glArchivItem_Bitmap_Raw.h"
#include "ogl/glArchivItem_Bob.h"
#include "ogl/glFont.h"
#include "ogl/glSmartBitmap.h"
#include "ogl/glTexturePacker.h"
#include "resources/ArchiveLoader.h"
#include "resources/ArchiveLocator.h"
#include "resources/ResolvedFile.h"
#include "gameTypes/Direction.h"
#include "gameTypes/DirectionToImgDir.h"
#include "gameData/JobConsts.h"
#include "gameData/MilitaryConsts.h"
#include "gameData/NationConsts.h"
#include "libsiedler2/ArchivItem_Font.h"
#include "libsiedler2/ArchivItem_Palette.h"
#include "libsiedler2/ArchivItem_PaletteAnimation.h"
#include "libsiedler2/ArchivItem_Text.h"
#include "libsiedler2/ErrorCodes.h"
#include "libsiedler2/PixelBufferBGRA.h"
#include "libsiedler2/PixelBufferPaletted.h"
#include "libsiedler2/libsiedler2.h"
#include "s25util/Log.h"
#include "s25util/StringConversion.h"
#include "s25util/System.h"
#include "s25util/dynamicUniqueCast.h"
#include "s25util/strAlgos.h"
#include <boost/filesystem.hpp>
#include <boost/range/adaptor/map.hpp>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <iomanip>
#include <memory>
#include <sstream>
#include <stdexcept>

struct Loader::FileEntry
{
    libsiedler2::Archiv archive;
    /// List of files used to build this archive
    ResolvedFile resolvedFile;
};

template<typename T>
static T convertChecked(libsiedler2::ArchivItem* item)
{
    T res = dynamic_cast<T>(item);
    RTTR_Assert(!item || res);
    return res;
}

Loader::Loader(Log& logger, const RttrConfig& config)
    : logger_(logger), config_(config), archiveLocator_(std::make_unique<ArchiveLocator>(logger)),
      archiveLoader_(std::make_unique<ArchiveLoader>(logger)), isWinterGFX_(false), nation_gfx(), nationIcons_(),
      map_gfx(nullptr), stp(nullptr)
{}

Loader::~Loader() = default;

void Loader::initResourceFolders(const std::vector<Nation>& usedNations, const std::vector<AddonId>& enabledAddons)
{
    addDefaultResourceFolders(config_, *archiveLocator_, usedNations, enabledAddons);
}

glArchivItem_Bitmap* Loader::GetImageN(const ResourceId& file, unsigned nr)
{
    return convertChecked<glArchivItem_Bitmap*>(files_[file].archive[nr]);
}

ITexture* Loader::GetTextureN(const ResourceId& file, unsigned nr)
{
    return convertChecked<ITexture*>(files_[file].archive[nr]);
}

glArchivItem_Bitmap* Loader::GetImage(const ResourceId& file, const std::string& name)
{
    return convertChecked<glArchivItem_Bitmap*>(files_[file].archive.find(name));
}

glArchivItem_Bitmap_Player* Loader::GetPlayerImage(const ResourceId& file, unsigned nr)
{
    return convertChecked<glArchivItem_Bitmap_Player*>(files_[file].archive[nr]);
}

glFont* Loader::GetFont(FontSize size)
{
    return fonts.empty() ? nullptr : &fonts[static_cast<unsigned>(size)];
}

libsiedler2::ArchivItem_Palette* Loader::GetPaletteN(const ResourceId& file, unsigned nr)
{
    return dynamic_cast<libsiedler2::ArchivItem_Palette*>(files_[file].archive[nr]);
}

SoundEffectItem* Loader::GetSoundN(const ResourceId& file, unsigned nr)
{
    return dynamic_cast<SoundEffectItem*>(files_[file].archive[nr]);
}

std::string Loader::GetTextN(const ResourceId& file, unsigned nr)
{
    auto* archive = dynamic_cast<libsiedler2::ArchivItem_Text*>(files_[file].archive[nr]);
    return archive ? archive->getText() : "text missing";
}

libsiedler2::Archiv& Loader::GetArchive(const ResourceId& file)
{
    RTTR_Assert(helpers::contains(files_, file));
    return files_[file].archive;
}

glArchivItem_Bob* Loader::GetBob(const ResourceId& file)
{
    return dynamic_cast<glArchivItem_Bob*>(files_[file].archive.get(0));
}

glArchivItem_BitmapBase* Loader::GetNationImageN(unsigned nation, unsigned nr)
{
    return dynamic_cast<glArchivItem_BitmapBase*>(nation_gfx[nation]->get(nr));
}

glArchivItem_Bitmap* Loader::GetNationImage(unsigned nation, unsigned nr)
{
    return checkedCast<glArchivItem_Bitmap*>(GetNationImageN(nation, nr));
}

glArchivItem_Bitmap* Loader::GetNationIcon(unsigned nation, BuildingType bld)
{
    return convertChecked<glArchivItem_Bitmap*>(nationIcons_[nation]->get(rttr::enum_cast(bld)));
}

ITexture* Loader::GetNationTex(unsigned nation, unsigned nr)
{
    return checkedCast<ITexture*>(GetNationImage(nation, nr));
}

glArchivItem_Bitmap_Player* Loader::GetNationPlayerImage(unsigned nation, unsigned nr)
{
    return checkedCast<glArchivItem_Bitmap_Player*>(GetNationImageN(nation, nr));
}

glArchivItem_Bitmap* Loader::GetMapImageN(unsigned nr)
{
    return convertChecked<glArchivItem_Bitmap*>(map_gfx->get(nr));
}

ITexture* Loader::GetMapTexN(unsigned nr)
{
    return convertChecked<ITexture*>(map_gfx->get(nr));
}

glArchivItem_Bitmap_Player* Loader::GetMapPlayerImage(unsigned nr)
{
    return convertChecked<glArchivItem_Bitmap_Player*>(map_gfx->get(nr));
}

/**
 *  Load general files required also outside of games
 *
 *  @return @p true on success, @p false on error.
 */
bool Loader::LoadFilesAtStart()
{
    namespace res = s25::resources;
    // Palettes
    if(!LoadFiles({res::pal5, res::pal6, res::pal7, res::paletti0, res::paletti1, res::paletti8})
       || !Load(ResourceId("colors")))
        return false;

    if(!LoadFonts())
        return false;

    std::vector<std::string> files = {res::resource,
                                      res::io,                       // Menu graphics
                                      res::setup013, res::setup015}; // Backgrounds for options and free play

    const std::array<bfs::path, 2> loadScreenFolders{config_.ExpandPath(s25::folders::loadScreens),
                                                     config_.ExpandPath(s25::folders::loadScreensMissions)};
    for(const std::string& loadScreenId : LOAD_SCREENS)
    {
        const std::string filename = s25util::toUpper(loadScreenId) + ".LBM";
        if(exists(loadScreenFolders[0] / filename))
            files.push_back((loadScreenFolders[0] / filename).string());
        else
            files.push_back((loadScreenFolders[1] / filename).string());
    }

    if(!LoadFiles(files))
        return false;

    if(!LoadSounds())
        return false;

    return LoadResources({"io_new", "client", "languages", "logo", "menu", "rttr"});
}

bool Loader::LoadSounds()
{
    if(!Load(config_.ExpandPath(s25::files::soundOrig)))
        return false;
    const Timer timer(true);
    logger_.write(_("Starting sound conversion: "));
    try
    {
        convertSounds(GetArchive("sound"), config_.ExpandPath(s25::files::soundScript));
    } catch(const std::runtime_error& e)
    {
        logger_.write(_("failed: %1%\n")) % e.what();
        return false;
    }
    using namespace std::chrono;
    logger_.write(_("done in %ums\n")) % duration_cast<milliseconds>(timer.getElapsed()).count();

    const bfs::path oggPath = config_.ExpandPath(s25::folders::sng);
    std::vector<bfs::path> oggFiles = ListDir(oggPath, "ogg");

    sng_lst.reserve(oggFiles.size());
    for(const auto& oggFile : oggFiles)
    {
        try
        {
            libsiedler2::Archiv sng = archiveLoader_->loadFileOrDir(oggFile);
            auto music = libutil::dynamicUniqueCast<MusicItem>(sng.release(0));
            if(music)
                sng_lst.emplace_back(std::move(music));
            else
                logger_.write(_("WARNING: Found invalid music item for %1%\n")) % oggFile;
        } catch(const LoadError&)
        {
            return false;
        }
    }

    if(sng_lst.empty())
    {
        logger_.write(_("WARNING: Did not find the music files.\n\tYou have to run the updater once or copy the .ogg "
                        "files manually to \"%1%\" or you "
                        "won't be able to hear the music.\n"))
          % oggPath;
    }

    return true;
}

bool Loader::LoadFonts()
{
    if(!Load(ResourceId("fonts"), GetPaletteN("pal5")))
        return false;
    fonts.clear();
    const auto& loadedFonts = GetArchive("fonts");
    for(unsigned i = 0; i <= helpers::MaxEnumValue_v<FontSize>; i++)
    {
        const auto* curFont = dynamic_cast<const libsiedler2::ArchivItem_Font*>(loadedFonts[i]);
        if(!curFont)
        {
            logger_.write(_("Unable to load font at index %1%\n")) % i;
            return false;
        }
        fonts.emplace_back(*curFont);
    }
    return true;
}

void Loader::LoadDummyGUIFiles()
{
    // Palettes
    {
        auto palette = std::make_unique<libsiedler2::ArchivItem_Palette>();
        for(int i = 0; i < 256; i++)
            palette->set(i, libsiedler2::ColorRGB(42, 137, i));
        files_["pal5"].archive.pushC(*palette);
        // Player color palette
        for(int i = 128; i < 128 + libsiedler2::ArchivItem_Bitmap_Player::numPlayerClrs; i++)
            palette->set(i, libsiedler2::ColorRGB(i, i, i));
        files_["colors"].archive.push(std::move(palette));
    }
    // GUI elements
    libsiedler2::Archiv& resource = files_["resource"].archive;
    resource.alloc(57);
    for(unsigned id = 4; id < 36; id++)
    {
        auto bmp = std::make_unique<glArchivItem_Bitmap_RLE>();
        libsiedler2::PixelBufferBGRA buffer(1, 1);
        bmp->create(buffer);
        resource.set(id, std::move(bmp));
    }
    for(unsigned id = 36; id < 57; id++)
    {
        auto bmp = std::make_unique<glArchivItem_Bitmap_Raw>();
        libsiedler2::PixelBufferBGRA buffer(1, 1);
        bmp->create(buffer);
        resource.set(id, std::move(bmp));
    }
    libsiedler2::Archiv& io = files_["io"].archive;
    for(unsigned id = 0; id < 264; id++)
    {
        auto bmp = std::make_unique<glArchivItem_Bitmap_Raw>();
        libsiedler2::PixelBufferBGRA buffer(1, 1);
        bmp->create(buffer);
        io.push(std::move(bmp));
    }
    // Fonts
    auto* palette = GetPaletteN("colors");
    libsiedler2::PixelBufferBGRA buffer(15, 16);
    for(unsigned i = 0; i <= helpers::MaxEnumValue_v<FontSize>; i++)
    {
        auto font = std::make_unique<libsiedler2::ArchivItem_Font>();
        const unsigned dx = 9 + i * (helpers::MaxEnumValue_v<FontSize> + 1);
        const unsigned dy = 10 + i * (helpers::MaxEnumValue_v<FontSize> + 1);
        font->setDx(dx);
        font->setDy(dy);
        font->alloc(255);
        for(unsigned id = 0x20; id < 255; id++)
        {
            auto bmp = std::make_unique<glArchivItem_Bitmap_Player>();
            bmp->create(dx, dy, buffer, palette, 0);
            font->set(id, std::move(bmp));
        }
        fonts.emplace_back(*font);
    }
}

/**
 *  Load files required during a game
 *
 *  @param[in] mapGfxPath Path to map gfx files
 *  @param[in] isWinterGFX True iff winter nation files should be loaded
 *  @param[in] nations True entry for each nation to load
 *
 *  @return @p true on success
 */
bool Loader::LoadFilesAtGame(const std::string& mapGfxPath, bool isWinterGFX, const std::vector<Nation>& nations,
                             const std::vector<AddonId>& enabledAddons)
{
    initResourceFolders(nations, enabledAddons);

    namespace res = s25::resources;
    std::vector<std::string> files = {res::rom_bobs, res::carrier,  res::jobs,     res::boat,
                                      res::boot_z,   res::mis0bobs, res::mis1bobs, res::mis2bobs,
                                      res::mis3bobs, res::mis4bobs, res::mis5bobs};

    const libsiedler2::ArchivItem_Palette* pal5 = GetPaletteN("pal5");

    if(!LoadFiles(files) || !Load(ResourceId("map_new"), pal5))
        return false;

    // Load nation building and icon graphics
    nation_gfx = nationIcons_ = {};
    const std::string natPrefix = isWinterGFX ? "W" : "";
    for(Nation nation : nations)
    {
        const bfs::path nationFolder = (nation < NUM_NATIVE_NATIONS) ?
                                         config_.ExpandPath(s25::folders::mbob) :
                                         config_.ExpandPath(s25::folders::assetsNations) / NationNames[nation];
        const auto shortName = s25util::toUpper(std::string(NationNames[nation], 0, 3));
        const bfs::path buildingsFilePath = nationFolder / (natPrefix + shortName + "_Z.LST");
        const bfs::path iconsFilePath = nationFolder / (shortName + "_ICON.LST");
        if(!Load(buildingsFilePath, pal5) || !Load(iconsFilePath, pal5))
            return false;
        nation_gfx[nation] = &files_[ResourceId::make(buildingsFilePath)].archive;
        nationIcons_[nation] = &files_[ResourceId::make(iconsFilePath)].archive;
    }

    // TODO: Move to addon folder and make it overwrite existing file
    if(!LoadResources({"charburner", "charburner_bobs"}))
        return false;

    const bfs::path mapGFXFile = config_.ExpandPath(mapGfxPath);
    if(!Load(mapGFXFile, pal5))
        return false;
    map_gfx = &GetArchive(ResourceId::make(mapGFXFile));

    isWinterGFX_ = isWinterGFX;

    return true;
}

bool Loader::LoadFiles(const std::vector<std::string>& files)
{
    const libsiedler2::ArchivItem_Palette* pal5 = GetPaletteN("pal5");
    // load the files
    for(const std::string& curFile : files)
    {
        const bfs::path filePath = config_.ExpandPath(curFile);
        if(!Load(filePath, pal5))
        {
            logger_.write(_("Failed to load %s\n")) % filePath;
            return false;
        }
    }

    return true;
}

bool Loader::LoadResources(const std::vector<ResourceId>& resources)
{
    const libsiedler2::ArchivItem_Palette* pal5 = GetPaletteN("pal5");
    for(const ResourceId& curResource : resources)
    {
        if(!Load(curResource, pal5))
        {
            logger_.write(_("Failed to load %s\n")) % curResource;
            return false;
        }
    }

    return true;
}

void Loader::fillCaches()
{
    stp = std::make_unique<glTexturePacker>();

    // Animals
    for(const auto species : helpers::EnumRange<Species>{})
    {
        for(const auto dir : helpers::EnumRange<Direction>{})
        {
            for(unsigned ani_step = 0; ani_step < ANIMALCONSTS[species].animation_steps; ++ani_step)
            {
                glSmartBitmap& bmp = getAnimalSprite(Species(species), dir, ani_step);

                bmp.reset();

                bmp.add(GetMapImageN(ANIMALCONSTS[species].walking_id
                                     + ANIMALCONSTS[species].animation_steps * rttr::enum_cast(dir + 3u) + ani_step));

                if(ANIMALCONSTS[species].shadow_id)
                {
                    if(species == Species::Duck)
                        // Ente Sonderfall, da gibts nur einen Schatten für jede Richtung!
                        bmp.addShadow(GetMapImageN(ANIMALCONSTS[species].shadow_id));
                    else
                        // ansonsten immer pro Richtung einen Schatten
                        bmp.addShadow(GetMapImageN(ANIMALCONSTS[species].shadow_id + rttr::enum_cast(dir + 3u)));
                }

                stp->add(bmp);
            }
        }

        glSmartBitmap& bmp = getDeadAnimalSprite(species);

        bmp.reset();

        if(ANIMALCONSTS[species].dead_id)
        {
            bmp.add(GetMapImageN(ANIMALCONSTS[species].dead_id));

            if(ANIMALCONSTS[species].shadow_dead_id)
            {
                bmp.addShadow(GetMapImageN(ANIMALCONSTS[species].shadow_dead_id));
            }

            stp->add(bmp);
        }
    }

    glArchivItem_Bob* bob_jobs = GetBob("jobs");
    if(!bob_jobs)
        throw std::runtime_error("jobs not found");

    for(const auto nation : helpers::enumRange<Nation>())
    {
        if(!nation_gfx[nation])
            continue;
        // BUILDINGS
        for(const auto type : helpers::enumRange<BuildingType>())
        {
            BuildingSprites& sprites = building_cache[nation][type];

            sprites.building.reset();
            sprites.skeleton.reset();
            sprites.door.reset();

            if(type == BuildingType::Charburner)
            {
                unsigned id = nation * 8;

                sprites.building.add(GetImageN("charburner", id + (isWinterGFX_ ? 6 : 1)));
                sprites.building.addShadow(GetImageN("charburner", id + 2));

                sprites.skeleton.add(GetImageN("charburner", id + 3));
                sprites.skeleton.addShadow(GetImageN("charburner", id + 4));

                sprites.door.add(GetImageN("charburner", id + (isWinterGFX_ ? 7 : 5)));
            } else
            {
                sprites.building.add(GetNationImage(nation, 250 + 5 * rttr::enum_cast(type)));
                sprites.building.addShadow(GetNationImage(nation, 250 + 5 * rttr::enum_cast(type) + 1));
                if(type == BuildingType::Headquarters)
                {
                    // HQ has no skeleton, but we have a tent that can act as an HQ
                    sprites.skeleton.add(GetImageN("mis0bobs", 6));
                    sprites.skeleton.addShadow(GetImageN("mis0bobs", 7));
                } else
                {
                    sprites.skeleton.add(GetNationImage(nation, 250 + 5 * rttr::enum_cast(type) + 2));
                    sprites.skeleton.addShadow(GetNationImage(nation, 250 + 5 * rttr::enum_cast(type) + 3));
                }
                sprites.door.add(GetNationImage(nation, 250 + 5 * rttr::enum_cast(type) + 4));
            }

            stp->add(sprites.building);
            stp->add(sprites.skeleton);
            stp->add(sprites.door);
        }

        // FLAGS
        for(unsigned type = 0; type < 3; ++type)
        {
            for(unsigned ani_step = 0; ani_step < 8; ++ani_step)
            {
                // Flaggentyp berücksichtigen
                int nr = ani_step + 100 + 20 * type;

                glSmartBitmap& bmp = flag_cache[nation][type][ani_step];

                bmp.reset();

                bmp.add(GetNationPlayerImage(nation, nr));
                bmp.addShadow(GetNationImage(nation, nr + 10));

                stp->add(bmp);
            }
        }

        // Bobs from jobs.bob.
        for(const auto job : helpers::enumRange<Job>())
        {
            for(Direction dir : helpers::EnumRange<Direction>{})
            {
                for(unsigned ani_step = 0; ani_step < 8; ++ani_step)
                {
                    glSmartBitmap& bmp = bob_jobs_cache(nation, job, rttr::enum_cast(dir))[ani_step];
                    bmp.reset();

                    const auto& spriteData = JOB_SPRITE_CONSTS[Job(job)];
                    const libsiedler2::ImgDir imgDir = toImgDir(dir);

                    bmp.add(dynamic_cast<glArchivItem_Bitmap_Player*>(
                      bob_jobs->getBody(spriteData.isFat(), imgDir, ani_step)));
                    bmp.add(dynamic_cast<glArchivItem_Bitmap_Player*>(
                      bob_jobs->getOverlay(spriteData.getBobId(Nation(nation)), spriteData.isFat(), imgDir, ani_step)));
                    bmp.addShadow(GetMapImageN(900 + static_cast<unsigned>(imgDir) * 8 + ani_step));

                    stp->add(bmp);
                }
            }
        }
        // Fat carrier, so that we do not need an additional sub-array
        for(Direction dir : helpers::EnumRange<Direction>{})
        {
            for(unsigned ani_step = 0; ani_step < 8; ++ani_step)
            {
                glSmartBitmap& bmp =
                  bob_jobs_cache(nation, helpers::NumEnumValues_v<Job>, rttr::enum_cast(dir))[ani_step];
                bmp.reset();

                const libsiedler2::ImgDir imgDir = toImgDir(dir);

                bmp.add(dynamic_cast<glArchivItem_Bitmap_Player*>(bob_jobs->getBody(true, imgDir, ani_step)));
                bmp.add(dynamic_cast<glArchivItem_Bitmap_Player*>(bob_jobs->getOverlay(0, true, imgDir, ani_step)));
                bmp.addShadow(GetMapImageN(900 + static_cast<unsigned>(imgDir) * 8 + ani_step));

                stp->add(bmp);
            }
        }

        {
            glSmartBitmap& bmp = boundary_stone_cache[nation];
            bmp.reset();

            bmp.add(GetNationPlayerImage(nation, 0));
            bmp.addShadow(GetNationImage(nation, 1));

            stp->add(bmp);
        }

        libsiedler2::Archiv& romBobs = GetArchive("rom_bobs");
        const auto getAlternative = [&romBobs](unsigned id, unsigned altId) {
            auto* bmp = convertChecked<glArchivItem_Bitmap_Player*>(romBobs[id]);
            return bmp ? bmp : convertChecked<glArchivItem_Bitmap_Player*>(romBobs[altId]);
        };
        // Special handling for non-native nations: Use roman animations if own are missing
        const unsigned fallbackNation = nation < NUM_NATIVE_NATIONS ? nation : static_cast<unsigned>(NAT_ROMANS);
        const auto& natFightAnimIds = FIGHT_ANIMATIONS[nation];
        const auto& altNatFightAnimIds = FIGHT_ANIMATIONS[fallbackNation];
        const auto& natHitIds = HIT_SOLDIERS[nation];
        const auto& altNatHitIds = HIT_SOLDIERS[fallbackNation];
        for(unsigned rank = 0; rank < NUM_SOLDIER_RANKS; ++rank)
        {
            for(unsigned dir = 0; dir < 2; ++dir)
            {
                FightSprites& sprites = fight_cache[nation][rank][dir];
                const auto& fightAnimIds = natFightAnimIds[rank][dir];
                const auto& altFightAnimIds = altNatFightAnimIds[rank][dir];
                for(unsigned ani_step = 0; ani_step < 8; ++ani_step)
                {
                    glSmartBitmap& bmp = sprites.attacking[ani_step];
                    bmp.reset();
                    bmp.add(getAlternative(fightAnimIds.attacking[ani_step], altFightAnimIds.attacking[ani_step]));
                    stp->add(bmp);
                }

                for(unsigned i = 0; i < sprites.defending.size(); i++)
                {
                    for(unsigned ani_step = 0; ani_step < 8; ++ani_step)
                    {
                        glSmartBitmap& bmp = sprites.defending[i][ani_step];
                        bmp.reset();
                        bmp.add(
                          getAlternative(fightAnimIds.defending[i][ani_step], altFightAnimIds.defending[i][ani_step]));
                        stp->add(bmp);
                    }
                }
                glSmartBitmap& bmp = sprites.hit;
                bmp.reset();
                // Hit sprites for left/right are consecutive
                bmp.add(getAlternative(natHitIds[rank] + dir, altNatHitIds[rank] + dir));
                stp->add(bmp);
            }
        }
    }

    // BUILDING FLAG ANIMATION (for military buildings)
    /*
        for (unsigned ani_step = 0; ani_step < 8; ++ani_step)
        {
            glSmartBitmap &bmp = building_flag_cache[ani_step];

            bmp.reset();

            bmp.add(static_cast<glArchivItem_Bitmap_Player *>(GetMapImageN(3162+ani_step)));

            int a, b, c, d;
            static_cast<glArchivItem_Bitmap_Player *>(GetMapImageN(3162+ani_step))->getVisibleArea(a, b, c, d);
            fprintf(stderr, "%i,%i (%ix%i)\n", a, b, c, d);


            stp->add(bmp);
        }
    */
    // Trees
    for(unsigned type = 0; type < 9; ++type)
    {
        for(unsigned ani_step = 0; ani_step < 15; ++ani_step)
        {
            glSmartBitmap& bmp = tree_cache[type][ani_step];

            bmp.reset();

            bmp.add(GetMapImageN(200 + type * 15 + ani_step));
            bmp.addShadow(GetMapImageN(350 + type * 15 + ani_step));

            stp->add(bmp);
        }
    }

    // Granite
    for(unsigned type = 0; type < 2; ++type)
    {
        for(unsigned size = 0; size < 6; ++size)
        {
            glSmartBitmap& bmp = granite_cache[type][size];

            bmp.reset();

            bmp.add(GetMapImageN(516 + type * 6 + size));
            bmp.addShadow(GetMapImageN(616 + type * 6 + size));

            stp->add(bmp);
        }
    }

    // Grainfields
    for(unsigned type = 0; type < 2; ++type)
    {
        for(unsigned size = 0; size < 4; ++size)
        {
            glSmartBitmap& bmp = grainfield_cache[type][size];

            bmp.reset();

            bmp.add(GetMapImageN(532 + type * 5 + size));
            bmp.addShadow(GetMapImageN(632 + type * 5 + size));

            stp->add(bmp);
        }
    }

    // Donkeys
    for(const auto dir : helpers::EnumRange<Direction>{})
    {
        for(unsigned ani_step = 0; ani_step < 8; ++ani_step)
        {
            glSmartBitmap& bmp = getDonkeySprite(dir, ani_step);

            bmp.reset();

            bmp.add(GetMapImageN(2000 + rttr::enum_cast(dir + 3u) * 8 + ani_step));
            bmp.addShadow(GetMapImageN(2048 + rttr::enum_cast(dir) % 3));

            stp->add(bmp);
        }
    }

    // Boats
    for(const auto dir : helpers::EnumRange<Direction>{})
    {
        for(unsigned ani_step = 0; ani_step < 8; ++ani_step)
        {
            glSmartBitmap& bmp = getBoatCarrierSprite(dir, ani_step);

            bmp.reset();

            bmp.add(GetPlayerImage("boat", rttr::enum_cast(dir + 3u) * 8 + ani_step));
            bmp.addShadow(GetMapImageN(2048 + rttr::enum_cast(dir) % 3));

            stp->add(bmp);
        }
    }

    // carrier_cache[ware][direction][animation_step][fat]
    glArchivItem_Bob* bob_carrier = GetBob("carrier");
    if(!bob_carrier)
        throw std::runtime_error("carrier not found");

    for(const auto ware : helpers::EnumRange<GoodType>{})
    {
        for(bool fat : {true, false})
        {
            for(Direction dir : helpers::EnumRange<Direction>{})
            {
                for(unsigned ani_step = 0; ani_step < 8; ++ani_step)
                {
                    glSmartBitmap& bmp = getCarrierSprite(ware, fat, dir, ani_step);
                    bmp.reset();

                    // Japanese shield is missing
                    const unsigned id =
                      rttr::enum_cast((ware == GoodType::ShieldJapanese) ? GoodType::ShieldRomans : ware);

                    const libsiedler2::ImgDir imgDir = toImgDir(dir);

                    bmp.add(dynamic_cast<glArchivItem_Bitmap_Player*>(bob_carrier->getBody(fat, imgDir, ani_step)));
                    bmp.add(
                      dynamic_cast<glArchivItem_Bitmap_Player*>(bob_carrier->getOverlay(id, fat, imgDir, ani_step)));
                    bmp.addShadow(GetMapImageN(900 + static_cast<unsigned>(imgDir) * 8 + ani_step));

                    stp->add(bmp);
                }
            }
        }
    }

    // gateway animation :)
    {
        const unsigned char start_index = 248;
        const unsigned char color_count = 4;

        libsiedler2::ArchivItem_Palette* palette = GetPaletteN("pal5");
        glArchivItem_Bitmap* image = GetMapImageN(561);
        glArchivItem_Bitmap* shadow = GetMapImageN(661);

        if((image) && (shadow) && (palette))
        {
            unsigned short width = image->getWidth();
            unsigned short height = image->getHeight();

            std::vector<unsigned char> buffer(width * height, 254);

            image->print(&buffer.front(), width, height, libsiedler2::TextureFormat::Paletted, palette, 0, 0, 0, 0,
                         width, height);

            for(unsigned char i = 0; i < color_count; ++i)
            {
                glSmartBitmap& bmp = gateway_cache[i + 1];

                bmp.reset();

                for(unsigned x = 0; x < width; ++x)
                {
                    for(unsigned y = 0; y < height; ++y)
                    {
                        if(buffer[y * width + x] >= start_index && buffer[y * width + x] < start_index + color_count)
                        {
                            if(++buffer[y * width + x] >= start_index + color_count)
                                buffer[y * width + x] = start_index;
                        }
                    }
                }

                auto bitmap = std::make_unique<glArchivItem_Bitmap_Raw>();
                bitmap->create(width, height, &buffer.front(), width, height, libsiedler2::TextureFormat::Paletted,
                               palette);
                bitmap->setNx(image->getNx());
                bitmap->setNy(image->getNy());

                bmp.add(bitmap.release(), true);
                bmp.addShadow(shadow);

                stp->add(bmp);
            }
        } else
        {
            for(unsigned char i = 0; i < color_count; ++i)
            {
                glSmartBitmap& bmp = gateway_cache[i + 1];

                bmp.reset();
            }
        }
    }

    if(SETTINGS.video.shared_textures)
    {
        // generate mega texture
        stp->pack();
    } else
        stp.reset();
}

/**
 *  Extrahiert eine Textur aus den Daten.
 */
std::unique_ptr<glArchivItem_Bitmap> Loader::ExtractTexture(const glArchivItem_Bitmap& srcImg, const Rect& rect)
{
    Extent texSize = rect.getSize();
    if(texSize.x == 0 && rect.right < srcImg.getWidth())
        texSize.x = srcImg.getWidth() - rect.right;
    if(texSize.y == 0 && rect.bottom < srcImg.getHeight())
        texSize.y = srcImg.getHeight() - rect.bottom;
    libsiedler2::PixelBufferPaletted buffer(texSize.x, texSize.y);

    if(int ec = srcImg.print(buffer, nullptr, 0, 0, rect.left, rect.top))
        throw std::runtime_error(std::string("Error loading texture: ") + libsiedler2::getErrorString(ec));

    std::unique_ptr<glArchivItem_Bitmap> bitmap = std::make_unique<glArchivItem_Bitmap_Raw>();
    if(int ec = bitmap->create(buffer, srcImg.getPalette()))
    {
        throw std::runtime_error(std::string("Error loading texture: ") + libsiedler2::getErrorString(ec));
    }
    return bitmap;
}

/**
 *  Extrahiert mehrere (animierte) Texturen aus den Daten.
 */
std::unique_ptr<libsiedler2::Archiv> Loader::ExtractAnimatedTexture(const glArchivItem_Bitmap& srcImg, const Rect& rect,
                                                                    uint8_t start_index, uint8_t color_count)
{
    Extent texSize = rect.getSize();
    if(texSize.x == 0 && rect.right < srcImg.getWidth())
        texSize.x = srcImg.getWidth() - rect.right;
    if(texSize.y == 0 && rect.bottom < srcImg.getHeight())
        texSize.y = srcImg.getHeight() - rect.bottom;
    libsiedler2::PixelBufferPaletted buffer(texSize.x, texSize.y);

    srcImg.print(buffer, nullptr, 0, 0, rect.left, rect.top);

    auto destination = std::make_unique<libsiedler2::Archiv>();
    libsiedler2::ArchivItem_PaletteAnimation anim;
    anim.isActive = true;
    anim.moveUp = false;
    anim.firstClr = start_index;
    anim.lastClr = start_index + color_count - 1u;
    const libsiedler2::ArchivItem_Palette* curPal = nullptr;
    for(unsigned i = 0; i < color_count; ++i)
    {
        auto newPal = (i == 0) ? clone(srcImg.getPalette()) : anim.apply(*curPal);
        auto bitmap = std::make_unique<glArchivItem_Bitmap_Raw>();
        bitmap->setPalette(std::move(newPal));
        if(int ec = bitmap->create(buffer))
            throw std::runtime_error("Error extracting animated texture: " + libsiedler2::getErrorString(ec));
        curPal = bitmap->getPalette();
        destination->push(std::move(bitmap));
    }
    return destination;
}

bool Loader::Load(libsiedler2::Archiv& archive, const bfs::path& path, const libsiedler2::ArchivItem_Palette* palette)
{
    try
    {
        archive = archiveLoader_->load(archiveLocator_->resolve(path), palette);
        return true;
    } catch(const LoadError&)
    {
        return false;
    }
}

bool Loader::Load(libsiedler2::Archiv& archive, const ResourceId& resId, const libsiedler2::ArchivItem_Palette* palette)
{
    const ResolvedFile resolvedFile = archiveLocator_->resolve(resId);
    if(!resolvedFile)
    {
        logger_.write(_("Failed to resolve resource %1%\n")) % resId;
        return false;
    }
    try
    {
        archive = archiveLoader_->load(resolvedFile, palette);
        return true;
    } catch(const LoadError&)
    {
        return false;
    }
}

template<typename T>
bool Loader::LoadImpl(const T& resIdOrPath, const libsiedler2::ArchivItem_Palette* palette)
{
    const auto resolvedFile = archiveLocator_->resolve(resIdOrPath);
    if(!resolvedFile)
    {
        logger_.write(_("Failed to resolve resource %1%\n")) % resIdOrPath;
        return false;
    }
    FileEntry& entry = files_[ResourceId::make(resIdOrPath)];
    // Do we really need to reload or can we reused the loaded version?
    if(entry.resolvedFile != resolvedFile)
    {
        try
        {
            entry.archive = archiveLoader_->load(resolvedFile, palette);
        } catch(const LoadError&)
        {
            return false;
        }
        // Update how we loaded this
        entry.resolvedFile = resolvedFile;
    }
    RTTR_Assert(!entry.archive.empty());
    return true;
}

bool Loader::Load(const bfs::path& path, const libsiedler2::ArchivItem_Palette* palette)
{
    return LoadImpl(path, palette);
}

bool Loader::Load(const ResourceId& resId, const libsiedler2::ArchivItem_Palette* palette)
{
    return LoadImpl(resId, palette);
}

void addDefaultResourceFolders(const RttrConfig& config, ArchiveLocator& locator,
                               const std::vector<Nation>& usedNations, const std::vector<AddonId>& enabledAddons)
{
    locator.clear();
    locator.addAssetFolder(config.ExpandPath(s25::folders::assetsBase));
    for(Nation nation : usedNations)
    {
        const auto overrideFolder = config.ExpandPath(s25::folders::assetsNations) / NationNames[nation];
        if(bfs::exists(overrideFolder))
            locator.addOverrideFolder(overrideFolder);
    }
    locator.addOverrideFolder(config.ExpandPath(s25::folders::assetsOverrides));
    for(AddonId addonId : enabledAddons)
    {
        const auto overrideFolder =
          config.ExpandPath(s25::folders::assetsAddons) / s25util::toStringClassic(static_cast<uint32_t>(addonId));
        if(bfs::exists(overrideFolder))
            locator.addOverrideFolder(overrideFolder);
    }
    const bfs::path userOverrides = config.ExpandPath(s25::folders::assetsUserOverrides);
    if(exists(userOverrides))
        locator.addOverrideFolder(userOverrides);
}

Loader& getGlobalLoader()
{
    static Loader loader{LOG, RTTRCONFIG};
    return loader;
}
