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
#include "drivers/VideoDriverWrapper.h"
#include "files.h"
#include "helpers/Deleter.h"
#include "ogl/SoundEffectItem.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "ogl/glArchivItem_Bitmap_RLE.h"
#include "ogl/glArchivItem_Bitmap_Raw.h"
#include "ogl/glArchivItem_Bob.h"
#include "ogl/glArchivItem_Font.h"
#include "ogl/glSmartBitmap.h"
#include "ogl/glTexturePacker.h"
#include "gameTypes/Direction.h"
#include "gameData/JobConsts.h"
#include "libsiedler2/ArchivItem_Ini.h"
#include "libsiedler2/ArchivItem_Palette.h"
#include "libsiedler2/ArchivItem_PaletteAnimation.h"
#include "libsiedler2/ArchivItem_Text.h"
#include "libsiedler2/ErrorCodes.h"
#include "libsiedler2/IAllocator.h"
#include "libsiedler2/PixelBufferARGB.h"
#include "libsiedler2/PixelBufferPaletted.h"
#include "libsiedler2/libsiedler2.h"
#include "libutil/Log.h"
#include "libutil/StringConversion.h"
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/assign/std/vector.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
#include <algorithm>
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <stdexcept>

Loader::Loader() : isWinterGFX_(false), map_gfx(NULL), stp(NULL)
{
    std::fill(nation_gfx.begin(), nation_gfx.end(), static_cast<libsiedler2::Archiv*>(NULL));
}

Loader::~Loader()
{
    delete stp;
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

    files += 5, 6, 7, 8, 9, 10, 17, // Paletten:     pal5.bbm, pal6.bbm, pal7.bbm, paletti0.bbm, paletti1.bbm, paletti8.bbm, colors.act
      11, 12,                       // Menüdateien:  resource.dat, io.dat
      102, 103,                     // Hintergründe: setup013.lbm, setup015.lbm
      64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84; // Die ganzen Spielladescreens.

    if(!LoadFilesFromArray(files, true))
        return false;

    if(!LoadSounds())
        return false;

    if(!LoadLsts(95)) // lade systemweite und persönliche lst files
        return false;

    return true;
}

/**
 *  @brief
 *
 *  @param isOriginal If this is set to true, the file is considered to be the base archiv so all possibly loaded overrides are
 * removed/overwritten first
 */
bool Loader::LoadFileOrDir(const std::string& file, bool isOriginal)
{
    if(file.at(0) == '~')
        throw std::logic_error("You must use resolved pathes: " + file);

    if(!bfs::exists(file))
    {
        LOG.write(_("File or directory does not exist: %s\n")) % file;
        return false;
    }
    // is the entry a directory?
    if(bfs::is_directory(file))
    {
        // yes, load all files in the directory
        unsigned ladezeit = VIDEODRIVER.GetTickCount();

        LOG.write(_("Loading LST,LBM,BOB,IDX,BMP,TXT,GER,ENG,INI files from \"%s\"\n")) % RTTRCONFIG.ExpandPath(file);

        std::vector<std::string> lst = ListDir(file, "lst", true);
        lst = ListDir(file, "lbm", true, &lst);
        lst = ListDir(file, "bob", true, &lst);
        lst = ListDir(file, "idx", true, &lst);
        lst = ListDir(file, "bmp", true, &lst);
        lst = ListDir(file, "txt", true, &lst);
        lst = ListDir(file, "ger", true, &lst);
        lst = ListDir(file, "eng", true, &lst);
        lst = ListDir(file, "ini", true, &lst);

        libsiedler2::ArchivItem_Palette* pal5 = GetPaletteN("pal5");
        for(std::vector<std::string>::iterator i = lst.begin(); i != lst.end(); ++i)
        {
            if(!LoadFile(*i, pal5, isOriginal))
                return false;
        }
        LOG.write(_("finished in %ums\n")) % (VIDEODRIVER.GetTickCount() - ladezeit);
    } else
    {
        // no, only single file specified
        if(!LoadFile(file, GetPaletteN("pal5"), isOriginal))
            return false;
    }
    return true;
}

/**
 *  Lädt Dateien aus FILE_PATHS bzw aus dem Verzeichnis.
 *
 *  @param isOriginal If this is set to true, the file is considered to be the base archiv so all possibly loaded overrides are
 * removed/overwritten first
 *
 *  @return @p true bei Erfolg, @p false bei Fehler.
 */
bool Loader::LoadFilesFromArray(const std::vector<unsigned>& files, bool isOriginal)
{
    // load the files or directorys
    BOOST_FOREACH(unsigned curFileIdx, files)
    {
        std::string filePath = RTTRCONFIG.ExpandPath(FILE_PATHS[curFileIdx]);
        if(!LoadFileOrDir(filePath, isOriginal))
        {
            LOG.write(_("Failed to load %s\n")) % filePath;
            return false;
        }
    }

    return true;
}

/**
 *  Lädt die "override" lst-files aus den systemweiten und persönlichen verzeichnissen
 *
 *  @return @p true bei Erfolg, @p false bei Fehler.
 */
bool Loader::LoadLsts(unsigned dir)
{
    // systemweite lsts laden
    std::vector<unsigned> files(1, dir);

    if(!bfs::equivalent(RTTRCONFIG.ExpandPath(FILE_PATHS[dir]), RTTRCONFIG.ExpandPath(FILE_PATHS[dir + 3])))
        files.push_back(dir + 3);

    return LoadFilesFromArray(files, false);
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
    {
        // Archive might be faulty: Remove if it is and recreate
        if(!LoadFile(soundLSTPath, NULL, true))
            bfs::remove(soundLSTPath);
    }
    // ist die konvertierte sound.lst vorhanden?
    if(!bfs::exists(soundLSTPath))
    {
        // nein, dann konvertieren

        std::stringstream cmdss;
        cmdss << RTTRCONFIG.ExpandPath(FILE_PATHS[57]); // pfad zum sound-converter hinzufügen

// name anhängen
#ifdef _WIN32
        cmdss << "\\sound-convert.exe";
#else
        cmdss << "/sound-convert";
#endif

        // parameter anhängen
        cmdss << " -s \"";
        cmdss << RTTRCONFIG.ExpandPath(FILE_PATHS[56]); // script
        cmdss << "\" -f \"";
        cmdss << RTTRCONFIG.ExpandPath(FILE_PATHS[49]); // quelle
        cmdss << "\" -t \"";
        cmdss << soundLSTPath; // ziel
        cmdss << "\"";

        std::string cmd = cmdss.str();
#ifdef _WIN32
        std::replace(cmd.begin(), cmd.end(), '/', '\\'); // Slash in Backslash verwandeln, sonst will "system" unter win nicht
#endif                                                   // _WIN32

        LOG.write(_("Starting Sound-Converter...\n"));
        if(system(cmd.c_str()) == -1)
            return false;

        // die konvertierte muss nicht extra geladen werden, da sie im override-ordner landet
    }

    // ggf original laden, hier das overriding benutzen wär ladezeitverschwendung
    if(!boost::filesystem::exists(soundLSTPath))
    {
        // existiert nicht
        if(!LoadFile(RTTRCONFIG.ExpandPath(FILE_PATHS[49]), NULL, true))
            return false;
    }

    std::vector<std::string> oggFiles = ListDir(RTTRCONFIG.ExpandPath(FILE_PATHS[50]), "ogg");

    unsigned i = 0;
    sng_lst.alloc(oggFiles.size());
    for(std::vector<std::string>::iterator it = oggFiles.begin(); it != oggFiles.end(); ++it)
    {
        libsiedler2::Archiv sng;

        LOG.write(_("Loading \"%s\": ")) % *it;
        unsigned startTime = VIDEODRIVER.GetTickCount();
        if(int ec = libsiedler2::Load(*it, sng))
        {
            LOG.write(_("failed: %1%\n")) % libsiedler2::getErrorString(ec);
            return false;
        }
        LOG.write(_("done in %ums\n")) % (VIDEODRIVER.GetTickCount() - startTime);

        sng_lst.set(i++, sng.release(0));
    }

    return true;
}

void Loader::LoadDummyGUIFiles()
{
    // Palettes
    libsiedler2::ArchivItem_Palette* palette = new libsiedler2::ArchivItem_Palette;
    files_["colors"].archiv.pushC(*palette);
    files_["pal5"].archiv.push(palette);
    // GUI elements
    libsiedler2::Archiv& resource = files_["resource"].archiv;
    resource.alloc(57);
    for(unsigned id = 4; id < 36; id++)
    {
        glArchivItem_Bitmap_RLE* bmp = new glArchivItem_Bitmap_RLE();
        libsiedler2::PixelBufferARGB buffer(1, 1);
        bmp->create(buffer);
        resource.set(id, bmp);
    }
    for(unsigned id = 36; id < 57; id++)
    {
        glArchivItem_Bitmap_Raw* bmp = new glArchivItem_Bitmap_Raw();
        libsiedler2::PixelBufferARGB buffer(1, 1);
        bmp->create(buffer);
        resource.set(id, bmp);
    }
    libsiedler2::Archiv& io = files_["io"].archiv;
    for(unsigned id = 0; id < 264; id++)
    {
        glArchivItem_Bitmap_Raw* bmp = new glArchivItem_Bitmap_Raw();
        libsiedler2::PixelBufferARGB buffer(1, 1);
        bmp->create(buffer);
        io.push(bmp);
    }
    // Fonts
    libsiedler2::Archiv& fonts = files_["outline_fonts"].archiv;
    fonts.alloc(3);
    libsiedler2::PixelBufferARGB buffer(15, 16);
    for(unsigned i = 0; i < 3; i++)
    {
        glArchivItem_Font* font = new glArchivItem_Font();
        const unsigned dx = 9 + i * 3;
        const unsigned dy = 10 + i * 3;
        font->setDx(dx);
        font->setDy(dy);
        font->alloc(255);
        for(unsigned id = 0x21; id < 255; id++)
        {
            glArchivItem_Bitmap_Player* bmp = new glArchivItem_Bitmap_Player();
            bmp->create(dx, dy, buffer, palette, 0);
            font->set(id, bmp);
        }
        fonts.set(i, font);
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
bool Loader::LoadFilesAtGame(bool isWinterGFX, const std::vector<bool>& nations)
{
    using namespace boost::assign; // Adds the vector += operator
    std::vector<unsigned> files;

    files += 26, 44, 45, 86, 92, // rom_bobs.lst, carrier.bob, jobs.bob, boat.lst, boot_z.lst
      58, 59, 60, 61, 62, 63,    // mis0bobs.lst, mis1bobs.lst, mis2bobs.lst, mis3bobs.lst, mis4bobs.lst, mis5bobs.lst
      35, 36, 37, 38;            // afr_icon.lst, jap_icon.lst, rom_icon.lst, vik_icon.lst

    for(unsigned char i = 0; i < NUM_NATIVE_NATS; ++i)
    {
        // ggf. Völker-Grafiken laden
        if((i < nations.size() && nations[i]) || (i == NAT_ROMANS && NAT_BABYLONIANS < nations.size() && nations[NAT_BABYLONIANS]))
            files += 27 + i + (isWinterGFX ? NUM_NATIVE_NATS : 0);
    }

    // Load files, but only once. If they are modified by overrides they will still be loaded again
    if(!LoadFilesFromArray(files, true))
        return false;

    std::string mapGFXFile = MAP_GFXSET_Z[boost::underlying_cast<uint8_t>(isWinterGFX)];
    if(!LoadFileOrDir(RTTRCONFIG.ExpandPath(FILE_PATHS[23]) + "/" + mapGFXFile + ".LST", true))
        return false;
    map_gfx = &GetInfoN(boost::algorithm::to_lower_copy(mapGFXFile));
    std::string texGFXFile = TEX_GFXSET[boost::underlying_cast<uint8_t>(isWinterGFX)];
    if(!LoadFileOrDir(RTTRCONFIG.ExpandPath(FILE_PATHS[20]) + "/" + texGFXFile + ".LBM", true))
        return false;

    if(NAT_BABYLONIANS < nations.size() && nations[NAT_BABYLONIANS]
       && !LoadFileOrDir(RTTRCONFIG.ExpandPath("<RTTR_RTTR>/LSTS/GAME/Babylonier"), true))
        return false;

    if(!LoadLsts(96)) // lade systemweite und persönliche lst files
        return false;

    isWinterGFX_ = isWinterGFX;

    for(unsigned nation = 0; nation < NUM_NATS; ++nation)
        nation_gfx[nation] = &GetInfoN(NATION_GFXSET_Z[boost::underlying_cast<uint8_t>(isWinterGFX)][nation]);

    return true;
}

void Loader::fillCaches()
{
    delete stp;
    stp = new glTexturePacker();

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
                                                                //Offsets to new job imgs
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

                glArchivItem_Bitmap_Raw* bitmap = new glArchivItem_Bitmap_Raw();
                bitmap->create(width, height, &buffer.front(), width, height, libsiedler2::FORMAT_PALETTED, palette);
                bitmap->setNx(image->getNx());
                bitmap->setNy(image->getNy());

                bmp.add(bitmap, true);
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
        deletePtr(stp);
}

/**
 *  Lädt Dateien von Addons.
 *
 *  @param[in] id die Addon ID
 *
 *  @return @p true bei Erfolg, @p false bei Fehler.
 */
bool Loader::LoadFilesFromAddon(const AddonId id)
{
    std::stringstream s;
    s << RTTRCONFIG.ExpandPath(FILE_PATHS[96]) << "/Addon_0x" << std::setw(8) << std::setfill('0') << std::hex << id << "/";

    return LoadFileOrDir(s.str(), false);
}

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

glArchivItem_Font* Loader::GetFontN(const std::string& file, unsigned nr)
{
    return dynamic_cast<glArchivItem_Font*>(files_[file].archiv[nr]);
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
    libsiedler2::ArchivItem_Text* archiv = dynamic_cast<libsiedler2::ArchivItem_Text*>(files_[file].archiv[nr]);
    return archiv ? archiv->getText() : "text missing";
}

libsiedler2::Archiv& Loader::GetInfoN(const std::string& file)
{
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

/**
 *  Extrahiert eine Textur aus den Daten.
 */
glArchivItem_Bitmap* Loader::ExtractTexture(const glArchivItem_Bitmap& srcImg, const Rect& rect)
{
    Extent texSize = rect.getSize();
    if(texSize.x == 0 && rect.right < srcImg.getWidth())
        texSize.x = srcImg.getWidth() - rect.right;
    if(texSize.y == 0 && rect.bottom < srcImg.getHeight())
        texSize.y = srcImg.getHeight() - rect.bottom;
    libsiedler2::PixelBufferPaletted buffer(texSize.x, texSize.y);

    if(int ec = srcImg.print(buffer, NULL, 0, 0, rect.left, rect.top))
        throw std::runtime_error(std::string("Error loading texture: ") + libsiedler2::getErrorString(ec));

    glArchivItem_Bitmap_Raw* bitmap = new glArchivItem_Bitmap_Raw();
    if(int ec = bitmap->create(buffer, srcImg.getPalette()))
    {
        delete bitmap;
        throw std::runtime_error(std::string("Error loading texture: ") + libsiedler2::getErrorString(ec));
    }
    return bitmap;
}

/**
 *  Extrahiert mehrere (animierte) Texturen aus den Daten.
 */
libsiedler2::Archiv* Loader::ExtractAnimatedTexture(const glArchivItem_Bitmap& srcImg, const Rect& rect, uint8_t start_index,
                                                    uint8_t color_count)
{
    Extent texSize = rect.getSize();
    if(texSize.x == 0 && rect.right < srcImg.getWidth())
        texSize.x = srcImg.getWidth() - rect.right;
    if(texSize.y == 0 && rect.bottom < srcImg.getHeight())
        texSize.y = srcImg.getHeight() - rect.bottom;
    libsiedler2::PixelBufferPaletted buffer(texSize.x, texSize.y);

    srcImg.print(buffer, NULL, 0, 0, rect.left, rect.top);

    boost::interprocess::unique_ptr<libsiedler2::Archiv, Deleter<libsiedler2::Archiv> > destination(new libsiedler2::Archiv());
    libsiedler2::ArchivItem_PaletteAnimation anim;
    anim.isActive = true;
    anim.moveUp = false;
    anim.firstClr = start_index;
    anim.lastClr = start_index + color_count - 1u;
    libsiedler2::ArchivItem_Palette* curPal = NULL;
    for(unsigned i = 0; i < color_count; ++i)
    {
        if(i == 0)
            curPal = srcImg.getPalette()->clone();
        else
            curPal = anim.apply(*curPal);
        boost::interprocess::unique_ptr<glArchivItem_Bitmap_Raw, Deleter<glArchivItem_Bitmap> > bitmap(new glArchivItem_Bitmap_Raw);

        bitmap->setPalette(curPal);

        if(int ec = bitmap->create(buffer))
            throw std::runtime_error("Error extracting animated texture: " + libsiedler2::getErrorString(ec));
        destination->push(bitmap.release());
    }
    return destination.release();
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
bool Loader::LoadArchiv(const std::string& pfad, const libsiedler2::ArchivItem_Palette* palette, libsiedler2::Archiv& archiv)
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

/**
 *  @brief Loads a file or directory into an archiv
 *
 *  @param filePath Path to file or directory
 *  @param palette Palette to use for possible graphic files
 *  @param to Archtive to write to
 */
bool Loader::LoadFile(const std::string& filePath, const libsiedler2::ArchivItem_Palette* palette, libsiedler2::Archiv& to)
{
    if(filePath.at(0) == '~')
        throw std::logic_error("You must use resolved pathes: " + filePath);

    if(!boost::filesystem::exists(filePath))
    {
        LOG.write(_("File or directory does not exist: %s\n")) % filePath;
        return false;
    }
    if(boost::filesystem::is_regular_file(filePath))
        return LoadArchiv(filePath, palette, to);
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
 *  @brief
 *
 *  @param pfad Path to file or directory
 *  @param palette Palette to use for possible graphic files
 *  @param isOriginal If this is set to true, the file is considered to be the base archiv so all possibly loaded overrides are
 * removed/overwritten first
 */
bool Loader::LoadFile(const std::string& pfad, const libsiedler2::ArchivItem_Palette* palette, bool isOriginal)
{
    std::string lowerPath = boost::algorithm::to_lower_copy(pfad);

    boost::filesystem::path filePath(lowerPath);
    boost::filesystem::path fileName = filePath.filename();
    std::string name = fileName.stem().string();

    FileEntry& entry = files_[name];
    bool isLoaded = !entry.archiv.empty();
    if(isLoaded && entry.hasOverrides && isOriginal)
    {
        // We are loading the original file which was already loaded but modified --> Clear it to reload
        entry.archiv.clear();
        isLoaded = false;
    }
    // If the file is already loaded and we are not loading a override -> exit
    if(isLoaded && isOriginal)
        return true;

    if(!isLoaded)
    {
        entry.hasOverrides = !isOriginal;
        return LoadFile(pfad, palette, entry.archiv);
    }

    // haben wir eine override file? dann nicht-leere items überschreiben
    libsiedler2::Archiv newEntries;
    if(!LoadFile(pfad, palette, newEntries))
        return false;

#ifndef NDEBUG
    LOG.write(_("Replacing entries of previously loaded file '%s'\n")) % name;
#endif // !NDEBUG

    libsiedler2::Archiv* existing = &GetInfoN(name);
    // *.bob archives have exactly 1 entry which is a 'folder' of the actual entries
    // An overwrite can be a (real) folder with those entries and we want to put them into that 'folder'
    // So we check if the new archiv is a folder or an archiv by checking if it contains only 1 BOB entry
    if(fileName.extension() == ".bob" && !(newEntries.size() == 1 && newEntries.get(0)->getBobType() == libsiedler2::BOBTYPE_BOB))
    {
        existing = dynamic_cast<libsiedler2::Archiv*>(existing->get(0));
        if(!existing)
        {
            LOG.write(_("Error while replacing a BOB file\n"));
            return false;
        }
    }

    if(newEntries.size() > existing->size())
        existing->alloc_inc(newEntries.size() - existing->size());

    for(unsigned i = 0; i < newEntries.size(); ++i)
    {
        if(newEntries[i])
        {
#ifndef NDEBUG
            LOG.write(_("Replacing entry %d with %s\n")) % i % newEntries[i]->getName();
#endif // !NDEBUG
            existing->set(i, newEntries.release(i));
        }
    }

    // Tell the system that we used overrides
    entry.hasOverrides = true;

    return true;
}
