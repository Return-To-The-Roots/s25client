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

#include "helpers/SimpleMultiArray.h"
#include <boost/array.hpp>
#include <string>

///////////////////////////////////////////////////////////////////////////////
// Konstanten
const boost::array<const char*, 104> SUPPRESS_UNUSED FILE_PATHS = {{
  /*  0 */ "<RTTR_CONFIG>/CONFIG.INI",         // die Einstellungsdatei
  /*  1 */ "",                                 // unused
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
  /* 17 */ "<RTTR_RTTR>/COLORS.ACT",            // Spezialpalette wegen Schriften usw
  /* 18 */ "",                                  // unused
  /* 19 */ "",                                  // unused
  /* 20 */ "<RTTR_GAME>/GFX/TEXTURES/TEX5.LBM", // Grünland
  /* 21 */ "<RTTR_GAME>/GFX/TEXTURES/TEX6.LBM", // Ödland
  /* 22 */ "<RTTR_GAME>/GFX/TEXTURES/TEX7.LBM", // Winterwelt
  /* 23 */ "<RTTR_GAME>/DATA/MAP_0_Z.LST",      // Grünland
  /* 24 */ "<RTTR_GAME>/DATA/MAP_1_Z.LST",      // Ödland
  /* 25 */ "<RTTR_GAME>/DATA/MAP_2_Z.LST",      // Winterwelt
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
  /* 57 */ "<RTTR_BIN>/RTTR",                // Basispfad für den Soundconverter
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
  /*100 */ "",                                  // unused
  /*101 */ "",                                  // unused
  /*102 */ "<RTTR_GAME>/GFX/PICS/SETUP013.LBM", // Optionen
  /*103 */ "<RTTR_GAME>/GFX/PICS/SETUP015.LBM"  // Freies Spiel
}};

BOOST_CONSTEXPR_OR_CONST unsigned FILE_SPLASH_ID = 104;

BOOST_CONSTEXPR_OR_CONST unsigned NUM_FILE_LOAD_IDS = 21;
const boost::array<const std::string, NUM_FILE_LOAD_IDS> SUPPRESS_UNUSED FILE_LOAD_IDS = {
  {"setup666", "setup667", "setup801", "setup802", "setup803", "setup804", "setup805", "setup806", "setup810", "setup811", "setup895",
   "setup896", "africa",   "austra",   "europe",   "green",    "japan",    "namerica", "nasia",    "samerica", "sasia"}};

BOOST_CONSTEXPR_OR_CONST unsigned NUM_GFXSETS = 3;
BOOST_CONSTEXPR_OR_CONST unsigned NUM_NATIONS = 5;

const boost::array<const std::string, NUM_NATIONS> SUPPRESS_UNUSED NATION_ICON_IDS = {
  {"afr_icon", "jap_icon", "rom_icon", "vik_icon", "bab_icon"}};

const helpers::SimpleMultiArray<const std::string, NUM_GFXSETS, NUM_NATIONS> SUPPRESS_UNUSED NATION_GFXSET_Z = {
  {{"afr_z", "jap_z", "rom_z", "vik_z", "bab_z"},
   {"afr_z", "jap_z", "rom_z", "vik_z", "bab_z"},
   {"wafr_z", "wjap_z", "wrom_z", "wvik_z", "wbab_z"}}};

const boost::array<const std::string, NUM_GFXSETS> SUPPRESS_UNUSED MAP_GFXSET_Z = {
  {"map_0_z", "map_1_z", "map_2_z"}};

const boost::array<const std::string, NUM_GFXSETS> SUPPRESS_UNUSED TEX_GFXSET = {
  {"tex5", "tex6", "tex7"}};

#endif // FILES_H_INCLUDED
