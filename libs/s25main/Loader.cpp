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

///////////////////////////////////////////////////////////////////////////////

#include "rttrDefines.h" // IWYU pragma: keep
#include "Loader.h"
#include "ListDir.h"
#include "RttrConfig.h"
#include "Settings.h"
#include "addons/const_addons.h"
#include "convertSounds.h"
#include "drivers/VideoDriverWrapper.h"
#include "files.h"
#include "helpers/containerUtils.h"
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
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/assign/std/vector.hpp>
#include <boost/filesystem.hpp>
#include <boost/range/adaptor/map.hpp>
#include <algorithm>
#include <cstdio>
#include <iomanip>
#include <memory>
#include <sstream>
#include <stdexcept>

Loader::Loader() : isWinterGFX_(false), map_gfx(nullptr), stp(nullptr)
{
    std::fill(nation_gfx.begin(), nation_gfx.end(), static_cast<libsiedler2::Archiv*>(nullptr));
}

Loader::~Loader() = default;

glArchivItem_Bitmap* Loader::GetImageN(const std::string& file, unsigned nr)
{
    return convertChecked<glArchivItem_Bitmap*>(files_[file].archiv[nr]);
}

ITexture* Loader::GetTextureN(const std::string& file, unsigned nr)
{
    return convertChecked<ITexture*>(files_[file].archiv[nr]);
}

glArchivItem_Bitmap* Loader::GetImage(const std::string& file, const std::string& name)
{
    return convertChecked<glArchivItem_Bitmap*>(files_[file].archiv.find(name));
}

glArchivItem_Bitmap_Player* Loader::GetPlayerImage(const std::string& file, unsigned nr)
{
    return convertChecked<glArchivItem_Bitmap_Player*>(files_[file].archiv[nr]);
}

glFont* Loader::GetFont(FontSize size)
{
    return fonts.empty() ? nullptr : &fonts[static_cast<unsigned>(size)];
}

libsiedler2::ArchivItem_Palette* Loader::GetPaletteN(const std::string& file, unsigned nr)
{
    return dynamic_cast<libsiedler2::ArchivItem_Palette*>(files_[file].archiv[nr]);
}

SoundEffectItem* Loader::GetSoundN(const std::string& file, unsigned nr)
{
    return dynamic_cast<SoundEffectItem*>(files_[file].archiv[nr]);
}

std::string Loader::GetTextN(const std::string& file, unsigned nr)
{
    auto* archiv = dynamic_cast<libsiedler2::ArchivItem_Text*>(files_[file].archiv[nr]);
    return archiv ? archiv->getText() : "text missing";
}

libsiedler2::Archiv& Loader::GetArchive(const std::string& file)
{
    RTTR_Assert(helpers::contains(files_, file));
    return files_[file].archiv;
}

glArchivItem_Bob* Loader::GetBobN(const std::string& file)
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
    path = RTTRCONFIG.ExpandPath(path);
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
    s << RTTRCONFIG.ExpandPath(FILE_PATHS[96]) << "/Addon_0x" << std::setw(8) << std::setfill('0') << std::hex << static_cast<unsigned>(id);
    std::string path = s.str();
    if(bfs::exists(path))
        AddOverrideFolder(path);
}

void Loader::ClearOverrideFolders()
{
    overrideFolders_.clear();
}

/**
 *  Lädt alle allgemeinen Dateien.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler.
 */
bool Loader::LoadFilesAtStart()
{
    using namespace boost::assign; // Adds the vector += operator
    std::vector<unsigned> files;

    files += 5, 6, 7, 8, 9, 10, 17; // Paletten:     pal5.bbm, pal6.bbm, pal7.bbm, paletti0.bbm, paletti1.bbm, paletti8.bbm, colors.act
    if(!LoadFilesFromArray(files))
        return false;

    if(!LoadFonts())
        return false;

    files.clear();
    files += 11, 12,                                                                      // Menüdateien:  resource.dat, io.dat
      102, 103,                                                                           // Hintergründe: setup013.lbm, setup015.lbm
      64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84; // Die ganzen Spielladescreens.

    if(!LoadFilesFromArray(files))
        return false;

    if(!LoadSounds())
        return false;

    return LoadOverrideFiles();
}

/**
 *  Lädt alle Sounds.
 *
 *  @return liefert true bei Erfolg, false bei Fehler
 */
bool Loader::LoadSounds()
{
    std::string soundLSTPath = RTTRCONFIG.ExpandPath(FILE_PATHS[55]);
    if(bfs::exists(soundLSTPath))
        bfs::remove(soundLSTPath);
    if(!LoadFile(RTTRCONFIG.ExpandPath(FILE_PATHS[49])))
        return false;
    auto const convertStartTime = VIDEODRIVER.GetTickCount();
    LOG.write(_("Starting sound conversion..."));
    if(!convertSounds(GetArchive("sound"), RTTRCONFIG.ExpandPath(FILE_PATHS[56])))
    {
        LOG.write(_("failed\n"));
        return false;
    }
    LOG.write(_("done in %ums\n")) % (VIDEODRIVER.GetTickCount() - convertStartTime);

    const std::string oggPath = RTTRCONFIG.ExpandPath(FILE_PATHS[50]);
    std::vector<std::string> oggFiles = ListDir(oggPath, "ogg");

    unsigned i = 0;
    sng_lst.alloc(oggFiles.size());
    for(auto& oggFile : oggFiles)
    {
        libsiedler2::Archiv sng;
        if(!LoadArchiv(sng, oggFile))
            return false;
        sng_lst.set(i++, sng.release(0));
    }

    if(sng_lst.empty())
    {
        LOG.write(
          _("WARNING: Did not find the music files.\n\tYou have to run the updater once or copy the .ogg files manually to \"%1%\" or you "
            "won't be able to hear the music.\n"))
          % oggPath;
    }

    return true;
}

bool Loader::LoadFonts()
{
    if(!LoadFile(RTTRCONFIG.ExpandPath("<RTTR_RTTR>/LSTS/fonts.LST"), GetPaletteN("pal5")))
        return false;
    fonts.clear();
    const auto& loadedFonts = GetArchive("fonts");
    for(unsigned i = 0; i <= helpers::MaxEnumValue_v<FontSize>; i++)
    {
        const auto* curFont = dynamic_cast<const libsiedler2::ArchivItem_Font*>(loadedFonts[i]);
        if(!curFont)
        {
            LOG.write(_("Unable to load font at index %1%\n")) % i;
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
 *  Lädt die Spieldateien.
 *
 *  @param[in] gfxset  Das GFX-Set
 *  @param[in] nations Array der zu ladenden Nationen.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler.
 */
bool Loader::LoadFilesAtGame(const std::string& mapGfxPath, bool isWinterGFX, const std::vector<bool>& nations)
{
    if(NAT_BABYLONIANS < nations.size() && nations[NAT_BABYLONIANS])
        AddOverrideFolder("<RTTR_RTTR>/LSTS/GAME/Babylonier", false);

    using namespace boost::assign; // Adds the vector += operator
    std::vector<unsigned> files;

    files += 26, 44, 45, 86, 92, // rom_bobs.lst, carrier.bob, jobs.bob, boat.lst, boot_z.lst
      58, 59, 60, 61, 62, 63,    // mis0bobs.lst, mis1bobs.lst, mis2bobs.lst, mis3bobs.lst, mis4bobs.lst, mis5bobs.lst
      35, 36, 37, 38;            // afr_icon.lst, jap_icon.lst, rom_icon.lst, vik_icon.lst

    for(unsigned char i = 0; i < NUM_NATIVE_NATS; ++i)
    {
        // ggf. Völker-Grafiken laden
        if((i < nations.size() && nations[i]) || (i == NAT_ROMANS && NAT_BABYLONIANS < nations.size() && nations[NAT_BABYLONIANS]))
            files.push_back(27 + i + (isWinterGFX ? NUM_NATIVE_NATS : 0));
    }

    // Load files
    if(!LoadFilesFromArray(files))
        return false;

    const libsiedler2::ArchivItem_Palette* pal5 = GetPaletteN("pal5");

    std::string mapGFXFile = RTTRCONFIG.ExpandPath(mapGfxPath);
    if(!LoadFile(mapGFXFile, pal5))
        return false;
    map_gfx = &GetArchive(boost::algorithm::to_lower_copy(bfs::path(mapGFXFile).stem().string()));

    isWinterGFX_ = isWinterGFX;

    // TODO: Only put actually required archives here
    for(unsigned nation = 0; nation < NUM_NATS; ++nation)
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
        std::string filePath = RTTRCONFIG.ExpandPath(curFile);
        if(!LoadFile(filePath, pal5))
        {
            LOG.write(_("Failed to load %s\n")) % filePath;
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
                                id += NATION_RTTR_TO_S2[nation] * 6;
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

            image->print(&buffer.front(), width, height, libsiedler2::FORMAT_PALETTED, palette, 0, 0, 0, 0, width, height);

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
                bitmap->create(width, height, &buffer.front(), width, height, libsiedler2::FORMAT_PALETTED, palette);
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

std::vector<std::string> Loader::GetFilesToLoad(const std::string& filepath)
{
    std::vector<std::string> result;
    result.push_back(filepath);
    const std::string filename = bfs::path(filepath).filename().string();
    for(const OverrideFolder& overrideFolder : overrideFolders_)
    {
        for(const std::string& overrideFile : overrideFolder.files)
        {
            if(overrideFile == filename)
            {
                const auto fullFilePath = bfs::path(overrideFolder.path) / overrideFile;
                if(!bfs::exists(fullFilePath))
                    continue;
                const std::string overideFilepath = fullFilePath.string(); // NOLINT
                if(!helpers::contains(result, overideFilepath))
                    result.push_back(overideFilepath);
                break;
            }
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
        // If target entry is empty, just move the std::make_unique<one>()
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
                    LOG.write(_("Failed to merge entry %1%. Archive expected!\n")) % i;
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

bool Loader::LoadFile(libsiedler2::Archiv& archiv, const std::string& pfad, const libsiedler2::ArchivItem_Palette* palette)
{
    archiv.clear();
    const std::vector<std::string> filesToLoad = GetFilesToLoad(pfad);
    for(const std::string& curFilepath : filesToLoad)
    {
        libsiedler2::Archiv newEntries;
        if(!LoadSingleFile(newEntries, curFilepath, palette))
            return false;
        if(!MergeArchives(archiv, newEntries))
            return false;
    }
    return true;
}

/**
 *  @brief
 *
 *  @param pfad Path to file or directory
 *  @param palette Palette to use for possible graphic files
 */
bool Loader::LoadFile(const std::string& pfad, const libsiedler2::ArchivItem_Palette* palette, bool isFromOverrideDir)
{
    std::string lowerPath = boost::algorithm::to_lower_copy(pfad);
    std::string name = bfs::path(lowerPath).filename().stem().string();

    FileEntry& entry = files_[name];
    // Load if: 1. Not loaded
    //          2. archive content changed BUT we are not loading an override file or the file wasn't loaded since the last override
    //          change
    if(entry.archiv.empty() || (entry.filesUsed != GetFilesToLoad(pfad) && (!isFromOverrideDir || !entry.loadedAfterOverrideChange)))
    {
        if(!LoadFile(entry.archiv, pfad, palette))
            return false;
        entry.loadedAfterOverrideChange = true;
    }
    return true;
}

/**
 *  @brief Loads a file or directory into an archiv
 *
 *  @param filePath Path to file or directory
 *  @param palette Palette to use for possible graphic files
 *  @param to Archtive to write to
 */
bool Loader::LoadSingleFile(libsiedler2::Archiv& to, const std::string& filePath, const libsiedler2::ArchivItem_Palette* palette)
{
    if(filePath.at(0) == '~')
        throw std::logic_error("You must use resolved pathes: " + filePath);

    if(!boost::filesystem::exists(filePath))
    {
        LOG.write(_("File or directory does not exist: %s\n")) % filePath;
        return false;
    }
    if(boost::filesystem::is_regular_file(filePath))
        return LoadArchiv(to, filePath, palette);
    if(!boost::filesystem::is_directory(filePath))
    {
        LOG.write(_("Could not determine type of path %s\n")) % filePath;
        return false;
    }

    LOG.write(_("Loading directory %s\n")) % filePath;
    unsigned startTime = VIDEODRIVER.GetTickCount();
    std::vector<libsiedler2::FileEntry> files = libsiedler2::ReadFolderInfo(filePath);
    LOG.write(_("  Loading %1% entries: ")) % files.size();
    if(int ec = libsiedler2::LoadFolder(files, to, palette))
    {
        LOG.write(_("failed: %1%\n")) % libsiedler2::getErrorString(ec);
        return false;
    }
    LOG.write(_("done in %ums\n")) % (VIDEODRIVER.GetTickCount() - startTime);

    return true;
}

/**
 *  @brief Lädt eine Datei in ein Archiv.
 *
 *  @param[in] pfad    Der Dateipfad
 *  @param[in] palette (falls benötigt) die Palette.
 *  @param[in] archiv  Das Zielarchivinfo.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler.
 */
bool Loader::LoadArchiv(libsiedler2::Archiv& archiv, const std::string& pfad, const libsiedler2::ArchivItem_Palette* palette)
{
    unsigned ladezeit = VIDEODRIVER.GetTickCount();

    std::string file = RTTRCONFIG.ExpandPath(pfad);

    LOG.write(_("Loading \"%s\": ")) % file;
    fflush(stdout);

    if(int ec = libsiedler2::Load(file, archiv, palette))
    {
        LOG.write(_("failed: %1%\n")) % libsiedler2::getErrorString(ec);
        return false;
    }

    LOG.write(_("done in %ums\n")) % (VIDEODRIVER.GetTickCount() - ladezeit);

    return true;
}

bool Loader::LoadOverrideDirectory(const std::string& path)
{
    if(!bfs::is_directory(path))
    {
        LOG.write(_("Directory does not exist: %s\n")) % path;
        return false;
    }

    // yes, load all files in the directory
    unsigned ladezeit = VIDEODRIVER.GetTickCount();

    LOG.write(_("Loading LST,LBM,BOB,IDX,BMP,TXT,GER,ENG,INI files from \"%s\"\n")) % RTTRCONFIG.ExpandPath(path);

    std::vector<std::string> filesAndFolders = ListDir(path, "lst", true);
    filesAndFolders = ListDir(path, "lbm", true, &filesAndFolders);
    filesAndFolders = ListDir(path, "bob", true, &filesAndFolders);
    filesAndFolders = ListDir(path, "idx", true, &filesAndFolders);
    filesAndFolders = ListDir(path, "bmp", true, &filesAndFolders);
    filesAndFolders = ListDir(path, "txt", true, &filesAndFolders);
    filesAndFolders = ListDir(path, "ger", true, &filesAndFolders);
    filesAndFolders = ListDir(path, "eng", true, &filesAndFolders);
    filesAndFolders = ListDir(path, "ini", true, &filesAndFolders);

    const libsiedler2::ArchivItem_Palette* pal5 = GetPaletteN("pal5");
    for(const std::string& i : filesAndFolders)
    {
        if(!LoadFile(i, pal5, true))
            return false;
    }
    LOG.write(_("finished in %ums\n")) % (VIDEODRIVER.GetTickCount() - ladezeit);
    return true;
}

/**
 *  Lädt Dateien aus FILE_PATHS bzw aus dem Verzeichnis.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler.
 */
bool Loader::LoadFilesFromArray(const std::vector<unsigned>& files)
{
    std::vector<std::string> sFiles;
    sFiles.reserve(files.size());
    for(unsigned curFileIdx : files)
        sFiles.push_back(FILE_PATHS[curFileIdx]);
    return LoadFiles(sFiles);
}
