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

#include "helpers/MultiArray.h"
#include "s25util/warningSuppression.h"
#include <array>
#include <string>

RTTR_IGNORE_DIAGNOSTIC("-Wunused-variable")

namespace s25 {
namespace folders {
    constexpr auto config = "<RTTR_CONFIG>";                   // Einstellungsordner
    constexpr auto data = "<RTTR_GAME>/DATA";                  // S2 game data
    constexpr auto driver = "<RTTR_DRIVER>";                   // Treiberordner
    constexpr auto gameLstsGlobal = "<RTTR_RTTR>/LSTS/GAME";   // systemweite lstfiles (immer bei spielstart geladen)
    constexpr auto gameLstsUser = "<RTTR_USERDATA>/LSTS/GAME"; // persönliche lstfiles (immer bei spielstart geladen)
    constexpr auto gamedata = "<RTTR_RTTR>/gamedata";          // Path to the gamedata
    constexpr auto languages = "<RTTR_RTTR>/languages";        // die ganzen Sprachdateien
    constexpr auto logs = "<RTTR_USERDATA>/LOGS";              // Log-Ordner
    constexpr auto lstsGlobal = "<RTTR_RTTR>/LSTS";            // systemweite lstfiles (immer bei start geladen)
    constexpr auto lstsUser = "<RTTR_USERDATA>/LSTS";          // persönliche lstfiles (immer bei start geladen)
    constexpr auto maps = "<RTTR_GAME>/DATA/MAPS";
    constexpr auto maps2 = "<RTTR_GAME>/DATA/MAPS2";
    constexpr auto maps3 = "<RTTR_GAME>/DATA/MAPS3";
    constexpr auto maps4 = "<RTTR_GAME>/DATA/MAPS4";
    constexpr auto mapsNew = "<RTTR_RTTR>/MAPS/NEW";  // unsere eigenen neuen Karten
    constexpr auto mapsUser = "<RTTR_USERDATA>/MAPS"; // die heruntergeladenen Karten
    constexpr auto music = "<RTTR_RTTR>/MUSIC";
    constexpr auto other = "<RTTR_RTTR>/MAPS/OTHER";            // Andere Maps
    constexpr auto replays = "<RTTR_USERDATA>/REPLAYS";         // Replayordner
    constexpr auto save = "<RTTR_USERDATA>/SAVE";               // Der Speicherordner
    constexpr auto screenshots = "<RTTR_USERDATA>/screenshots"; // Screenshots
    constexpr auto sea = "<RTTR_RTTR>/MAPS/SEA";                // Seefahrtkarten
    constexpr auto sng = "<RTTR_RTTR>/MUSIC/SNG";               // die musik
    constexpr auto texte = "<RTTR_RTTR>/texte";
    constexpr auto textures = "<RTTR_GAME>/GFX/TEXTURES"; // Terrain textures
    constexpr auto worlds = "<RTTR_USERDATA>/WORLDS";
} // namespace folders
namespace files {
    constexpr auto soundOrig = "<RTTR_GAME>/DATA/SOUNDDAT/SOUND.LST"; // die originale sound.lst
    constexpr auto soundScript = "<RTTR_RTTR>/sound.scs";             // Das konvertier-script
} // namespace files
namespace resources {
    constexpr auto afr_icon = "<RTTR_GAME>/DATA/MBOB/AFR_ICON.LST";
    constexpr auto afr_z = "<RTTR_GAME>/DATA/MBOB/AFR_Z.LST"; // Afrikaner
    constexpr auto africa = "<RTTR_GAME>/GFX/PICS/MISSION/AFRICA.LBM";
    constexpr auto austra = "<RTTR_GAME>/GFX/PICS/MISSION/AUSTRA.LBM";
    constexpr auto boat = "<RTTR_GAME>/DATA/BOBS/BOAT.LST";
    constexpr auto boot_z = "<RTTR_GAME>/DATA/BOOT_Z.LST";
    constexpr auto carrier = "<RTTR_GAME>/DATA/BOBS/CARRIER.BOB";
    constexpr auto colors = "<RTTR_RTTR>/COLORS.ACT";   // Spezialpalette wegen Schriften usw
    constexpr auto config = "<RTTR_CONFIG>/CONFIG.INI"; // die Einstellungsdatei
    constexpr auto europe = "<RTTR_GAME>/GFX/PICS/MISSION/EUROPE.LBM";
    constexpr auto green = "<RTTR_GAME>/GFX/PICS/MISSION/GREEN.LBM";
    constexpr auto io = "<RTTR_GAME>/DATA/IO/IO.DAT";
    constexpr auto jap_icon = "<RTTR_GAME>/DATA/MBOB/JAP_ICON.LST";
    constexpr auto jap_z = "<RTTR_GAME>/DATA/MBOB/JAP_Z.LST"; // Japaner
    constexpr auto japan = "<RTTR_GAME>/GFX/PICS/MISSION/JAPAN.LBM";
    constexpr auto jobs = "<RTTR_GAME>/DATA/BOBS/JOBS.BOB";
    constexpr auto mis0bobs = "<RTTR_GAME>/DATA/MIS0BOBS.LST";
    constexpr auto mis1bobs = "<RTTR_GAME>/DATA/MIS1BOBS.LST";
    constexpr auto mis2bobs = "<RTTR_GAME>/DATA/MIS2BOBS.LST";
    constexpr auto mis3bobs = "<RTTR_GAME>/DATA/MIS3BOBS.LST";
    constexpr auto mis4bobs = "<RTTR_GAME>/DATA/MIS4BOBS.LST";
    constexpr auto mis5bobs = "<RTTR_GAME>/DATA/MIS5BOBS.LST";
    constexpr auto namerica = "<RTTR_GAME>/GFX/PICS/MISSION/NAMERICA.LBM";
    constexpr auto nasia = "<RTTR_GAME>/GFX/PICS/MISSION/NASIA.LBM";
    constexpr auto pal5 = "<RTTR_GAME>/GFX/PALETTE/PAL5.BBM"; // die ganzen Paletten
    constexpr auto pal6 = "<RTTR_GAME>/GFX/PALETTE/PAL6.BBM";
    constexpr auto pal7 = "<RTTR_GAME>/GFX/PALETTE/PAL7.BBM";
    constexpr auto paletti0 = "<RTTR_GAME>/GFX/PALETTE/PALETTI0.BBM";
    constexpr auto paletti1 = "<RTTR_GAME>/GFX/PALETTE/PALETTI1.BBM";
    constexpr auto paletti8 = "<RTTR_GAME>/GFX/PALETTE/PALETTI8.BBM";
    constexpr auto resource = "<RTTR_GAME>/DATA/RESOURCE.DAT";
    constexpr auto rom_bobs = "<RTTR_GAME>/DATA/CBOB/ROM_BOBS.LST";
    constexpr auto rom_icon = "<RTTR_GAME>/DATA/MBOB/ROM_ICON.LST";
    constexpr auto rom_z = "<RTTR_GAME>/DATA/MBOB/ROM_Z.LST"; // Römer
    constexpr auto rttr = "<RTTR_RTTR>/RTTR.LST";
    constexpr auto samerica = "<RTTR_GAME>/GFX/PICS/MISSION/SAMERICA.LBM";
    constexpr auto sasia = "<RTTR_GAME>/GFX/PICS/MISSION/SASIA.LBM";
    constexpr auto setup013 = "<RTTR_GAME>/GFX/PICS/SETUP013.LBM"; // Optionen
    constexpr auto setup015 = "<RTTR_GAME>/GFX/PICS/SETUP015.LBM"; // Freies Spiel
    constexpr auto setup666 = "<RTTR_GAME>/GFX/PICS/SETUP666.LBM"; // die Karten-Lade-Screens
    constexpr auto setup667 = "<RTTR_GAME>/GFX/PICS/SETUP667.LBM";
    constexpr auto setup801 = "<RTTR_GAME>/GFX/PICS/SETUP801.LBM";
    constexpr auto setup802 = "<RTTR_GAME>/GFX/PICS/SETUP802.LBM";
    constexpr auto setup803 = "<RTTR_GAME>/GFX/PICS/SETUP803.LBM";
    constexpr auto setup804 = "<RTTR_GAME>/GFX/PICS/SETUP804.LBM";
    constexpr auto setup805 = "<RTTR_GAME>/GFX/PICS/SETUP805.LBM";
    constexpr auto setup806 = "<RTTR_GAME>/GFX/PICS/SETUP806.LBM";
    constexpr auto setup810 = "<RTTR_GAME>/GFX/PICS/SETUP810.LBM";
    constexpr auto setup811 = "<RTTR_GAME>/GFX/PICS/SETUP811.LBM";
    constexpr auto setup895 = "<RTTR_GAME>/GFX/PICS/SETUP895.LBM";
    constexpr auto setup896 = "<RTTR_GAME>/GFX/PICS/SETUP896.LBM";
    constexpr auto sngs = "<RTTR_GAME>/DATA/SOUNDDAT/SNG/SNG_*.DAT";
    constexpr auto sound = "<RTTR_USERDATA>/LSTS/SOUND.LST"; // Die konvertierte sound.lst
    constexpr auto vik_icon = "<RTTR_GAME>/DATA/MBOB/VIK_ICON.LST";
    constexpr auto vik_z = "<RTTR_GAME>/DATA/MBOB/VIK_Z.LST";   // Wikinger
    constexpr auto wafr_z = "<RTTR_GAME>/DATA/MBOB/WAFR_Z.LST"; // Afrikaner Winter
    constexpr auto wjap_z = "<RTTR_GAME>/DATA/MBOB/WJAP_Z.LST"; // Japaner Winter
    constexpr auto wrom_z = "<RTTR_GAME>/DATA/MBOB/WROM_Z.LST"; // Römer Winter
    constexpr auto wvik_z = "<RTTR_GAME>/DATA/MBOB/WVIK_Z.LST"; // Wikinger Winter
} // namespace resources
} // namespace s25

RTTR_POP_DIAGNOSTIC

///////////////////////////////////////////////////////////////////////////////
// Konstanten
const std::array<const char*, 104> SUPPRESS_UNUSED FILE_PATHS = {{
  /*  0 */ "<RTTR_CONFIG>/CONFIG.INI",         // die Einstellungsdatei
  /*  1 */ "<RTTR_RTTR>/gamedata",             // Path to the gamedata
  /*  2 */ "",                                 // unused
  /*  3 */ "",                                 // unused
  /*  4 */ "",                                 // unused
  /*  5 */ "<RTTR_GAME>/GFX/PALETTE/PAL5.BBM", // die ganzen Paletten
  /*  6 */ "<RTTR_GAME>/GFX/PALETTE/PAL6.BBM",
  /*  7 */ "<RTTR_GAME>/GFX/PALETTE/PAL7.BBM",
  /*  8 */ "<RTTR_GAME>/GFX/PALETTE/PALETTI0.BBM",
  /*  9 */ "<RTTR_GAME>/GFX/PALETTE/PALETTI1.BBM",
  /* 10 */ "<RTTR_GAME>/GFX/PALETTE/PALETTI8.BBM",
  /* 11 */ "<RTTR_GAME>/DATA/RESOURCE.DAT",
  /* 12 */ "<RTTR_GAME>/DATA/IO/IO.DAT",
  /* 13 */ "",                      // unused
  /* 14 */ "",                      // unused
  /* 15 */ "<RTTR_RTTR>/languages", // die ganzen Sprachdateien
  /* 16 */ "<RTTR_RTTR>/RTTR.LST",
  /* 17 */ "<RTTR_RTTR>/COLORS.ACT",   // Spezialpalette wegen Schriften usw
  /* 18 */ "",                         // unused
  /* 19 */ "",                         // unused
  /* 20 */ "<RTTR_GAME>/GFX/TEXTURES", // Terrain textures
  /* 21 */ "",
  /* 22 */ "",
  /* 23 */ "<RTTR_GAME>/DATA", // S2 game data
  /* 24 */ "",
  /* 25 */ "",
  /* 26 */ "<RTTR_GAME>/DATA/CBOB/ROM_BOBS.LST",
  /* 27 */ "<RTTR_GAME>/DATA/MBOB/AFR_Z.LST",  // Afrikaner
  /* 28 */ "<RTTR_GAME>/DATA/MBOB/JAP_Z.LST",  // Japaner
  /* 29 */ "<RTTR_GAME>/DATA/MBOB/ROM_Z.LST",  // Römer
  /* 30 */ "<RTTR_GAME>/DATA/MBOB/VIK_Z.LST",  // Wikinger
  /* 31 */ "<RTTR_GAME>/DATA/MBOB/WAFR_Z.LST", // Afrikaner Winter
  /* 32 */ "<RTTR_GAME>/DATA/MBOB/WJAP_Z.LST", // Japaner Winter
  /* 33 */ "<RTTR_GAME>/DATA/MBOB/WROM_Z.LST", // Römer Winter
  /* 34 */ "<RTTR_GAME>/DATA/MBOB/WVIK_Z.LST", // Wikinger Winter
  /* 35 */ "<RTTR_GAME>/DATA/MBOB/AFR_ICON.LST",
  /* 36 */ "<RTTR_GAME>/DATA/MBOB/JAP_ICON.LST",
  /* 37 */ "<RTTR_GAME>/DATA/MBOB/ROM_ICON.LST",
  /* 38 */ "<RTTR_GAME>/DATA/MBOB/VIK_ICON.LST",
  /* 39 */ "<RTTR_GAME>/DATA/MAPS3",
  /* 40 */ "<RTTR_GAME>/DATA/MAPS4",
  /* 41 */ "<RTTR_USERDATA>/WORLDS",
  /* 42 */ "<RTTR_GAME>/DATA/MAPS2",
  /* 43 */ "<RTTR_GAME>/DATA/MAPS",
  /* 44 */ "<RTTR_GAME>/DATA/BOBS/CARRIER.BOB",
  /* 45 */ "<RTTR_GAME>/DATA/BOBS/JOBS.BOB",
  /* 46 */ "<RTTR_DRIVER>",                       // Treiberordner
  /* 47 */ "<RTTR_USERDATA>/LOGS",                // Log-Ordner
  /* 48 */ "<RTTR_USERDATA>/MAPS",                // die heruntergeladenen Karten
  /* 49 */ "<RTTR_GAME>/DATA/SOUNDDAT/SOUND.LST", // die originale sound.lst
  /* 50 */ "<RTTR_RTTR>/MUSIC/SNG",               // die musik
  /* 51 */ "<RTTR_USERDATA>/REPLAYS",             // Replayordner
  /* 52 */ "<RTTR_RTTR>/MAPS/NEW",                // unsere eigenen neuen Karten
  /* 53 */ "<RTTR_GAME>/DATA/SOUNDDAT/SNG/SNG_*.DAT",
  /* 54 */ "",                               // unused
  /* 55 */ "<RTTR_USERDATA>/LSTS/SOUND.LST", // Die konvertierte sound.lst
  /* 56 */ "<RTTR_RTTR>/sound.scs",          // Das konvertier-script
  /* 57 */ "<RTTR_EXTRA_BIN>",               // Basispfad für den Soundconverter
  /* 58 */ "<RTTR_GAME>/DATA/MIS0BOBS.LST",
  /* 59 */ "<RTTR_GAME>/DATA/MIS1BOBS.LST",
  /* 60 */ "<RTTR_GAME>/DATA/MIS2BOBS.LST",
  /* 61 */ "<RTTR_GAME>/DATA/MIS3BOBS.LST",
  /* 62 */ "<RTTR_GAME>/DATA/MIS4BOBS.LST",
  /* 63 */ "<RTTR_GAME>/DATA/MIS5BOBS.LST",
  /* 64 */ "<RTTR_GAME>/GFX/PICS/SETUP666.LBM", // die Karten-Lade-Screens
  /* 65 */ "<RTTR_GAME>/GFX/PICS/SETUP667.LBM",
  /* 66 */ "<RTTR_GAME>/GFX/PICS/SETUP801.LBM",
  /* 67 */ "<RTTR_GAME>/GFX/PICS/SETUP802.LBM",
  /* 68 */ "<RTTR_GAME>/GFX/PICS/SETUP803.LBM",
  /* 69 */ "<RTTR_GAME>/GFX/PICS/SETUP804.LBM",
  /* 70 */ "<RTTR_GAME>/GFX/PICS/SETUP805.LBM",
  /* 71 */ "<RTTR_GAME>/GFX/PICS/SETUP806.LBM",
  /* 72 */ "<RTTR_GAME>/GFX/PICS/SETUP810.LBM",
  /* 73 */ "<RTTR_GAME>/GFX/PICS/SETUP811.LBM",
  /* 74 */ "<RTTR_GAME>/GFX/PICS/SETUP895.LBM",
  /* 75 */ "<RTTR_GAME>/GFX/PICS/SETUP896.LBM",
  /* 76 */ "<RTTR_GAME>/GFX/PICS/MISSION/AFRICA.LBM",
  /* 77 */ "<RTTR_GAME>/GFX/PICS/MISSION/AUSTRA.LBM",
  /* 78 */ "<RTTR_GAME>/GFX/PICS/MISSION/EUROPE.LBM",
  /* 79 */ "<RTTR_GAME>/GFX/PICS/MISSION/GREEN.LBM",
  /* 80 */ "<RTTR_GAME>/GFX/PICS/MISSION/JAPAN.LBM",
  /* 81 */ "<RTTR_GAME>/GFX/PICS/MISSION/NAMERICA.LBM",
  /* 82 */ "<RTTR_GAME>/GFX/PICS/MISSION/NASIA.LBM",
  /* 83 */ "<RTTR_GAME>/GFX/PICS/MISSION/SAMERICA.LBM",
  /* 84 */ "<RTTR_GAME>/GFX/PICS/MISSION/SASIA.LBM",
  /* 85 */ "<RTTR_USERDATA>/SAVE", // Der Speicherordner
  /* 86 */ "<RTTR_GAME>/DATA/BOBS/BOAT.LST",
  /* 87 */ "", // unused
  /* 88 */ "<RTTR_RTTR>/texte",
  /* 89 */ "", // unused
  /* 90 */ "<RTTR_RTTR>/MUSIC",
  /* 91 */ "<RTTR_RTTR>/MAPS/OTHER", // Andere Maps
  /* 92 */ "<RTTR_GAME>/DATA/BOOT_Z.LST",
  /* 93 */ "<RTTR_RTTR>/MAPS/SEA",              // Seefahrtkarten
  /* 94 */ "<RTTR_CONFIG>",                     // Einstellungsordner
  /* 95 */ "<RTTR_RTTR>/LSTS",                  // systemweite lstfiles (immer bei start geladen)
  /* 96 */ "<RTTR_RTTR>/LSTS/GAME",             // systemweite lstfiles (immer bei spielstart geladen)
  /* 97 */ "",                                  // unused
  /* 98 */ "<RTTR_USERDATA>/LSTS",              // persönliche lstfiles (immer bei start geladen)
  /* 99 */ "<RTTR_USERDATA>/LSTS/GAME",         // persönliche lstfiles (immer bei spielstart geladen)
  /*100 */ "<RTTR_USERDATA>/screenshots",       // Screenshots
  /*101 */ "",                                  // unused
  /*102 */ "<RTTR_GAME>/GFX/PICS/SETUP013.LBM", // Optionen
  /*103 */ "<RTTR_GAME>/GFX/PICS/SETUP015.LBM"  // Freies Spiel
}};

constexpr unsigned FILE_SPLASH_ID = 104;

const std::array<const std::string, 21> SUPPRESS_UNUSED LOAD_SCREENS = {
  {"setup666", "setup667", "setup801", "setup802", "setup803", "setup804", "setup805", "setup806", "setup810", "setup811", "setup895",
   "setup896", "africa",   "austra",   "europe",   "green",    "japan",    "namerica", "nasia",    "samerica", "sasia"}};

constexpr unsigned NUM_GFXSETS = 3;
constexpr unsigned NUM_NATIONS = 5;

const std::array<const std::string, NUM_NATIONS> SUPPRESS_UNUSED NATION_ICON_IDS = {
  {"afr_icon", "jap_icon", "rom_icon", "vik_icon", "bab_icon"}};

const helpers::MultiArray<const std::string, 2, NUM_NATIONS> SUPPRESS_UNUSED NATION_GFXSET_Z = {
  {{"afr_z", "jap_z", "rom_z", "vik_z", "bab_z"}, {"wafr_z", "wjap_z", "wrom_z", "wvik_z", "wbab_z"}}};

const std::array<const std::string, NUM_GFXSETS> SUPPRESS_UNUSED MAP_GFXSET_Z = {{"MAP_0_Z", "MAP_1_Z", "MAP_2_Z"}};

const std::array<const std::string, NUM_GFXSETS> SUPPRESS_UNUSED TEX_GFXSET = {{"TEX5", "TEX6", "TEX7"}};

#endif // FILES_H_INCLUDED
