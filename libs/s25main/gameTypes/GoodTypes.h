// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "helpers/MaxEnumValue.h"
#include "mygettext/mygettext.h"
#include <cstdint>
#include <map>
#include <string>

// Warentypen
enum class GoodType : uint8_t
{
    /*  0 */ Beer,           // Bier
    /*  1 */ Tongs,          // Zange
    /*  2 */ Hammer,         // Hammer
    /*  3 */ Axe,            // Axt
    /*  4 */ Saw,            // Säge
    /*  5 */ PickAxe,        // Spitzhacke
    /*  6 */ Shovel,         // Schaufel
    /*  7 */ Crucible,       // Schmelztiegel
    /*  8 */ RodAndLine,     // Angel
    /*  9 */ Scythe,         // Sense
    /* 10 */ WaterEmpty,     // Wasser
    /* 11 */ Water,          // Wasser
    /* 12 */ Cleaver,        // Beil
    /* 13 */ Rollingpin,     // Nudelholz
    /* 14 */ Bow,            // Bogen
    /* 15 */ Boat,           // Boot
    /* 16 */ Sword,          // Schwert
    /* 17 */ Iron,           // Eisen
    /* 18 */ Flour,          // Mehl
    /* 19 */ Fish,           // Fisch
    /* 20 */ Bread,          // Brot
    /* 21 */ ShieldRomans,   // Schild
    /* 22 */ Wood,           // Holz
    /* 23 */ Boards,         // Bretter
    /* 24 */ Stones,         // Steine
    /* 25 */ ShieldVikings,  // Schild
    /* 26 */ ShieldAfricans, // Schild
    /* 27 */ Grain,          // Getreide
    /* 28 */ Coins,          // Mnzen
    /* 29 */ Gold,           // Gold
    /* 30 */ IronOre,        // Eisenerz
    /* 31 */ Coal,           // Kohle
    /* 32 */ Meat,           // Fleisch
    /* 33 */ Ham,            // Schinken ( Schwein )
    /* 34 */ ShieldJapanese, // Schild
    /* 35 */ Grapes,
    /* 36 */ Wine,
    /* 37 */ Nothing // Nothing. Is not counted as a good. TODO: Remove
};
constexpr auto maxEnumValue(GoodType)
{
    return GoodType::Wine;
}

/// List of all tools (correspond to buttons at IO:140-163)
enum class Tool
{
    Tongs,
    Hammer,
    Axe,
    Saw,
    PickAxe,
    Shovel,
    Crucible,
    RodAndLine,
    Scythe,
    Cleaver,
    Rollingpin,
    Bow
};
constexpr auto maxEnumValue(Tool)
{
    return Tool::Bow;
}

/// Offset into the map image archive to get the ware stack (lying on ground) texture
constexpr unsigned WARE_STACK_TEX_MAP_OFFSET = 2200;
/// Offset into the map image archive to get the ware texture
constexpr unsigned WARES_TEX_MAP_OFFSET = 2250;
/// Offset into the map image archive to get the ware texture when carried by a donkey
constexpr unsigned WARES_DONKEY_TEX_MAP_OFFSET = 2350;

const std::map<Tool, std::string> TOOL_NAMES_1 = {
  {Tool::Tongs, "Tongs"},       {Tool::Hammer, "Hammer"},         {Tool::Axe, "Axe"},
  {Tool::Saw, "Saw"},           {Tool::PickAxe, "PickAxe"},       {Tool::Shovel, "Shovel"},
  {Tool::Crucible, "Crucible"}, {Tool::RodAndLine, "RodAndLine"}, {Tool::Scythe, "Scythe"},
  {Tool::Cleaver, "Cleaver"},   {Tool::Rollingpin, "Rollingpin"}, {Tool::Bow, "Bow"}};

const std::map<std::string, GoodType> GOOD_NAMES_MAP = {{"Beer", GoodType::Beer},
                                                        {"Tongs", GoodType::Tongs},
                                                        {"Hammer", GoodType::Hammer},
                                                        {"Axe", GoodType::Axe},
                                                        {"Saw", GoodType::Saw},
                                                        {"PickAxe", GoodType::PickAxe},
                                                        {"Shovel", GoodType::Shovel},
                                                        {"Crucible", GoodType::Crucible},
                                                        {"RodAndLine", GoodType::RodAndLine},
                                                        {"Scythe", GoodType::Scythe},
                                                        {"WaterEmpty", GoodType::WaterEmpty},
                                                        {"Water", GoodType::Water},
                                                        {"Cleaver", GoodType::Cleaver},
                                                        {"Rollingpin", GoodType::Rollingpin},
                                                        {"Bow", GoodType::Bow},
                                                        {"Boat", GoodType::Boat},
                                                        {"Sword", GoodType::Sword},
                                                        {"Iron", GoodType::Iron},
                                                        {"Flour", GoodType::Flour},
                                                        {"Fish", GoodType::Fish},
                                                        {"Bread", GoodType::Bread},
                                                        {"ShieldRomans", GoodType::ShieldRomans},
                                                        {"Wood", GoodType::Wood},
                                                        {"Boards", GoodType::Boards},
                                                        {"Stones", GoodType::Stones},
                                                        {"ShieldVikings", GoodType::ShieldVikings},
                                                        {"ShieldAfricans", GoodType::ShieldAfricans},
                                                        {"Grain", GoodType::Grain},
                                                        {"Coins", GoodType::Coins},
                                                        {"Gold", GoodType::Gold},
                                                        {"IronOre", GoodType::IronOre},
                                                        {"Coal", GoodType::Coal},
                                                        {"Meat", GoodType::Meat},
                                                        {"Ham", GoodType::Ham},
                                                        {"ShieldJapanese", GoodType::ShieldJapanese},
                                                        {"Grapes", GoodType::Grapes},
                                                        {"Wine", GoodType::Wine},
                                                        {"Nothing", GoodType::Nothing}};

const std::map<GoodType, std::string> GOOD_NAMES_1 = {{GoodType::Beer, "Beer"},
                                                      {GoodType::Tongs, "Tongs"},
                                                      {GoodType::Hammer, "Hammer"},
                                                      {GoodType::Axe, "Axe"},
                                                      {GoodType::Saw, "Saw"},
                                                      {GoodType::PickAxe, "PickAxe"},
                                                      {GoodType::Shovel, "Shovel"},
                                                      {GoodType::Crucible, "Crucible"},
                                                      {GoodType::RodAndLine, "RodAndLine"},
                                                      {GoodType::Scythe, "Scythe"},
                                                      {GoodType::WaterEmpty, "WaterEmpty"},
                                                      {GoodType::Water, "Water"},
                                                      {GoodType::Cleaver, "Cleaver"},
                                                      {GoodType::Rollingpin, "Rollingpin"},
                                                      {GoodType::Bow, "Bow"},
                                                      {GoodType::Boat, "Boat"},
                                                      {GoodType::Sword, "Sword"},
                                                      {GoodType::Iron, "Iron"},
                                                      {GoodType::Flour, "Flour"},
                                                      {GoodType::Fish, "Fish"},
                                                      {GoodType::Bread, "Bread"},
                                                      {GoodType::ShieldRomans, "ShieldRomans"},
                                                      {GoodType::Wood, "Wood"},
                                                      {GoodType::Boards, "Boards"},
                                                      {GoodType::Stones, "Stones"},
                                                      {GoodType::ShieldVikings, "ShieldVikings"},
                                                      {GoodType::ShieldAfricans, "ShieldAfricans"},
                                                      {GoodType::Grain, "Grain"},
                                                      {GoodType::Coins, "Coins"},
                                                      {GoodType::Gold, "Gold"},
                                                      {GoodType::IronOre, "IronOre"},
                                                      {GoodType::Coal, "Coal"},
                                                      {GoodType::Meat, "Meat"},
                                                      {GoodType::Ham, "Ham"},
                                                      {GoodType::ShieldJapanese, "ShieldJapanese"},
                                                      {GoodType::Grapes, "Grapes"},
                                                      {GoodType::Wine, "Wine"},
                                                      {GoodType::Nothing, "Nothing"}};
