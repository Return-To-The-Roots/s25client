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

#include "helpers/MultiArray.h"
#include "s25util/warningSuppression.h"
#include <array>
#include <string>

RTTR_IGNORE_DIAGNOSTIC("-Wunused-variable")

namespace s25 {
namespace folders {
    constexpr auto assetsAddons = "<RTTR_RTTR>/assets/addons";       // Addon specifc assets
    constexpr auto assetsBase = "<RTTR_RTTR>/assets/base";           // Assets introduced by rttr
    constexpr auto assetsNations = "<RTTR_RTTR>/assets/nations";     // Addon specific assets
    constexpr auto assetsOverrides = "<RTTR_RTTR>/assets/overrides"; // Assets overriding S2 files
    constexpr auto assetsUserOverrides = "<RTTR_USERDATA>/LSTS";     // User overrides for assets
    constexpr auto config = "<RTTR_CONFIG>";
    constexpr auto data = "<RTTR_GAME>/DATA"; // S2 game data
    constexpr auto driver = "<RTTR_DRIVER>";
    constexpr auto gamedata = "<RTTR_RTTR>/gamedata";   // Path to the gamedata
    constexpr auto languages = "<RTTR_RTTR>/languages"; // translation files
    constexpr auto loadScreens = "<RTTR_GAME>/GFX/PICS";
    constexpr auto loadScreensMissions = "<RTTR_GAME>/GFX/PICS/MISSION";
    constexpr auto logs = "<RTTR_USERDATA>/LOGS";
    constexpr auto mapsCampaign = "<RTTR_GAME>/DATA/MAPS";
    constexpr auto mapsContinents = "<RTTR_GAME>/DATA/MAPS2";
    constexpr auto mapsNew = "<RTTR_GAME>/DATA/MAPS4";
    constexpr auto mapsOld = "<RTTR_GAME>/DATA/MAPS3";
    constexpr auto mapsOther = "<RTTR_RTTR>/MAPS/OTHER";
    constexpr auto mapsOwn = "<RTTR_USERDATA>/WORLDS";
    constexpr auto mapsPlayed = "<RTTR_USERDATA>/MAPS"; // downloaded maps
    constexpr auto mapsRttr = "<RTTR_RTTR>/MAPS/NEW";   // maps added by RTTR
    constexpr auto mapsSea = "<RTTR_RTTR>/MAPS/SEA";    // seafaring maps
    constexpr auto mbob = "<RTTR_GAME>/DATA/MBOB";      // nation graphics
    constexpr auto music = "<RTTR_RTTR>/MUSIC";
    constexpr auto playlists = "<RTTR_USERDATA>/playlists";
    constexpr auto replays = "<RTTR_USERDATA>/REPLAYS";
    constexpr auto save = "<RTTR_USERDATA>/SAVE";
    constexpr auto screenshots = "<RTTR_USERDATA>/screenshots";
    constexpr auto sng = "<RTTR_RTTR>/MUSIC/SNG"; // downloaded background music files
    constexpr auto texte = "<RTTR_RTTR>/texte";
    constexpr auto textures = "<RTTR_GAME>/GFX/TEXTURES"; // Terrain textures
} // namespace folders
namespace files {
    constexpr auto splash = "<RTTR_RTTR>/assets/base/splash.bmp";
    constexpr auto soundOrig = "<RTTR_GAME>/DATA/SOUNDDAT/SOUND.LST"; // original sound.lst
    constexpr auto soundScript = "<RTTR_RTTR>/sound.scs";             // converter script
    constexpr auto defaultPlaylist = "<RTTR_RTTR>/MUSIC/S2_Standard.pll";
} // namespace files
namespace resources {
    constexpr auto boat = "<RTTR_GAME>/DATA/BOBS/BOAT.LST";
    constexpr auto boot_z = "<RTTR_GAME>/DATA/BOOT_Z.LST";
    constexpr auto carrier = "<RTTR_GAME>/DATA/BOBS/CARRIER.BOB";
    constexpr auto config = "<RTTR_CONFIG>/CONFIG.INI"; // main config file
    constexpr auto io = "<RTTR_GAME>/DATA/IO/IO.DAT";
    constexpr auto jobs = "<RTTR_GAME>/DATA/BOBS/JOBS.BOB";
    constexpr auto mis0bobs = "<RTTR_GAME>/DATA/MIS0BOBS.LST";
    constexpr auto mis1bobs = "<RTTR_GAME>/DATA/MIS1BOBS.LST";
    constexpr auto mis2bobs = "<RTTR_GAME>/DATA/MIS2BOBS.LST";
    constexpr auto mis3bobs = "<RTTR_GAME>/DATA/MIS3BOBS.LST";
    constexpr auto mis4bobs = "<RTTR_GAME>/DATA/MIS4BOBS.LST";
    constexpr auto mis5bobs = "<RTTR_GAME>/DATA/MIS5BOBS.LST";
    constexpr auto pal5 = "<RTTR_GAME>/GFX/PALETTE/PAL5.BBM";
    constexpr auto pal6 = "<RTTR_GAME>/GFX/PALETTE/PAL6.BBM";
    constexpr auto pal7 = "<RTTR_GAME>/GFX/PALETTE/PAL7.BBM";
    constexpr auto paletti0 = "<RTTR_GAME>/GFX/PALETTE/PALETTI0.BBM";
    constexpr auto paletti1 = "<RTTR_GAME>/GFX/PALETTE/PALETTI1.BBM";
    constexpr auto paletti8 = "<RTTR_GAME>/GFX/PALETTE/PALETTI8.BBM";
    constexpr auto resource = "<RTTR_GAME>/DATA/RESOURCE.DAT";
    constexpr auto rom_bobs = "<RTTR_GAME>/DATA/CBOB/ROM_BOBS.LST";
    constexpr auto setup013 = "<RTTR_GAME>/GFX/PICS/SETUP013.LBM"; // option backgrond
    constexpr auto setup015 = "<RTTR_GAME>/GFX/PICS/SETUP015.LBM"; // free play background
} // namespace resources
} // namespace s25

// TODO: Make this an array of ResourceId
const std::array<const std::string, 21> LOAD_SCREENS = {
  {"setup666", "setup667", "setup801", "setup802", "setup803", "setup804", "setup805",
   "setup806", "setup810", "setup811", "setup895", "setup896", "africa",   "austra",
   "europe",   "green",    "japan",    "namerica", "nasia",    "samerica", "sasia"}};

RTTR_POP_DIAGNOSTIC
