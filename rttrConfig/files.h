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
#ifndef FILES_H_INCLUDED
#define FILES_H_INCLUDED

#pragma once

#include <build_paths.h>

#include <string>

// Expected: RTTR_BINDIR, RTTR_DATADIR, RTTR_GAMEDIR, RTTR_LIBDIR, RTTR_DRIVERDIR
#if !(defined(RTTR_BINDIR) && defined(RTTR_DATADIR) && defined(RTTR_GAMEDIR) && defined(RTTR_LIBDIR) && defined(RTTR_DRIVERDIR))
#error "At least one of the RTTR_*DIR is undefined!"
#endif

#ifndef RTTRDIR
#define RTTRDIR RTTR_DATADIR "/RTTR"
#endif // !RTTR_DRIVERDIR

#ifndef RTTR_SETTINGSDIR
#if defined(_WIN32)
#define RTTR_SETTINGSDIR "~/Return To The Roots"
#elif defined(__APPLE__)
#define RTTR_SETTINGSDIR "~/Library/Application Support/Return To The Roots"
#else
#define RTTR_SETTINGSDIR "~/.s25rttr"
#endif
#endif // !RTTR_SETTINGSDIR

///////////////////////////////////////////////////////////////////////////////
// Konstanten
const char* const FILE_PATHS[] = {
  /*  0 */ RTTR_SETTINGSDIR "/CONFIG.INI",       // die Einstellungsdatei
  /*  1 */ RTTRDIR "",                           // unbenutzt
  /*  2 */ RTTR_GAMEDIR "",                      // unbenutzt
  /*  3 */ RTTR_GAMEDIR "",                      // unbenutzt
  /*  4 */ RTTRDIR "",                           // unbenutzt
  /*  5 */ RTTR_GAMEDIR "/GFX/PALETTE/PAL5.BBM", // die ganzen Paletten
  /*  6 */ RTTR_GAMEDIR "/GFX/PALETTE/PAL6.BBM",
  /*  7 */ RTTR_GAMEDIR "/GFX/PALETTE/PAL7.BBM",
  /*  8 */ RTTR_GAMEDIR "/GFX/PALETTE/PALETTI0.BBM",
  /*  9 */ RTTR_GAMEDIR "/GFX/PALETTE/PALETTI1.BBM",
  /* 10 */ RTTR_GAMEDIR "/GFX/PALETTE/PALETTI8.BBM",
  /* 11 */ RTTR_GAMEDIR "/DATA/RESOURCE.DAT",
  /* 12 */ RTTR_GAMEDIR "/DATA/IO/IO.DAT",
  /* 13 */ RTTR_GAMEDIR "",      // unbenutzt
  /* 14 */ RTTR_GAMEDIR "",      // unbenutzt
  /* 15 */ RTTRDIR "/languages", // die ganzen Sprachdateien
  /* 16 */ RTTRDIR "/RTTR.LST",
  /* 17 */ RTTRDIR "/COLORS.ACT",                 // Spezialpalette wegen Schriften usw
  /* 18 */ RTTRDIR "",                            // unbenutzt
  /* 19 */ RTTR_GAMEDIR "",                       // unbenutzt
  /* 20 */ RTTR_GAMEDIR "/GFX/TEXTURES/TEX5.LBM", // Grünland
  /* 21 */ RTTR_GAMEDIR "/GFX/TEXTURES/TEX6.LBM", // Ödland
  /* 22 */ RTTR_GAMEDIR "/GFX/TEXTURES/TEX7.LBM", // Winterwelt
  /* 23 */ RTTR_GAMEDIR "/DATA/MAP_0_Z.LST",      // Grünland
  /* 24 */ RTTR_GAMEDIR "/DATA/MAP_1_Z.LST",      // Ödland
  /* 25 */ RTTR_GAMEDIR "/DATA/MAP_2_Z.LST",      // Winterwelt
  /* 26 */ RTTR_GAMEDIR "/DATA/CBOB/ROM_BOBS.LST",
  /* 27 */ RTTR_GAMEDIR "/DATA/MBOB/AFR_Z.LST",  // Afrikaner
  /* 28 */ RTTR_GAMEDIR "/DATA/MBOB/JAP_Z.LST",  // Japaner
  /* 29 */ RTTR_GAMEDIR "/DATA/MBOB/ROM_Z.LST",  // Römer
  /* 30 */ RTTR_GAMEDIR "/DATA/MBOB/VIK_Z.LST",  // Wikinger
  /* 31 */ RTTR_GAMEDIR "/DATA/MBOB/WAFR_Z.LST", // Afrikaner Winter
  /* 32 */ RTTR_GAMEDIR "/DATA/MBOB/WJAP_Z.LST", // Japaner Winter
  /* 33 */ RTTR_GAMEDIR "/DATA/MBOB/WROM_Z.LST", // Römer Winter
  /* 34 */ RTTR_GAMEDIR "/DATA/MBOB/WVIK_Z.LST", // Wikinger Winter
  /* 35 */ RTTR_GAMEDIR "/DATA/MBOB/AFR_ICON.LST",
  /* 36 */ RTTR_GAMEDIR "/DATA/MBOB/JAP_ICON.LST",
  /* 37 */ RTTR_GAMEDIR "/DATA/MBOB/ROM_ICON.LST",
  /* 38 */ RTTR_GAMEDIR "/DATA/MBOB/VIK_ICON.LST",
  /* 39 */ RTTR_GAMEDIR "/DATA/MAPS3/",
  /* 40 */ RTTR_GAMEDIR "/DATA/MAPS4/",
  /* 41 */ RTTR_GAMEDIR "/WORLDS/",
  /* 42 */ RTTR_GAMEDIR "/DATA/MAPS2/",
  /* 43 */ RTTR_GAMEDIR "/DATA/MAPS/",
  /* 44 */ RTTR_GAMEDIR "/DATA/BOBS/CARRIER.BOB",
  /* 45 */ RTTR_GAMEDIR "/DATA/BOBS/JOBS.BOB",
  /* 46 */ RTTR_DRIVERDIR "/",                      // Treiberordner
  /* 47 */ RTTR_SETTINGSDIR "/LOGS/",               // Log-Ordner
  /* 48 */ RTTR_SETTINGSDIR "/MAPS/",               // die heruntergeladenen Karten
  /* 49 */ RTTR_GAMEDIR "/DATA/SOUNDDAT/SOUND.LST", // die originale sound.lst
  /* 50 */ RTTRDIR "/MUSIC/SNG",                    // die musik
  /* 51 */ RTTR_SETTINGSDIR "/REPLAYS/",            // Replayordner
  /* 52 */ RTTRDIR "/MAPS/NEW/",                    // unsere eigenen neuen Karten
  /* 53 */ RTTR_GAMEDIR "/DATA/SOUNDDAT/SNG/SNG_*.DAT",
  /* 54 */ RTTRDIR "",                         // unbenutzt
  /* 55 */ RTTR_SETTINGSDIR "/LSTS/SOUND.LST", // Die konvertierte sound.lst
  /* 56 */ RTTRDIR "/sound.scs",               // Das konvertier-script
  /* 57 */ RTTR_BINDIR "/RTTR/",               // Basispfad für den Soundconverter
  /* 58 */ RTTR_GAMEDIR "/DATA/MIS0BOBS.LST",
  /* 59 */ RTTR_GAMEDIR "/DATA/MIS1BOBS.LST",
  /* 60 */ RTTR_GAMEDIR "/DATA/MIS2BOBS.LST",
  /* 61 */ RTTR_GAMEDIR "/DATA/MIS3BOBS.LST",
  /* 62 */ RTTR_GAMEDIR "/DATA/MIS4BOBS.LST",
  /* 63 */ RTTR_GAMEDIR "/DATA/MIS5BOBS.LST",
  /* 64 */ RTTR_GAMEDIR "/GFX/PICS/SETUP666.LBM", // die Karten-Lade-Screens
  /* 65 */ RTTR_GAMEDIR "/GFX/PICS/SETUP667.LBM",
  /* 66 */ RTTR_GAMEDIR "/GFX/PICS/SETUP801.LBM",
  /* 67 */ RTTR_GAMEDIR "/GFX/PICS/SETUP802.LBM",
  /* 68 */ RTTR_GAMEDIR "/GFX/PICS/SETUP803.LBM",
  /* 69 */ RTTR_GAMEDIR "/GFX/PICS/SETUP804.LBM",
  /* 70 */ RTTR_GAMEDIR "/GFX/PICS/SETUP805.LBM",
  /* 71 */ RTTR_GAMEDIR "/GFX/PICS/SETUP806.LBM",
  /* 72 */ RTTR_GAMEDIR "/GFX/PICS/SETUP810.LBM",
  /* 73 */ RTTR_GAMEDIR "/GFX/PICS/SETUP811.LBM",
  /* 74 */ RTTR_GAMEDIR "/GFX/PICS/SETUP895.LBM",
  /* 75 */ RTTR_GAMEDIR "/GFX/PICS/SETUP896.LBM",
  /* 76 */ RTTR_GAMEDIR "/GFX/PICS/MISSION/AFRICA.LBM",
  /* 77 */ RTTR_GAMEDIR "/GFX/PICS/MISSION/AUSTRA.LBM",
  /* 78 */ RTTR_GAMEDIR "/GFX/PICS/MISSION/EUROPE.LBM",
  /* 79 */ RTTR_GAMEDIR "/GFX/PICS/MISSION/GREEN.LBM",
  /* 80 */ RTTR_GAMEDIR "/GFX/PICS/MISSION/JAPAN.LBM",
  /* 81 */ RTTR_GAMEDIR "/GFX/PICS/MISSION/NAMERICA.LBM",
  /* 82 */ RTTR_GAMEDIR "/GFX/PICS/MISSION/NASIA.LBM",
  /* 83 */ RTTR_GAMEDIR "/GFX/PICS/MISSION/SAMERICA.LBM",
  /* 84 */ RTTR_GAMEDIR "/GFX/PICS/MISSION/SASIA.LBM",
  /* 85 */ RTTR_SETTINGSDIR "/SAVE/", // Der Speicherordner
  /* 86 */ RTTR_GAMEDIR "/DATA/BOBS/BOAT.LST",
  /* 87 */ RTTRDIR "", // unbenutzt
  /* 88 */ RTTRDIR "/texte/",
  /* 89 */ RTTRDIR "", // unbenutzt
  /* 90 */ RTTRDIR "/MUSIC/",
  /* 91 */ RTTRDIR "/MAPS/OTHER/", // Andere Maps
  /* 92 */ RTTR_GAMEDIR "/DATA/BOOT_Z.LST",
  /* 93 */ RTTRDIR "/MAPS/SEA/",                  // Seefahrtkarten
  /* 94 */ RTTR_SETTINGSDIR,                      // Einstellungsordner
  /* 95 */ RTTRDIR "/LSTS/",                      // systemweite lstfiles (immer bei start geladen)
  /* 96 */ RTTRDIR "/LSTS/GAME/",                 // systemweite lstfiles (immer bei spielstart geladen)
  /* 97 */ RTTRDIR "",                            // unbenutzt
  /* 98 */ RTTR_SETTINGSDIR "/LSTS/",             // persönliche lstfiles (immer bei start geladen)
  /* 99 */ RTTR_SETTINGSDIR "/LSTS/GAME/",        // persönliche lstfiles (immer bei spielstart geladen)
  /*100 */ RTTR_SETTINGSDIR "",                   // unbenutzt
  /*101 */ RTTRDIR "",                            // unbenutzt
  /*102 */ RTTR_GAMEDIR "/GFX/PICS/SETUP013.LBM", // Optionen
  /*103 */ RTTR_GAMEDIR "/GFX/PICS/SETUP015.LBM", // Freies Spiel
  /*104 */ RTTRDIR "/splash.bmp"                  // Splash
};

const unsigned FILE_SPLASH_ID = 104;

const unsigned FILE_LOAD_IDS_COUNT = 21;
const std::string FILE_LOAD_IDS[] = {"setup666", "setup667", "setup801", "setup802", "setup803", "setup804", "setup805",
                                     "setup806", "setup810", "setup811", "setup895", "setup896", "africa",   "austra",
                                     "europe",   "green",    "japan",    "namerica", "nasia",    "samerica", "sasia"};

const unsigned GFXSET_COUNT = 3;
const unsigned NATION_COUNT = 5;

const std::string NATION_ICON_IDS[NATION_COUNT] = {"afr_icon", "jap_icon", "rom_icon", "vik_icon", "bab_icon"};

const std::string NATION_GFXSET_Z[GFXSET_COUNT][NATION_COUNT] = {{"afr_z", "jap_z", "rom_z", "vik_z", "bab_z"},
                                                                 {"afr_z", "jap_z", "rom_z", "vik_z", "bab_z"},
                                                                 {"wafr_z", "wjap_z", "wrom_z", "wvik_z", "wbab_z"}};

const std::string MAP_GFXSET_Z[GFXSET_COUNT] = {"map_0_z", "map_1_z", "map_2_z"};

const std::string TEX_GFXSET[GFXSET_COUNT] = {"tex5", "tex6", "tex7"};

#endif // FILES_H_INCLUDED
