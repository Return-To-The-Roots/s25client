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

///////////////////////////////////////////////////////////////////////////////

#include "defines.h"

#include "Loader.h"
#include "files.h"

#include "Settings.h"

#include "drivers/VideoDriverWrapper.h"
#include "drivers/AudioDriverWrapper.h"
#include "Log.h"

#include "GameWorld.h"
#include "ListDir.h"
#include "fileFuncs.h"

#include "ogl/glSmartBitmap.h"
#include "ogl/glArchivItem_Bitmap_Raw.h"
#include "ogl/glAllocator.h"
#include "gameData/JobConsts.h"
#include "gameData/TerrainData.h"

#include "../libsiedler2/src/types.h"
#include "../libsiedler2/src/prototypen.h"

#include <boost/filesystem.hpp>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <stdexcept>

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p Loader.
 *
 *  @author FloSoft
 */
Loader::Loader(void) : lastgfx(0xFF), stp(NULL)
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Destruktor von @p Loader.
 *
 *  @author FloSoft
 */
Loader::~Loader(void)
{
    delete stp;
    ClearTerrainTextures();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Lädt alle allgemeinen Dateien.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler.
 *
 *  @author FloSoft
 *  @author OLiver
 */
bool Loader::LoadFilesAtStart(void)
{
    const unsigned int files_count = 7 + 1 + 2 + 2 + 21;

    const unsigned int files[] =
    {
        5, 6, 7, 8, 9, 10, 17, // Paletten:     pal5.bbm, pal6.bbm, pal7.bbm, paletti0.bbm, paletti1.bbm, paletti8.bbm, colors.act
        FILE_SPLASH_ID,        // Splashscreen: splash.bmp
        11, 12,                // Menüdateien:  resource.dat, io.dat
        102, 103,              // Hintergründe: setup013.lbm, setup015.lbm
        64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84 // Die ganzen Spielladescreens.
    };

    if(!LoadFilesFromArray(files_count, files))
        return false;

    if(!LoadSounds())
        return false;

    if(!LoadLsts(95)) // lade systemweite und persönliche lst files
        return false;

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  @brief
 *
 *  @author FloSoft
 */
bool Loader::LoadFileOrDir(const std::string& file, const unsigned int file_id, bool load_always)
{
    if(file.at(0) == '~')
        throw std::logic_error("You must use resolved pathes: " + file);

    if(!boost::filesystem::exists(file))
    {
        LOG.lprintf(_("File or directory does not exist: %s\n"), file.c_str());
        return false;
    }
    // is the entry a directory?
    if(boost::filesystem::is_directory(file))
    {
        // yes, load all files in the directory
        unsigned int ladezeit = VIDEODRIVER.GetTickCount();

        LOG.lprintf(_("Loading LST,BOB,IDX,BMP,TXT,GER,ENG files from \"%s\"\n"), GetFilePath(file).c_str());

        std::list<std::string> lst;
        ListDir(file + "*.lst", true, NULL, NULL, &lst);
        ListDir(file + "*.bob", true, NULL, NULL, &lst);
        ListDir(file + "*.idx", true, NULL, NULL, &lst);
        ListDir(file + "*.bmp", true, NULL, NULL, &lst);
        ListDir(file + "*.txt", true, NULL, NULL, &lst);
        ListDir(file + "*.ger", true, NULL, NULL, &lst);
        ListDir(file + "*.eng", true, NULL, NULL, &lst);

        for(std::list<std::string>::iterator i = lst.begin(); i != lst.end(); ++i)
        {
            if(!LoadFile( i->c_str(), GetPaletteN("pal5"), load_always ) )
                return false;
        }
        LOG.lprintf(_("finished in %ums\n"), VIDEODRIVER.GetTickCount() - ladezeit);
    }
    else
    {
        // no, only single file specified
        if(!LoadFile(file.c_str(), GetPaletteN("pal5"), load_always ) )
            return false;

        // ggf Splash anzeigen
        if(file_id == FILE_SPLASH_ID)
        {
            glArchivItem_Bitmap* image = GetImageN("splash", 0);
            image->setFilter(GL_LINEAR);
            image->Draw(0, 0, VIDEODRIVER.GetScreenWidth(), VIDEODRIVER.GetScreenHeight(), 0, 0, 0, 0);
            VIDEODRIVER.SwapBuffers();
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Lädt Dateien aus FILE_PATHS bzw aus dem Verzeichnis.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler.
 *
 *  @author FloSoft
 */
bool Loader::LoadFilesFromArray(const unsigned int files_count, const unsigned int* files, bool load_always)
{
    // load the files or directorys
    for(unsigned int i = 0; i < files_count; ++i)
    {
        if(files[i] == 0xFFFFFFFF)
            continue;

        std::string filePath = GetFilePath(FILE_PATHS[ files[i] ]);
        if(!LoadFileOrDir(filePath, files[i], load_always))
        {
            LOG.lprintf(_("Failed to load %s\n"), filePath.c_str());
            return false;
        }
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Lädt die "override" lst-files aus den systemweiten und persönlichen verzeichnissen
 *
 *  @return @p true bei Erfolg, @p false bei Fehler.
 *
 *  @author FloSoft
 */
bool Loader::LoadLsts(unsigned int dir)
{
    // systemweite lsts laden
    unsigned int files_count = 0;
    unsigned int* files = NULL;

    if(GetFilePath(FILE_PATHS[dir]) == GetFilePath(FILE_PATHS[dir + 3]))
    {
        files_count = 1;
        files = new unsigned int[1];
        files[0] = dir;
    }
    else
    {
        files_count = 2;
        files = new unsigned int[2];
        files[0] = dir;
        files[1] = dir + 3;
    }

    if(!LoadFilesFromArray(files_count, files) )
    {
        delete[] files;
        return false;
    }

    delete[] files;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Lädt alle Sounds.
 *
 *  @return liefert true bei Erfolg, false bei Fehler
 *
 *  @author FloSoft
 */
bool Loader::LoadSounds(void)
{
    std::string soundLSTPath = GetFilePath(FILE_PATHS[55]);
    // ist die konvertierte sound.lst vorhanden?
    if(!boost::filesystem::exists(soundLSTPath))
    {
        // nein, dann konvertieren

        std::stringstream cmdss;
        cmdss << GetFilePath(FILE_PATHS[57]); // pfad zum sound-converter hinzufügen

        // name anhängen
#ifdef _WIN32
        cmdss << "\\sound-convert.exe";
#else
        cmdss << "/sound-convert";
#endif

        // parameter anhängen
        cmdss << " -s \"";
        cmdss << GetFilePath(FILE_PATHS[56]); // script
        cmdss << "\" -f \"";
        cmdss << GetFilePath(FILE_PATHS[49]); // quelle
        cmdss << "\" -t \"";
        cmdss << GetFilePath(FILE_PATHS[55]); // ziel
        cmdss << "\"";

        std::string cmd = cmdss.str();
#ifdef _WIN32
        std::replace(cmd.begin(), cmd.end(), '/', '\\'); // Slash in Backslash verwandeln, sonst will "system" unter win nicht
#endif // _WIN32

        LOG.lprintf(_("Starting Sound-Converter ..."));
        if(system(cmd.c_str()) == -1)
            return false;

        // die konvertierte muss nicht extra geladen werden, da sie im override-ordner landet
    }

    // ggf original laden, hier das overriding benutzen wär ladezeitverschwendung
    if(!boost::filesystem::exists(soundLSTPath))
    {
        // existiert nicht
        if(!LoadFile(GetFilePath(FILE_PATHS[49])))
            return false;
    }

    std::list<std::string> liste;
    ListDir(GetFilePath(FILE_PATHS[50]), false, NULL, NULL, &liste);

    unsigned int i = 0;
    sng_lst.alloc(liste.size());
    for(std::list<std::string>::iterator it = liste.begin(); it != liste.end(); ++it)
    {
        libsiedler2::ArchivInfo sng;

        LOG.lprintf(_("Loading \"%s\": "), it->c_str());
        if(libsiedler2::loader::LoadSND(it->c_str(), sng) != 0 )
        {
            LOG.lprintf(_("failed\n"));
            return false;
        }
        LOG.lprintf(_("finished\n"));

        sng_lst.setC(i++, *sng.get(0));
    }

    // Siedler I MIDI-Musik
    //sng_lst.pushC(sound_lst.get(0));

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  sortiert einen string nach Startzahl, Namen oder Länge (in dieser Reihenfolge).
 *  Wird für das Sortieren der Dateien benutzt.
 *
 *  @author FloSoft
 */
bool Loader::SortFilesHelper(const std::string& lhs, const std::string& rhs)
{
    int a, b;

    std::stringstream aa;
    aa << bfs::path(lhs).filename().string();
    std::stringstream bb;
    bb << bfs::path(rhs).filename().string();

    if( !(aa >> a) || !(bb >> b) )
    {
        for( std::string::const_iterator lit = lhs.begin(), rit = rhs.begin(); lit != lhs.end() && rit != rhs.end(); ++lit, ++rit )
            if( tolower( *lit ) < tolower( *rit ) )
                return true;
            else if( tolower( *lit ) > tolower( *rit ) )
                return false;
        if( lhs.size() < rhs.size() )
            return true;
    }
    else
    {
        if(a < b)
            return true;
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  zerlegt einen String in Einzelteile
 *  Wird für das richtige Laden der Dateien benutzt.
 *
 *  @author FloSoft
 */
std::vector<std::string> Loader::ExplodeString(std::string const& line, const char delim, const unsigned int max)
{
    std::istringstream in(line);
    std::vector<std::string> result;
    std::string token;

    unsigned int len = 0;
    while(std::getline(in, token, delim) && result.size() < max - 1)
    {
        len += token.size() + 1;
        result.push_back(token);
    }

    if(len < in.str().length())
        result.push_back(in.str().substr(len));

    return result;
}


///////////////////////////////////////////////////////////////////////////////
/**
 *  Lädt die Settings.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler.
 *
 *  @author FloSoft
 */
bool Loader::LoadSettings()
{
    const unsigned int files_count = 1;

    const unsigned int files[files_count] =
    {
        0   // config.ini
    };

    if(!LoadFilesFromArray(files_count, files))
        return false;

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Speichert die Settings.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler.
 *
 *  @author FloSoft
 */
bool Loader::SaveSettings()
{
    std::string file = GetFilePath(FILE_PATHS[0]);

    LOG.lprintf(_("Writing \"%s\": "), file.c_str());
    fflush(stdout);

    if(libsiedler2::Write(file.c_str(), files_.find("config")->second) != 0)
        return false;

    using namespace boost::filesystem;
    permissions(file, owner_read | owner_write);

    LOG.lprintf(_("finished\n"));

    return true;
}

glSmartBitmap Loader::animal_cache[SPEC_COUNT][6][ANIMAL_MAX_ANIMATION_STEPS + 1] = {{{glSmartBitmap()}}};

// building_cache[nation][type][skeleton?]
glSmartBitmap Loader::building_cache[NAT_COUNT][BUILDING_TYPES_COUNT][2] = {{{glSmartBitmap()}}};

// flag_cache[nation][type][animation]
glSmartBitmap Loader::flag_cache[NAT_COUNT][3][8] = {{{glSmartBitmap()}}};

glSmartBitmap Loader::building_flag_cache[8] = {glSmartBitmap()};

// 0 - 7 animation, 8 & 9 growing, 11 - 13 falling, 14 fallen
glSmartBitmap Loader::tree_cache[9][15] = {{glSmartBitmap()}};

// Bobs from jobs.bob: bob_jobs_cache[nation][job][direction][animation]
glSmartBitmap Loader::bob_jobs_cache[NAT_COUNT][JOB_TYPES_COUNT + 1][6][8];

// Granit - zwei Typen, sechs größen
glSmartBitmap Loader::granite_cache[2][6];

// Felder - 2 Typen, vier Größen
glSmartBitmap Loader::grainfield_cache[2][4];

// carrier_cache[ware][direction][animation_step][fat]
glSmartBitmap Loader::carrier_cache[WARE_TYPES_COUNT][6][8][2];

// boundary_stone_cache[nation]
glSmartBitmap Loader::boundary_stone_cache[NAT_COUNT];

// boat_cache[direction][animation_step]
glSmartBitmap Loader::boat_cache[6][8];

// donkey_cache[direction][animation_step]
glSmartBitmap Loader::donkey_cache[6][8];

// gateway animation
glSmartBitmap Loader::gateway_cache[5];

///////////////////////////////////////////////////////////////////////////////
/**
 *  Lädt die Spieldateien.
 *
 *  @param[in] gfxset  Das GFX-Set
 *  @param[in] nations Array der zu ladenden Nationen.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler.
 *
 *  @author OLiver
 */
bool Loader::LoadFilesAtGame(unsigned char gfxset, bool* nations)
{
    assert(gfxset <= 2);

    const unsigned int files_count = NATIVE_NAT_COUNT + 5 + 6 + 4 + 1 + 1;

    unsigned int files[files_count] =
    {
        0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, // ?afr_z.lst, ?jap_z.lst, ?rom_z.lst, ?vik_z.lst
        26, 44, 45, 86, 92,                             // rom_bobs.lst, carrier.bob, jobs.bob, boat.lst, boot_z.lst
        58, 59, 60, 61, 62, 63,                         // mis0bobs.lst, mis1bobs.lst, mis2bobs.lst, mis3bobs.lst, mis4bobs.lst, mis5bobs.lst
        35, 36, 37, 38,                                 // afr_icon.lst, jap_icon.lst, rom_icon.lst, vik_icon.lst
        23u + gfxset,                                    // map_?_z.lst
        20u + gfxset                                     // tex?.lbm
    };

    for(unsigned char i = 0; i < NATIVE_NAT_COUNT; ++i)
    {
        // ggf. Völker-Grafiken laden
        if(nations[i] || ((i == 2) && (nations[4])))
            files[i] = 27 + i + (gfxset == 2) * NATIVE_NAT_COUNT;
    }

    // dateien ggf nur einmal laden - qx: wozu? performance is hier echt egal -> fixing bug #1085693
    if (!LoadFilesFromArray(files_count, files, true))
    {
        lastgfx = 0xFF;
        return false;
    }

    if ((nations[4]) && !LoadFileOrDir(GetFilePath(RTTRDIR "/LSTS/GAME/Babylonier/"), 0, true))
    {
        lastgfx = 0xFF;
        return false;
    }

    if(!LoadLsts(96)) // lade systemweite und persönliche lst files
    {
        lastgfx = 0xFF;
        return false;
    }

    lastgfx = gfxset;



    for (unsigned int nation = 0; nation < NAT_COUNT; ++nation)
    {
        nation_gfx[nation] = &(this->files_[NATION_GFXSET_Z[lastgfx][nation]]);
    }

    map_gfx = &(this->files_[MAP_GFXSET_Z[lastgfx]]);
    tex_gfx = &(this->files_[TEX_GFXSET[lastgfx]]);

    return true;
}

void Loader::fillCaches()
{
    delete stp;
    stp = new glSmartTexturePacker();

// Animals
    for (unsigned species = 0; species < SPEC_COUNT; ++species)
    {
        for (unsigned dir = 0; dir < 6; ++dir)
        {
            for (unsigned ani_step = 0; ani_step < ANIMALCONSTS[species].animation_steps; ++ani_step)
            {
                glSmartBitmap& bmp = animal_cache[species][dir][ani_step];

                bmp.reset();

                bmp.add(GetMapImageN(ANIMALCONSTS[species].walking_id + ANIMALCONSTS[species].animation_steps * ( (dir + 3) % 6) + ani_step));

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

        if (ANIMALCONSTS[species].dead_id)
        {
            bmp.add(GetMapImageN(ANIMALCONSTS[species].dead_id));

            if (ANIMALCONSTS[species].shadow_dead_id)
            {
                bmp.addShadow(GetMapImageN(ANIMALCONSTS[species].shadow_dead_id));
            }

            stp->add(bmp);
        }
    }

    glArchivItem_Bob* bob_jobs = GetBobN("jobs");

    for (unsigned nation = 0; nation < NAT_COUNT; ++nation)
    {
// BUILDINGS
        for (unsigned type = 0; type < BUILDING_TYPES_COUNT; ++type)
        {
            glSmartBitmap& bmp = building_cache[nation][type][0];
            glSmartBitmap& skel = building_cache[nation][type][1];

            bmp.reset();
            skel.reset();

            if (type == BLD_CHARBURNER)
            {
                unsigned id = nation * 8;

                bmp.add(GetImageN("charburner", id + ((lastgfx == LT_WINTERWORLD) ? 6 : 1)));
                bmp.addShadow(GetImageN("charburner", id + 2));

                skel.add(GetImageN("charburner", id + 3));
                skel.addShadow(GetImageN("charburner", id + 4));
            }
            else
            {
                bmp.add(GetNationImageN(nation, 250 + 5 * type));
                bmp.addShadow(GetNationImageN(nation, 250 + 5 * type + 1));
                skel.add(GetNationImageN(nation, 250 + 5 * type + 2));
                skel.addShadow(GetNationImageN(nation, 250 + 5 * type + 3));
            }

            stp->add(bmp);
            stp->add(skel);
        }

// FLAGS
        for (unsigned type = 0; type < 3; ++type)
        {
            for (unsigned ani_step = 0; ani_step < 8; ++ani_step)
            {
                // Flaggentyp berücksichtigen
                int nr = ani_step + 100 + 20 * type;

                glSmartBitmap& bmp = flag_cache[nation][type][ani_step];

                bmp.reset();

                bmp.add(static_cast<glArchivItem_Bitmap_Player*>(GetNationImageN(nation, nr)));
                bmp.addShadow(GetNationImageN(nation, nr + 10));

                stp->add(bmp);
            }
        }

// Bobs from jobs.bob. Job = JOB_TYPES_COUNT is used for fat carriers. See below.
        for (unsigned job = 0; job < JOB_TYPES_COUNT + 1; ++job)
        {
            for (unsigned dir = 0; dir < 6; ++dir)
            {
                for (unsigned ani_step = 0; ani_step < 8; ++ani_step)
                {
                    bool fat;
                    unsigned id;
                    unsigned short overlayOffset = 96;

                    glSmartBitmap& bmp = bob_jobs_cache[nation][job][dir][ani_step];

                    bmp.reset();

                    if (job == JOB_TYPES_COUNT) // used for fat carrier, so that we do not need an additional sub-array
                    {
                        fat = true;
                        id = 0;
                    }
                    else
                    {
                        id = JOB_CONSTS[job].jobs_bob_id;
                        fat = JOB_CONSTS[job].fat;

                        if ((job == JOB_SCOUT) || ((job >= JOB_PRIVATE) && (job <= JOB_GENERAL)))
                        {
                            if (nation < NATIVE_NAT_COUNT)
                            {
                                id += NATION_RTTR_TO_S2[nation] * 6;
                            }
                            else if (nation == NAT_BABYLONIANS)
                            {
                                // replace babylonians by romans
                                id += NATION_RTTR_TO_S2[NAT_ROMANS] * 6;
                                /*
                                                                //Offsets to new job imgs
                                                                overlayOffset = (job == JOB_SCOUT) ? 1740 : 1655;

                                                                //8 Frames * 6 Directions * 6 Types
                                                                overlayOffset += (nation - NATIVE_NAT_COUNT) * (8 * 6 * 6);
                                */
                            }
                        }
                    }

                    unsigned int good = id * 96 + ani_step * 12 + ( (dir + 3) % 6 ) + fat * 6;
                    unsigned int body = fat * 48 + ( (dir + 3) % 6 ) * 8 + ani_step;

                    if (bob_jobs->getLink(good) == 92)
                    {
                        good -= fat * 6;
                        body -= fat * 48;
                    }

                    bmp.add(dynamic_cast<glArchivItem_Bitmap_Player*>(bob_jobs->get(body)));
                    bmp.add(dynamic_cast<glArchivItem_Bitmap_Player*>(bob_jobs->get(overlayOffset + bob_jobs->getLink(good))));
                    bmp.addShadow(GetMapImageN(900 + ( (dir + 3) % 6 ) * 8 + ani_step));

                    stp->add(bmp);
                }
            }
        }

        glSmartBitmap& bmp = boundary_stone_cache[nation];

        bmp.reset();

        bmp.add(dynamic_cast<glArchivItem_Bitmap_Player*>(GetNationImageN(nation, 0)));
        bmp.addShadow(GetNationImageN(nation, 1));

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
    for (unsigned type = 0; type < 9; ++type)
    {
        for (unsigned ani_step = 0; ani_step < 15; ++ani_step)
        {
            glSmartBitmap& bmp = tree_cache[type][ani_step];

            bmp.reset();

            bmp.add(GetMapImageN(200 + type * 15 + ani_step));
            bmp.addShadow(GetMapImageN(350 + type * 15 + ani_step));

            stp->add(bmp);
        }
    }

// Granite
    for (unsigned type = 0; type < 2; ++type)
    {
        for (unsigned size = 0; size < 6; ++size)
        {
            glSmartBitmap& bmp = granite_cache[type][size];

            bmp.reset();

            bmp.add(GetMapImageN(516 + type * 6 + size));
            bmp.addShadow(GetMapImageN(616 + type * 6 + size));

            stp->add(bmp);
        }
    }

// Grainfields
    for (unsigned type = 0; type < 2; ++type)
    {
        for (unsigned size = 0; size < 4; ++size)
        {
            glSmartBitmap& bmp = grainfield_cache[type][size];

            bmp.reset();

            bmp.add(GetMapImageN(532 + type * 5 + size));
            bmp.addShadow(GetMapImageN(632 + type * 5 + size));

            stp->add(bmp);
        }
    }

// Donkeys
    for (unsigned dir = 0; dir < 6; ++dir)
    {
        for (unsigned ani_step = 0; ani_step < 8; ++ani_step)
        {
            glSmartBitmap& bmp = donkey_cache[dir][ani_step];

            bmp.reset();

            bmp.add(GetMapImageN(2000 + ((dir + 3) % 6) * 8 + ani_step));
            bmp.addShadow(GetMapImageN(2048 + dir % 3));

            stp->add(bmp);
        }
    }

// Boats
    for (unsigned dir = 0; dir < 6; ++dir)
    {
        for (unsigned ani_step = 0; ani_step < 8; ++ani_step)
        {
            glSmartBitmap& bmp = boat_cache[dir][ani_step];

            bmp.reset();

            bmp.add(dynamic_cast<glArchivItem_Bitmap_Player*>(GetImageN("boat", ((dir + 3) % 6) * 8 + ani_step)));
            bmp.addShadow(dynamic_cast<glArchivItem_Bitmap*>(GetMapImageN(2048 + dir % 3)));

            stp->add(bmp);
        }
    }

// carrier_cache[ware][direction][animation_step][fat]
    glArchivItem_Bob* bob_carrier = GetBobN("carrier");

    for (unsigned ware = 0; ware < WARE_TYPES_COUNT; ++ware)
    {
        for (unsigned dir = 0; dir < 6; ++dir)
        {
            for (unsigned ani_step = 0; ani_step < 8; ++ani_step)
            {
                for (unsigned fat = 0; fat < 2; ++fat)
                {
                    unsigned id;
                    glSmartBitmap& bmp = carrier_cache[ware][dir][ani_step][fat];

                    bmp.reset();

                    if (ware == GD_SHIELDJAPANESE)
                    {
                        id = GD_SHIELDROMANS;
                    }
                    else
                    {
                        id = ware;
                    }

                    unsigned int good = id * 96 + ani_step * 12 + ( (dir + 3) % 6 ) + fat * 6;
                    unsigned int body = fat * 48 + ( (dir + 3) % 6 ) * 8 + ani_step;

                    /*if(bob_jobs->getLink(good) == 92)
                    {
                        good -= fat*6;
                        body -= fat*48;
                    }*/

                    bmp.add(dynamic_cast<glArchivItem_Bitmap_Player*>(bob_carrier->get(body)));
                    bmp.add(dynamic_cast<glArchivItem_Bitmap_Player*>(bob_carrier->get(96 + bob_carrier->getLink(good))));
                    bmp.addShadow(GetMapImageN(900 + ( (dir + 3) % 6 ) * 8 + ani_step));

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

        if ((image) && (shadow) && (palette))
        {
            unsigned short width = image->getWidth();
            unsigned short height = image->getHeight();

            unsigned char* buffer = new unsigned char[width * height];

            memset(buffer, 254, width * height);

            image->print(buffer, width, height, libsiedler2::FORMAT_PALETTED, palette, 0, 0, 0, 0, width, height);

            for(unsigned char i = 0; i < color_count; ++i)
            {
                glSmartBitmap& bmp = gateway_cache[i + 1];

                bmp.reset();

                for(unsigned int x = 0; x < width; ++x)
                {
                    for(unsigned int y = 0; y < height; ++y)
                    {
                        if(buffer[y * width + x] >= start_index && buffer[y * width + x] < start_index + color_count)
                        {
                            if(++buffer[y * width + x] >= start_index + color_count)
                                buffer[y * width + x] = start_index;
                        }
                    }
                }

                glArchivItem_Bitmap_Raw* bitmap = new glArchivItem_Bitmap_Raw();
                bitmap->create(width, height, buffer, width, height, libsiedler2::FORMAT_PALETTED, palette);
                bitmap->setNx(image->getNx());
                bitmap->setNy(image->getNy());

                bmp.add(bitmap);
                bmp.addShadow(shadow);

                stp->add(bmp);
            }

            delete[] buffer;
        }
        else
        {
            for(unsigned char i = 0; i < color_count; ++i)
            {
                glSmartBitmap& bmp = gateway_cache[i + 1];

                bmp.reset();
            }
        }
    }

    if (SETTINGS.video.shared_textures)
    {
        // generate mega texture
        stp->pack();
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Lädt Dateien von Addons.
 *
 *  @param[in] id die Addon ID
 *
 *  @return @p true bei Erfolg, @p false bei Fehler.
 *
 *  @author FloSoft
 */
bool Loader::LoadFilesFromAddon(const AddonId id)
{
    std::stringstream s;
    s << GetFilePath(FILE_PATHS[96]) << "Addon_0x" << std::setw(8) << std::setfill('0') << std::hex << id << "/";

    return LoadFileOrDir(s.str(), 96, true);
}

void Loader::ClearTerrainTextures()
{
    for(std::map<TerrainType, glArchivItem_Bitmap*>::iterator it = terrainTextures.begin(); it != terrainTextures.end(); ++it)
        delete it->second;
    for(std::map<TerrainType, libsiedler2::ArchivInfo*>::iterator it = terrainTexturesAnim.begin(); it != terrainTexturesAnim.end(); ++it)
        delete it->second;
    terrainTextures.clear();
    terrainTexturesAnim.clear();
    borders.clear();
    roads.clear();
    roads_points.clear();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  zerschneidet die Terraintexturen.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler.
 *
 *  @author OLiver
 */
bool Loader::CreateTerrainTextures(void)
{
    assert(lastgfx <= 2);
    ClearTerrainTextures();

    // Ränder
    Rect rec_raender[5] =
    {
        Rect(192, 176, 64, 16), // Schnee
        Rect(192, 192, 64, 16), // Berg
        Rect(192, 208, 64, 16), // Wste
        Rect(192, 224, 64, 16), // Wiese
        Rect(192, 240, 64, 16) // Wasser
    };

    // Wege
    Rect rec_roads[8] =
    {
        Rect(192, 0, 50, 16),
        Rect(192, 16, 50, 16),
        Rect(192, 32, 50, 16),
        Rect(192, 160, 50, 16),

        Rect(242, 0, 50, 16),
        Rect(242, 16, 50, 16),
        Rect(242, 32, 50, 16),
        Rect(242, 160, 50, 16),
    };

    bool waterLoaded = false;

    for(unsigned char i=0; i<TT_COUNT; ++i)
    {
        TerrainType t = TerrainType(i);
        if(TerrainData::IsWater(t))
        {
            // All water uses the same texture, so load only once
            if(waterLoaded)
                continue;
            t = TT_WATER;
            waterLoaded = true;
        }
        if(TerrainData::IsAnimated(t))
            terrainTexturesAnim[t] = ExtractAnimatedTexture(TerrainData::GetPosInTexture(t), TerrainData::GetFrameCount(t), TerrainData::GetStartColor(t));
        else
            terrainTextures[t] = ExtractTexture(TerrainData::GetPosInTexture(t));
    }

    // die 5 Ränder
    for(unsigned char i = 0; i < 5; ++i)
        borders.push(ExtractTexture(rec_raender[i]));

    // Wege
    for(unsigned char i = 0; i < 4; ++i)
    {
        roads.push(ExtractTexture(rec_roads[i]));
        roads_points.push(ExtractTexture(rec_roads[4 + i]));
    }

    return true;
}

glArchivItem_Bitmap& Loader::GetTerrainTexture(TerrainType t, unsigned animationFrame/* = 0*/)
{
    if(TerrainData::IsAnimated(t))
    {
        if(TerrainData::IsWater(t))
        {
            // All water uses the same texture
            t = TT_WATER;
        }
        libsiedler2::ArchivInfo* archive = terrainTexturesAnim[t];
        if(!archive)
            throw std::runtime_error("Invalid terrain texture requested");
        return *dynamic_cast<glArchivItem_Bitmap*>(archive->get(animationFrame));
    }else
    {
        glArchivItem_Bitmap* bmp = terrainTextures[t];
        if(!bmp)
            throw std::runtime_error("Invalid terrain texture requested");
        return *bmp;
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Extrahiert eine Textur aus den Daten.
 *
 *  @author OLiver
 */
glArchivItem_Bitmap_Raw* Loader::ExtractTexture(const Rect& rect)
{
    libsiedler2::ArchivItem_Palette* palette = GetTexPaletteN(1);
    glArchivItem_Bitmap* image = GetTexImageN(0);

    unsigned short width = rect.right - rect.left;
    unsigned short height = rect.bottom - rect.top;

    std::vector<unsigned char> buffer(width * height, libsiedler2::TRANSPARENT_INDEX);

    image->print(&buffer.front(), width, height, libsiedler2::FORMAT_PALETTED, palette, 0, 0, rect.left, rect.top, width, height);
    for(std::vector<unsigned char>::iterator it = buffer.begin(); it != buffer.end(); ++it)
    {
        if(*it == 0)
            *it = libsiedler2::TRANSPARENT_INDEX;
    }

    glArchivItem_Bitmap_Raw* bitmap = new glArchivItem_Bitmap_Raw();
    bitmap->create(width, height, &buffer.front(), width, height, libsiedler2::FORMAT_PALETTED, palette);
    bitmap->setPalette(palette);
    bitmap->setFormat(libsiedler2::FORMAT_PALETTED);
    return bitmap;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Extrahiert mehrere (animierte) Texturen aus den Daten.
 *
 *  @author OLiver
 */
libsiedler2::ArchivInfo* Loader::ExtractAnimatedTexture(const Rect& rect, unsigned char color_count, unsigned char start_index)
{
    libsiedler2::ArchivItem_Palette* palette = GetTexPaletteN(1);
    glArchivItem_Bitmap* image = GetTexImageN(0);

    unsigned short width = rect.right - rect.left;
    unsigned short height = rect.bottom - rect.top;

    // Mit Startindex (also irgendeiner Farbe) füllen, um transparente Pixel und damit schwarze Punke am Rand zu verhindern
    std::vector<unsigned char> buffer(width * height, start_index);

    image->print(&buffer.front(), width, height, libsiedler2::FORMAT_PALETTED, palette, 0, 0, rect.left, rect.top, width, height);

    glArchivItem_Bitmap_Raw bitmap;
    bitmap.setPalette(palette);
    bitmap.setFormat(libsiedler2::FORMAT_PALETTED);

    libsiedler2::ArchivInfo* destination = new libsiedler2::ArchivInfo();
    for(unsigned char i = 0; i < color_count; ++i)
    {
        for(std::vector<unsigned char>::iterator it = buffer.begin(); it != buffer.end(); ++it)
        {
            if(*it >= start_index && *it < start_index + color_count)
            {
                if(++*it >= start_index + color_count)
                    *it = start_index;
            }
        }

        bitmap.create(width, height, &buffer.front(), width, height, libsiedler2::FORMAT_PALETTED, palette);

        destination->pushC(bitmap);
    }
    return destination;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  @brief Lädt eine Datei in ein ArchivInfo.
 *
 *  @param[in] pfad    Der Dateipfad
 *  @param[in] palette (falls benötigt) die Palette.
 *  @param[in] archiv  Das Zielarchivinfo.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler.
 *
 *  @author FloSoft
 */
bool Loader::LoadArchiv(const std::string& pfad, const libsiedler2::ArchivItem_Palette* palette, libsiedler2::ArchivInfo& archiv)
{
    unsigned int ladezeit = VIDEODRIVER.GetTickCount();

    std::string file = GetFilePath(pfad);

    LOG.lprintf(_("Loading \"%s\": "), file.c_str());
    fflush(stdout);

    if(libsiedler2::Load(file.c_str(), archiv, palette) != 0)
    {
        LOG.lprintf(_("failed\n"));
        return false;
    }

    LOG.lprintf(_("done in %ums\n"), VIDEODRIVER.GetTickCount() - ladezeit);

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  @brief
 *
 *  @author FloSoft
 */
bool Loader::LoadFile(const std::string& filePath, const libsiedler2::ArchivItem_Palette* palette, libsiedler2::ArchivInfo& to)
{
    if(filePath.at(0) == '~')
        throw std::logic_error("You must use resolved pathes: " + filePath);

    if(!boost::filesystem::exists(filePath))
    {
        LOG.lprintf(_("File or directory does not exist: %s\n"), filePath.c_str());
        return false;
    }
    if(boost::filesystem::is_regular_file(filePath))
        return LoadArchiv(filePath, palette, to);
    if(!boost::filesystem::is_directory(filePath))
    {
        LOG.lprintf(_("Could not determine type of path %s\n"), filePath.c_str());
        return false;
    }

    LOG.lprintf(_("Loading directory %s\n"), GetFilePath(filePath).c_str());
    std::list<std::string> lst;
    ListDir(filePath + "/*.bmp", false, NULL, NULL, &lst);
    ListDir(filePath + "/*.txt", false, NULL, NULL, &lst);
    ListDir(filePath + "/*.ger", false, NULL, NULL, &lst);
    ListDir(filePath + "/*.eng", false, NULL, NULL, &lst);
    ListDir(filePath + "/*.fon", true, NULL, NULL, &lst);
    ListDir(filePath + "/*.empty", false, NULL, NULL, &lst);

    lst.sort(SortFilesHelper);

    std::vector<unsigned char> buffer(1000 * 1000 * 4);
    for(std::list<std::string>::iterator itFile = lst.begin(); itFile != lst.end(); ++itFile)
    {
        // read file number, to set the index correctly
        std::string filename = bfs::path(*itFile).filename().string();
        std::stringstream nrs;
        int nr = -1;
        nrs << filename;
        if(! (nrs >> nr) )
            nr = -1;

        // Dateiname zerlegen
        std::vector<std::string> wf = ExplodeString(*itFile, '.');

        libsiedler2::BOBTYPES bobtype = libsiedler2::BOBTYPE_BITMAP_RAW;
        short nx = 0;
        short ny = 0;
        unsigned char dx = 0;
        unsigned char dy = 0;
        libsiedler2::ArchivItem* item = NULL;

        // Common
        for(std::vector<std::string>::iterator it = wf.begin(); it != wf.end(); ++it)
        {
            if(*it == "rle")
                bobtype = libsiedler2::BOBTYPE_BITMAP_RLE;
            else if(*it == "player")
                bobtype = libsiedler2::BOBTYPE_BITMAP_PLAYER;
            else if(*it == "shadow")
                bobtype = libsiedler2::BOBTYPE_BITMAP_SHADOW;

            else if(it->substr(0, 2) == "nx")
                nx = atoi(it->substr(2).c_str());
            else if(it->substr(0, 2) == "ny")
                ny = atoi(it->substr(2).c_str());
            else if(it->substr(0, 2) == "dx")
                dx = atoi(it->substr(2).c_str());
            else if(it->substr(0, 2) == "dy")
                dy = atoi(it->substr(2).c_str());
        }

        if( wf.back() == "empty" ) // Placeholder
        {
            LOG.lprintf(_("Skipping %s\n"), itFile->c_str());
            to.alloc_inc(1);
            continue;
        }else if( wf.back() == "bmp" ) // Bitmap
        {
            libsiedler2::ArchivInfo temp;
            if(!LoadArchiv( *itFile, palette, temp ) )
                return false;

            // Nun Daten abhängig der Typen erstellen, nur erstes Element wird bei Bitmaps konvertiert

            glArchivItem_Bitmap* in = dynamic_cast<glArchivItem_Bitmap*>(temp.get(0));
            glArchivItem_Bitmap* out = dynamic_cast<glArchivItem_Bitmap*>(GlAllocator().create(bobtype));

            if(!out)
            {
                LOG.lprintf("unbekannter bobtype: %d\n", bobtype);
                return false;
            }

            out->setName(itFile->c_str());
            out->setNx(nx);
            out->setNy(ny);

            std::fill(buffer.begin(), buffer.end(), 0);
            in->print(&buffer.front(), 1000, 1000, libsiedler2::FORMAT_RGBA, palette);

            switch(bobtype)
            {
                case libsiedler2::BOBTYPE_BITMAP_RLE:
                case libsiedler2::BOBTYPE_BITMAP_SHADOW:
                {
                    out->create(in->getWidth(), in->getHeight(), &buffer.front(), 1000, 1000, libsiedler2::FORMAT_RGBA, palette);
                } break;
                case libsiedler2::BOBTYPE_BITMAP_PLAYER:
                {
                    dynamic_cast<glArchivItem_Bitmap_Player*>(out)->create(in->getWidth(), in->getHeight(), &buffer.front(), 1000, 1000, libsiedler2::FORMAT_RGBA, palette, 128);
                } break;
                case libsiedler2::BOBTYPE_BITMAP_RAW:
                    break; // Nothing?
                default:
                    throw std::logic_error("Invalid Bitmap type");
            }

            item = out;
        }else if( (wf.back() == "bbm") || (wf.back() == "act") ) // Palettes
        {
            libsiedler2::ArchivInfo temp;
            if(!LoadArchiv( *itFile, palette, temp ) )
                return false;
            item = GlAllocator().clone(*temp.get(0));
        }else if( wf.back() == "fon" ) // Font
        {
            glArchivItem_Font* font = new glArchivItem_Font();
            font->setName(itFile->c_str());
            font->setDx(dx);
            font->setDy(dy);

            if(!LoadFile(*itFile, palette, *font))
                return false;

            item = font;
        }

        if(item)
        {
            // had the filename a number? then set it to the corresponding item.
            if(nr >= 0)
            {
                if(nr >= (int)to.size())
                    to.alloc_inc(nr - to.size() + 1);
                to.set(nr, item);
            }
            else
                to.push(item);
        }
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  @brief
 *
 *  @author FloSoft
 */
bool Loader::LoadFile(const std::string& pfad, const libsiedler2::ArchivItem_Palette* palette, bool load_always)
{
    std::string lowerPath = pfad;
    std::transform( lowerPath.begin(), lowerPath.end(), lowerPath.begin(), tolower );

    boost::filesystem::path filePath(lowerPath);
    boost::filesystem::path fileName = filePath.filename();
    std::string name = fileName.stem().string();

    // bereits geladen und wir wollen kein nochmaliges laden
    if(!load_always && files_[name].size() != 0)
        return true;

    if(files_[name].size() == 0)
        return LoadFile(pfad, palette, files_[name]);

    // haben wir eine override file? dann nicht-leere items überschreiben
    libsiedler2::ArchivInfo newEntries;
    if(!LoadFile(pfad, palette, newEntries))
        return false;

    LOG.lprintf(_("Replacing entries of previously loaded file '%s'\n"), name.c_str());

    libsiedler2::ArchivInfo* existing = &files_.find(name)->second;
    if(fileName.extension() == ".bob")
    {
        existing = dynamic_cast<libsiedler2::ArchivInfo*>(existing->get(0));
        if(!existing)
        {
            LOG.lprintf(_("Error while replacing a BOB file\n"));
            return false;
        }
    }

    if(newEntries.size() > existing->size())
        existing->alloc_inc(newEntries.size() - existing->size());

    for(unsigned int i = 0; i < newEntries.size(); ++i)
    {
        if(newEntries.get(i))
        {
            LOG.lprintf(_("Replacing entry %d with %s\n"), i, newEntries.get(i)->getName().c_str());
            existing->setC(i, *newEntries.get(i));
        }
    }

    return true;
}
