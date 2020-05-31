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

#include "commonDefines.h"
#include "Loader.h"
#include "ListDir.h"
#include "RttrConfig.h"
#include "Settings.h"
#include "Timer.h"
#include "addons/const_addons.h"
#include "convertSounds.h"
#include "files.h"
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
#include "gameTypes/Direction.h"
#include "gameData/JobConsts.h"
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

using namespace std::chrono;

Loader::Loader(Log& logger, const RttrConfig& config)
    : logger_(logger), config_(config), isWinterGFX_(false), map_gfx(nullptr), stp(nullptr)
{
    std::fill(nation_gfx.begin(), nation_gfx.end(), static_cast<libsiedler2::Archiv*>(nullptr));
}

Loader::~Loader() = default;

glArchivItem_Bitmap* Loader::GetImageN(const ResourceId& file, unsigned nr)
{
    return convertChecked<glArchivItem_Bitmap*>(files_[file].archiv[nr]);
}

ITexture* Loader::GetTextureN(const ResourceId& file, unsigned nr)
{
    return convertChecked<ITexture*>(files_[file].archiv[nr]);
}

glArchivItem_Bitmap* Loader::GetImage(const ResourceId& file, const std::string& name)
{
    return convertChecked<glArchivItem_Bitmap*>(files_[file].archiv.find(name));
}

glArchivItem_Bitmap_Player* Loader::GetPlayerImage(const ResourceId& file, unsigned nr)
{
    return convertChecked<glArchivItem_Bitmap_Player*>(files_[file].archiv[nr]);
}

glFont* Loader::GetFont(FontSize size)
{
    return fonts.empty() ? nullptr : &fonts[static_cast<unsigned>(size)];
}

libsiedler2::ArchivItem_Palette* Loader::GetPaletteN(const ResourceId& file, unsigned nr)
{
    return dynamic_cast<libsiedler2::ArchivItem_Palette*>(files_[file].archiv[nr]);
}

SoundEffectItem* Loader::GetSoundN(const ResourceId& file, unsigned nr)
{
    return dynamic_cast<SoundEffectItem*>(files_[file].archiv[nr]);
}

std::string Loader::GetTextN(const ResourceId& file, unsigned nr)
{
    auto* archiv = dynamic_cast<libsiedler2::ArchivItem_Text*>(files_[file].archiv[nr]);
    return archiv ? archiv->getText() : "text missing";
}

libsiedler2::Archiv& Loader::GetArchive(const ResourceId& file)
{
    RTTR_Assert(helpers::contains(files_, file));
    return files_[file].archiv;
}

glArchivItem_Bob* Loader::GetBobN(const ResourceId& file)
{
    return dynamic_cast<glArchivItem_Bob*>(files_[file].archiv.get(0));
}

glArchivItem_BitmapBase* Loader::GetNationImageN(unsigned nation, unsigned nr)
{
    return dynamic_cast<glArchivItem_BitmapBase*>(nation_gfx[nation]->get(nr));
}

glArchivItem_Bitmap* Loader::GetNationImage(unsigned nation, unsigned nr)
{
    return checkedCast<glArchivItem_Bitmap*>(GetNationImageN(nation, nr));
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

void Loader::AddOverrideFolder(std::string path, bool atBack)
{
    path = config_.ExpandPath(path);
    if(!bfs::exists(path))
        throw std::runtime_error(std::string("Path ") + path + " does not exist");
    // Don't add folders twice although it is not an error
    for(const OverrideFolder& cur : overrideFolders_)
    {
        if(bfs::equivalent(cur.path, path))
            return;
    }
    OverrideFolder folder;
    folder.path = path;
    for(const auto& it : bfs::directory_iterator(path))
        folder.files.push_back(it.path().filename().string());

    std::sort(folder.files.begin(), folder.files.end());

    for(FileEntry& entry : files_ | boost::adaptors::map_values)
        entry.loadedAfterOverrideChange = false;
    if(atBack)
        overrideFolders_.push_back(folder);
    else
        overrideFolders_.insert(overrideFolders_.begin(), folder);
}

void Loader::AddAddonFolder(AddonId id)
{
    std::stringstream s;
    s << config_.ExpandPath(FILE_PATHS[96]) << "/Addon_0x" << std::setw(8) << std::setfill('0') << std::hex << static_cast<unsigned>(id);
    std::string path = s.str();
    if(bfs::exists(path))
        AddOverrideFolder(path);
}

void Loader::ClearOverrideFolders()
{
    overrideFolders_.clear();
}

/**
 *  Load general files required also outside of games
 *
 *  @return @p true on success, @p false on error.
 */
bool Loader::LoadFilesAtStart()
{
    std::vector<unsigned> files = {
      5, 6, 7, 8, 9, 10, 17}; // Palettes:     pal5.bbm, pal6.bbm, pal7.bbm, paletti0.bbm, paletti1.bbm, paletti8.bbm, colors.act
    if(!LoadFiles(files))
        return false;

    if(!LoadFonts())
        return false;

    files.clear();
    files = {11,  12,  // Menu graphics:  resource.dat, io.dat
             102, 103, // Backgrounds: setup013.lbm, setup015.lbm
             64,  65,  66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84}; // Loading screens.

    if(!LoadFiles(files))
        return false;

    if(!LoadSounds())
        return false;

    return LoadOverrideFiles();
}

bool Loader::LoadSounds()
{
    std::string soundLSTPath = config_.ExpandPath(FILE_PATHS[55]);
    if(bfs::exists(soundLSTPath))
        bfs::remove(soundLSTPath);
    if(!Load(config_.ExpandPath(FILE_PATHS[49])))
        return false;
    const Timer timer(true);
    logger_.write(_("Starting sound conversion..."));
    if(!convertSounds(GetArchive("sound"), config_.ExpandPath(FILE_PATHS[56])))
    {
        logger_.write(_("failed\n"));
        return false;
    }
    logger_.write(_("done in %ums\n")) % duration_cast<milliseconds>(timer.getElapsed()).count();

    const std::string oggPath = config_.ExpandPath(FILE_PATHS[50]);
    std::vector<bfs::path> oggFiles = ListDir(oggPath, "ogg");

    sng_lst.reserve(oggFiles.size());
    for(const auto& oggFile : oggFiles)
    {
        libsiedler2::Archiv sng;
        if(!DoLoadFile(sng, oggFile))
            return false;
        auto music = libutil::dynamicUniqueCast<MusicItem>(sng.release(0));
        if(music)
            sng_lst.emplace_back(std::move(music));
        else
            logger_.write(_("WARNING: Found invalid music item for %1%")) % oggFile;
    }

    if(sng_lst.empty())
    {
        logger_.write(
          _("WARNING: Did not find the music files.\n\tYou have to run the updater once or copy the .ogg files manually to \"%1%\" or you "
            "won't be able to hear the music.\n"))
          % oggPath;
    }

    return true;
}

bool Loader::LoadFonts()
{
    if(!Load(config_.ExpandPath("<RTTR_RTTR>/LSTS/fonts.LST"), GetPaletteN("pal5")))
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
        files_["pal5"].archiv.pushC(*palette);
        // Player color palette
        for(int i = 128; i < 128 + libsiedler2::ArchivItem_Bitmap_Player::numPlayerClrs; i++)
            palette->set(i, libsiedler2::ColorRGB(i, i, i));
        files_["colors"].archiv.push(std::move(palette));
    }
    // GUI elements
    libsiedler2::Archiv& resource = files_["resource"].archiv;
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
    libsiedler2::Archiv& io = files_["io"].archiv;
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
bool Loader::LoadFilesAtGame(const std::string& mapGfxPath, bool isWinterGFX, const std::vector<Nation>& nations)
{
    if(helpers::contains(nations, NAT_BABYLONIANS))
        AddOverrideFolder("<RTTR_RTTR>/LSTS/GAME/Babylonier", false);

    std::vector<unsigned> files = {26, 44, 45, 86, 92, // rom_bobs.lst, carrier.bob, jobs.bob, boat.lst, boot_z.lst
                                   58, 59, 60, 61, 62,
                                   63,              // mis0bobs.lst, mis1bobs.lst, mis2bobs.lst, mis3bobs.lst, mis4bobs.lst, mis5bobs.lst
                                   35, 36, 37, 38}; // afr_icon.lst, jap_icon.lst, rom_icon.lst, vik_icon.lst

    // Add nation building graphics
    for(Nation nation : nations)
    {
        // New nations are handled by loading the override folder
        if(nation < NUM_NATIVE_NATS)
            files.push_back(27 + nation + (isWinterGFX ? NUM_NATIVE_NATS : 0));
    }

    if(!LoadFiles(files))
        return false;

    const libsiedler2::ArchivItem_Palette* pal5 = GetPaletteN("pal5");

    const std::string mapGFXFile = config_.ExpandPath(mapGfxPath);
    if(!Load(mapGFXFile, pal5))
        return false;
    map_gfx = &GetArchive(MakeResourceId(mapGFXFile));

    isWinterGFX_ = isWinterGFX;

    nation_gfx = {};
    for(Nation nation : nations)
        nation_gfx[nation] = &files_[NATION_GFXSET_Z[isWinterGFX ? 1 : 0][nation]].archiv;

    return true;
}

bool Loader::LoadOverrideFiles()
{
    for(const OverrideFolder& overrideFolder : overrideFolders_)
    {
        if(!LoadOverrideDirectory(overrideFolder.path))
            return false;
    }
    return true;
}

bool Loader::LoadFiles(const std::vector<std::string>& files)
{
    const libsiedler2::ArchivItem_Palette* pal5 = GetPaletteN("pal5");
    // load the files
    for(const std::string& curFile : files)
    {
        std::string filePath = config_.ExpandPath(curFile);
        if(!Load(filePath, pal5))
        {
            logger_.write(_("Failed to load %s\n")) % filePath;
            return false;
        }
    }

    return true;
}

void Loader::fillCaches()
{
    stp = std::make_unique<glTexturePacker>();

    // Animals
    for(unsigned species = 0; species < NUM_SPECS; ++species)
    {
        for(unsigned dir = 0; dir < Direction::COUNT; ++dir)
        {
            for(unsigned ani_step = 0; ani_step < ANIMALCONSTS[species].animation_steps; ++ani_step)
            {
                glSmartBitmap& bmp = animal_cache[species][dir][ani_step];

                bmp.reset();

                bmp.add(
                  GetMapImageN(ANIMALCONSTS[species].walking_id + ANIMALCONSTS[species].animation_steps * ((dir + 3) % 6) + ani_step));

                if(ANIMALCONSTS[species].shadow_id)
                {
                    if(species == SPEC_DUCK)
                        // Ente Sonderfall, da gibts nur einen Schatten für jede Richtung!
                        bmp.addShadow(GetMapImageN(ANIMALCONSTS[species].shadow_id));
                    else
                        // ansonsten immer pro Richtung einen Schatten
                        bmp.addShadow(GetMapImageN(ANIMALCONSTS[species].shadow_id + (dir + 3) % 6));
                }

                stp->add(bmp);
            }
        }

        glSmartBitmap& bmp = animal_cache[species][0][ANIMAL_MAX_ANIMATION_STEPS];

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

    glArchivItem_Bob* bob_jobs = GetBobN("jobs");
    if(!bob_jobs)
        throw std::runtime_error("jobs not found");

    for(unsigned nation = 0; nation < NUM_NATS; ++nation)
    {
        if(!nation_gfx[nation])
            continue;
        // BUILDINGS
        for(unsigned type = 0; type < NUM_BUILDING_TYPES; ++type)
        {
            glSmartBitmap& bmp = building_cache[nation][type][0];
            glSmartBitmap& skel = building_cache[nation][type][1];

            bmp.reset();
            skel.reset();

            if(type == BLD_CHARBURNER)
            {
                unsigned id = nation * 8;

                bmp.add(GetImageN("charburner", id + (isWinterGFX_ ? 6 : 1)));
                bmp.addShadow(GetImageN("charburner", id + 2));

                skel.add(GetImageN("charburner", id + 3));
                skel.addShadow(GetImageN("charburner", id + 4));
            } else
            {
                bmp.add(GetNationImage(nation, 250 + 5 * type));
                bmp.addShadow(GetNationImage(nation, 250 + 5 * type + 1));
                if(type == BLD_HEADQUARTERS)
                {
                    // HQ has no skeleton, but we have a tent that can act as an HQ
                    skel.add(GetImageN("mis0bobs", 6));
                    skel.addShadow(GetImageN("mis0bobs", 7));
                } else
                {
                    skel.add(GetNationImage(nation, 250 + 5 * type + 2));
                    skel.addShadow(GetNationImage(nation, 250 + 5 * type + 3));
                }
            }

            stp->add(bmp);
            stp->add(skel);
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

        // Bobs from jobs.bob. Job = NUM_JOB_TYPES is used for fat carriers. See below.
        for(unsigned job = 0; job < NUM_JOB_TYPES + 1; ++job)
        {
            for(unsigned dir = 0; dir < Direction::COUNT; ++dir)
            {
                for(unsigned ani_step = 0; ani_step < 8; ++ani_step)
                {
                    bool fat;
                    unsigned id;
                    unsigned short overlayOffset = 96;

                    glSmartBitmap& bmp = bob_jobs_cache[nation][job][dir][ani_step];

                    bmp.reset();

                    if(job == NUM_JOB_TYPES) // used for fat carrier, so that we do not need an additional sub-array
                    {
                        fat = true;
                        id = 0;
                    } else
                    {
                        id = JOB_CONSTS[job].jobs_bob_id;
                        fat = JOB_CONSTS[job].fat;

                        if((job == JOB_SCOUT) || ((job >= JOB_PRIVATE) && (job <= JOB_GENERAL)))
                        {
                            if(nation < NUM_NATIVE_NATS)
                            {
                                id += NATION_RTTR_TO_S2[nation] * 6;
                            } else if(nation == NAT_BABYLONIANS) //-V547
                            {
                                id += NATION_RTTR_TO_S2[NAT_ROMANS] * 6;
                                /* TODO: change this once we have own job pictures for babylonians
                                                                //Offsets to std::make_unique<job imgs>()
                                                                overlayOffset = (job == JOB_SCOUT) ? 1740 : 1655;

                                                                //8 Frames * 6 Directions * 6 Types
                                                                overlayOffset += (nation - NUM_NATIVE_NATS) * (8 * 6 * 6);
                                */
                            } else
                                throw std::runtime_error("Wrong nation");
                        }
                    }

                    unsigned good = id * 96 + ani_step * 12 + ((dir + 3) % 6) + fat * 6;
                    unsigned body = fat * 48 + ((dir + 3) % 6) * 8 + ani_step;

                    if(bob_jobs->getLink(good) == 92)
                    {
                        good -= fat * 6;
                        body -= fat * 48;
                    }

                    bmp.add(dynamic_cast<glArchivItem_Bitmap_Player*>(bob_jobs->get(body)));
                    bmp.add(dynamic_cast<glArchivItem_Bitmap_Player*>(bob_jobs->get(overlayOffset + bob_jobs->getLink(good))));
                    bmp.addShadow(GetMapImageN(900 + ((dir + 3) % 6) * 8 + ani_step));

                    stp->add(bmp);
                }
            }
        }

        glSmartBitmap& bmp = boundary_stone_cache[nation];

        bmp.reset();

        bmp.add(GetNationPlayerImage(nation, 0));
        bmp.addShadow(GetNationImage(nation, 1));

        stp->add(bmp);
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
    for(unsigned dir = 0; dir < Direction::COUNT; ++dir)
    {
        for(unsigned ani_step = 0; ani_step < 8; ++ani_step)
        {
            glSmartBitmap& bmp = donkey_cache[dir][ani_step];

            bmp.reset();

            bmp.add(GetMapImageN(2000 + ((dir + 3) % 6) * 8 + ani_step));
            bmp.addShadow(GetMapImageN(2048 + dir % 3));

            stp->add(bmp);
        }
    }

    // Boats
    for(unsigned dir = 0; dir < Direction::COUNT; ++dir)
    {
        for(unsigned ani_step = 0; ani_step < 8; ++ani_step)
        {
            glSmartBitmap& bmp = boat_cache[dir][ani_step];

            bmp.reset();

            bmp.add(GetPlayerImage("boat", ((dir + 3) % 6) * 8 + ani_step));
            bmp.addShadow(GetMapImageN(2048 + dir % 3));

            stp->add(bmp);
        }
    }

    // carrier_cache[ware][direction][animation_step][fat]
    glArchivItem_Bob* bob_carrier = GetBobN("carrier");
    if(!bob_carrier)
        throw std::runtime_error("carrier not found");

    for(unsigned ware = 0; ware < NUM_WARE_TYPES; ++ware)
    {
        for(unsigned dir = 0; dir < Direction::COUNT; ++dir)
        {
            for(unsigned ani_step = 0; ani_step < 8; ++ani_step)
            {
                for(unsigned fat = 0; fat < 2; ++fat)
                {
                    glSmartBitmap& bmp = carrier_cache[ware][dir][ani_step][fat];
                    bmp.reset();

                    unsigned id;
                    // Japanese shield is missing
                    if(ware == GD_SHIELDJAPANESE)
                        id = GD_SHIELDROMANS;
                    else
                        id = ware;

                    unsigned imgDir = (dir + 3) % 6;

                    unsigned good = id * 96 + ani_step * 12 + fat * 6 + imgDir;
                    unsigned body = fat * 48 + imgDir * 8 + ani_step;

                    bmp.add(dynamic_cast<glArchivItem_Bitmap_Player*>(bob_carrier->get(body)));
                    bmp.add(dynamic_cast<glArchivItem_Bitmap_Player*>(bob_carrier->get(96 + bob_carrier->getLink(good))));
                    bmp.addShadow(GetMapImageN(900 + imgDir * 8 + ani_step));

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

            image->print(&buffer.front(), width, height, libsiedler2::TextureFormat::Paletted, palette, 0, 0, 0, 0, width, height);

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
                bitmap->create(width, height, &buffer.front(), width, height, libsiedler2::TextureFormat::Paletted, palette);
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

/// Create a resource id which is the file name without the extension and converted to lowercase
ResourceId Loader::MakeResourceId(const bfs::path& filepath)
{
    const auto name = filepath.stem();
    return ResourceId{s25util::toLower(name.string())};
}

std::vector<bfs::path> Loader::GetFilesToLoad(const bfs::path& filepath)
{
    std::vector<bfs::path> result;
    result.push_back(filepath);
    const ResourceId resId = MakeResourceId(filepath);
    for(const OverrideFolder& overrideFolder : overrideFolders_)
    {
        auto itFile = helpers::find_if(overrideFolder.files, [&resId](const bfs::path& file) { return MakeResourceId(file) == resId; });
        if(itFile != overrideFolder.files.end())
        {
            const auto fullFilePath = overrideFolder.path / *itFile;
            if(!bfs::exists(fullFilePath))
                logger_.write(_("Skipping removed file %1% when checking for files to load for %2%")) % fullFilePath % resId;
            else if(helpers::contains(result, fullFilePath))
                logger_.write(_("Skipping duplicate override file %1% for %2%")) % fullFilePath % resId;
            else
                result.push_back(fullFilePath);
        }
    }
    return result;
}

bool Loader::MergeArchives(libsiedler2::Archiv& targetArchiv, libsiedler2::Archiv& otherArchiv)
{
    if(targetArchiv.size() < otherArchiv.size())
        targetArchiv.alloc_inc(otherArchiv.size() - targetArchiv.size());
    for(unsigned i = 0; i < otherArchiv.size(); i++)
    {
        // Skip empty entries
        if(!otherArchiv[i])
            continue;
        // If target entry is empty, just move the new one
        if(!targetArchiv[i])
            targetArchiv.set(i, otherArchiv.release(i));
        else
        {
            auto* subArchiv = dynamic_cast<libsiedler2::Archiv*>(targetArchiv[i]);
            if(subArchiv)
            {
                // We have a sub-archiv -> Merge
                auto* otherSubArchiv = dynamic_cast<libsiedler2::Archiv*>(otherArchiv[i]);
                if(!otherSubArchiv)
                {
                    logger_.write(_("Failed to merge entry %1%. Archive expected!\n")) % i;
                    return false;
                }
                if(!MergeArchives(*subArchiv, *otherSubArchiv))
                    return false;
            } else
                targetArchiv.set(i, otherArchiv.release(i)); // Just replace
        }
    }
    return true;
}

bool Loader::Load(libsiedler2::Archiv& archiv, const bfs::path& path, const libsiedler2::ArchivItem_Palette* palette)
{
    archiv.clear();
    const std::vector<bfs::path> filesToLoad = GetFilesToLoad(path);
    for(const bfs::path& curFilepath : filesToLoad)
    {
        libsiedler2::Archiv newEntries;
        if(!DoLoadFileOrDirectory(newEntries, curFilepath, palette))
            return false;
        if(!MergeArchives(archiv, newEntries))
            return false;
    }
    return true;
}

/**
 *  @brief Load the given file or directory
 *
 *  @param pfad Path to file or directory
 *  @param palette Palette to use for possible graphic files
 */
bool Loader::Load(const bfs::path& pfad, const libsiedler2::ArchivItem_Palette* palette, bool isFromOverrideDir)
{
    FileEntry& entry = files_[MakeResourceId(pfad)];
    // Load if: 1. Not loaded
    //          2. archive content changed BUT we are not loading an override file or the file wasn't loaded since the last override
    //          change
    if(entry.archiv.empty() || (entry.filesUsed != GetFilesToLoad(pfad) && (!isFromOverrideDir || !entry.loadedAfterOverrideChange)))
    {
        if(!Load(entry.archiv, pfad, palette))
            return false;
        entry.loadedAfterOverrideChange = true;
    }
    return true;
}

class NestedArchive : public libsiedler2::Archiv, public libsiedler2::ArchivItem
{
    RTTR_CLONEABLE(NestedArchive)
};

/**
 *  @brief Loads a file or directory into an archiv
 *
 *  @param filePath Path to file or directory
 *  @param palette Palette to use for possible graphic files
 *  @param to Archive to write to
 */
bool Loader::DoLoadFileOrDirectory(libsiedler2::Archiv& to, const boost::filesystem::path& filePath,
                                   const libsiedler2::ArchivItem_Palette* palette)
{
    if(!exists(filePath))
    {
        logger_.write(_("File or directory does not exist: %s\n")) % filePath;
        return false;
    }
    if(is_regular_file(filePath))
        return DoLoadFile(to, filePath, palette);
    if(!is_directory(filePath))
    {
        logger_.write(_("Could not determine type of path %s\n")) % filePath;
        return false;
    }

    logger_.write(_("Loading directory %s\n")) % filePath;
    const Timer timer(true);
    std::vector<libsiedler2::FileEntry> files = libsiedler2::ReadFolderInfo(filePath.string());
    logger_.write(_("  Loading %1% entries: ")) % files.size();

    libsiedler2::Archiv* archiveToFill = &to;
    // Emulate bob structure: Single file where first entry is the BOB archive
    if(s25util::toLower(filePath.extension().string()) == ".bob")
    {
        if(to.empty())
            to.alloc(1);
        to.set(0, std::make_unique<NestedArchive>());
        archiveToFill = static_cast<NestedArchive*>(to[0]);
    }

    if(int ec = libsiedler2::LoadFolder(files, *archiveToFill, palette))
    {
        logger_.write(_("failed: %1%\n")) % libsiedler2::getErrorString(ec);
        return false;
    }

    logger_.write(_("done in %ums\n")) % duration_cast<milliseconds>(timer.getElapsed()).count();

    return true;
}

/**
 *  @brief Load a single file into the archive
 *
 *  @param[in] archiv Archive to add file to
 *  @param[in] filePath Path to file
 *  @param[in] palette palette to use if required
 */
bool Loader::DoLoadFile(libsiedler2::Archiv& archiv, const boost::filesystem::path& filePath,
                        const libsiedler2::ArchivItem_Palette* palette)
{
    const Timer timer(true);

    logger_.write(_("Loading \"%s\": ")) % filePath;
    fflush(stdout);

    if(int ec = libsiedler2::Load(filePath.string(), archiv, palette))
    {
        logger_.write(_("failed: %1%\n")) % libsiedler2::getErrorString(ec);
        return false;
    }

    logger_.write(_("done in %ums\n")) % duration_cast<milliseconds>(timer.getElapsed()).count();

    return true;
}

bool Loader::LoadOverrideDirectory(const bfs::path& path)
{
    if(!bfs::is_directory(path))
    {
        logger_.write(_("Directory does not exist: %s\n")) % path;
        return false;
    }

    const Timer timer(true);

    logger_.write(_("Loading LST,LBM,BOB,IDX,BMP,TXT,GER,ENG,INI files from \"%s\"\n")) % path;

    std::vector<bfs::path> filesAndFolders;
    for(const auto ext : {"lst", "lbm", "bob", "idx", "bmp", "txt", "ger", "eng", "ini"})
    {
        std::vector<bfs::path> curFiles = ListDir(path, ext, true);
        filesAndFolders.insert(filesAndFolders.end(), curFiles.begin(), curFiles.end());
    }

    const libsiedler2::ArchivItem_Palette* pal5 = GetPaletteN("pal5");
    for(const bfs::path& curPath : filesAndFolders)
    {
        if(!Load(curPath, pal5, true))
            return false;
    }
    logger_.write(_("finished in %ums\n")) % duration_cast<milliseconds>(timer.getElapsed()).count();
    return true;
}

/**
 *  Lädt Dateien aus FILE_PATHS bzw aus dem Verzeichnis.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler.
 */
bool Loader::LoadFiles(const std::vector<unsigned>& fileIndices)
{
    std::vector<std::string> files(fileIndices.size());
    std::transform(fileIndices.begin(), fileIndices.end(), files.begin(), [](unsigned idx) { return FILE_PATHS[idx]; });
    return LoadFiles(files);
}

Loader& getGlobalLoader()
{
    static Loader loader{LOG, RTTRCONFIG};
    return loader;
}
