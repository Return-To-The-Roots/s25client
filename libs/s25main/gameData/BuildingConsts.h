// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "DrawPoint.h"
#include "helpers/EnumArray.h"
#include "helpers/MultiArray.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/BuildingTypes.h"
#include "gameTypes/Nation.h"

extern const helpers::EnumArray<const char*, BuildingType> BUILDING_NAMES;

constexpr helpers::EnumArray<BuildingCost, BuildingType> SUPPRESS_UNUSED BUILDING_COSTS = {
  {{0, 0}, {2, 0}, {2, 3}, {0, 0}, {3, 5}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {4, 7}, {4, 0},
   {4, 0}, {4, 0}, {4, 0}, {4, 0}, {0, 0}, {4, 2}, {2, 0}, {2, 0}, {2, 0}, {2, 0}, {2, 2},
   {2, 0}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {4, 3}, {3, 3}, {4, 3}, {0, 0}, {2, 2}, {2, 2},
   {2, 2}, {2, 2}, {2, 0}, {2, 3}, {3, 3}, {3, 3}, {4, 6}, {4, 4}, {2, 3}, {4, 7}}};

// Bauqualitäten der Gebäude
constexpr helpers::EnumArray<BuildingQuality, BuildingType> SUPPRESS_UNUSED BUILDING_SIZE = {
  {BuildingQuality::Castle,  BuildingQuality::Hut,     BuildingQuality::Hut,     BuildingQuality::Nothing,
   BuildingQuality::House,   BuildingQuality::Nothing, BuildingQuality::Nothing, BuildingQuality::Nothing,
   BuildingQuality::Nothing, BuildingQuality::Castle,  BuildingQuality::Mine,    BuildingQuality::Mine,
   BuildingQuality::Mine,    BuildingQuality::Mine,    BuildingQuality::Hut,     BuildingQuality::Nothing,
   BuildingQuality::House,   BuildingQuality::Hut,     BuildingQuality::Hut,     BuildingQuality::Hut,
   BuildingQuality::Hut,     BuildingQuality::House,   BuildingQuality::Hut,     BuildingQuality::House,
   BuildingQuality::House,   BuildingQuality::House,   BuildingQuality::House,   BuildingQuality::Castle,
   BuildingQuality::Castle,  BuildingQuality::House,   BuildingQuality::Nothing, BuildingQuality::House,
   BuildingQuality::House,   BuildingQuality::House,   BuildingQuality::House,   BuildingQuality::Hut,
   BuildingQuality::House,   BuildingQuality::Castle,  BuildingQuality::Castle,  BuildingQuality::Harbor,
   BuildingQuality::Castle,  BuildingQuality::House,   BuildingQuality::Castle}};

const helpers::EnumArray<BldWorkDescription, BuildingType> SUPPRESS_UNUSED BLD_WORK_DESC = {{
  {}, // HQ
  {Job::Private, boost::none, WaresNeeded(GoodType::Coins), 1},
  {Job::Private, boost::none, WaresNeeded(GoodType::Coins), 2},
  {},
  {Job::Private, boost::none, WaresNeeded(GoodType::Coins), 4},
  {},
  {},
  {},
  {},
  {Job::Private, boost::none, WaresNeeded(GoodType::Coins), 6},
  {Job::Miner, GoodType::Stones, WaresNeeded(GoodType::Fish, GoodType::Meat, GoodType::Bread), 2, false},
  {Job::Miner, GoodType::Coal, WaresNeeded(GoodType::Fish, GoodType::Meat, GoodType::Bread), 2, false},
  {Job::Miner, GoodType::IronOre, WaresNeeded(GoodType::Fish, GoodType::Meat, GoodType::Bread), 2, false},
  {Job::Miner, GoodType::Gold, WaresNeeded(GoodType::Fish, GoodType::Meat, GoodType::Bread), 2, false},
  {Job::Scout}, // No production, just existence
  {},
  {Job::Helper, boost::none, WaresNeeded(GoodType::Stones), 4},
  {Job::Woodcutter, GoodType::Wood},
  {Job::Fisher, GoodType::Fish},
  {Job::Stonemason, GoodType::Stones},
  {Job::Forester, GoodType::Nothing}, // Produces trees
  {Job::Butcher, GoodType::Meat, WaresNeeded(GoodType::Ham)},
  {Job::Hunter, GoodType::Meat},
  {Job::Brewer, GoodType::Beer, WaresNeeded(GoodType::Grain, GoodType::Water)},
  {Job::Armorer, GoodType::Sword, WaresNeeded(GoodType::Iron, GoodType::Coal)},
  {Job::Metalworker, GoodType::Tongs, WaresNeeded(GoodType::Iron, GoodType::Boards)},
  {Job::IronFounder, GoodType::Iron, WaresNeeded(GoodType::IronOre, GoodType::Coal)},
  {Job::CharBurner, GoodType::Coal, WaresNeeded(GoodType::Wood, GoodType::Grain)},
  {Job::PigBreeder, GoodType::Ham, WaresNeeded(GoodType::Grain, GoodType::Water)},
  {}, // Storehouse
  {},
  {Job::Miller, GoodType::Flour, WaresNeeded(GoodType::Grain)},
  {Job::Baker, GoodType::Bread, WaresNeeded(GoodType::Flour, GoodType::Water)},
  {Job::Carpenter, GoodType::Boards, WaresNeeded(GoodType::Wood)},
  {Job::Minter, GoodType::Coins, WaresNeeded(GoodType::Gold, GoodType::Coal)},
  {Job::Helper, GoodType::Water},
  {Job::Shipwright, GoodType::Boat, WaresNeeded(GoodType::Boards)},
  {Job::Farmer, GoodType::Grain},
  {Job::DonkeyBreeder, GoodType::Nothing,
   WaresNeeded(GoodType::Grain, GoodType::Water)}, // Produces a job. TODO: Better way
  {},                                              // Harbour
  {Job::Winegrower, GoodType::Grapes, WaresNeeded(GoodType::Wood, GoodType::Water)},
  {Job::Vintner, GoodType::Wine, WaresNeeded(GoodType::Grapes)},
  {Job::TempleServant, GoodType::Gold, WaresNeeded(GoodType::Wine, GoodType::Meat, GoodType::Bread)},
}};

/// Smoke consts for all buildings and nations
extern const helpers::MultiEnumArray<SmokeConst, Nation, BuildingType> BUILDING_SMOKE_CONSTS;

/// Offset of the production-/gold- stop signs per building
constexpr helpers::MultiEnumArray<DrawPoint, Nation, BuildingType> SUPPRESS_UNUSED BUILDING_SIGN_CONSTS = {
  {// Nubier
   {{{0, 0},    {19, -4},   {19, -3},  {0, 0},    {23, -19}, {0, 0},    {0, 0},    {0, 0},    {0, 0},
     {29, -23}, {-2, -15},  {2, -13},  {-5, -16}, {-5, -15}, {0, 0},    {0, 0},    {0, 0},    {4, -16},
     {9, -12},  {7, -10},   {28, -18}, {-6, -16}, {-3, -15}, {21, -11}, {14, -7},  {-14, -9}, {11, -9},
     {-5, -24}, {-18, -34}, {0, 0},    {0, 0},    {3, -13},  {13, -13}, {-4, -20}, {7, -14},  {15, -4},
     {0, 0},    {-22, -16}, {-14, -8}, {0, 0},    {22, -7},  {-8, -10}, {9, -24}}},
   // Japaner
   {{{0, 0},     {12, -10}, {13, -10},  {0, 0},    {25, -22},  {0, 0},     {0, 0},    {0, 0},    {0, 0},
     {10, -14},  {20, -2},  {12, -14},  {14, -13}, {13, -14},  {0, 0},     {0, 0},    {0, 0},    {-8, -6},
     {0, 0},     {-13, -7}, {14, -11},  {15, -10}, {-13, -7},  {16, -11},  {21, -3},  {-6, -2},  {14, -14},
     {-26, -17}, {30, -25}, {0, 0},     {0, 0},    {0, -22},   {-30, -13}, {35, -20}, {13, -34}, {19, -15},
     {-22, -10}, {37, -13}, {-15, -36}, {0, 0},    {-26, -13}, {-8, -8},   {9, -10}}},
   // Römer
   {{{0, 0},     {15, -3},  {14, -2},   {0, 0},   {0, 0},    {0, 0},    {0, 0},    {0, 0},   {0, 0},
     {20, -39},  {15, -2},  {26, 4},    {21, 4},  {21, 4},   {0, 0},    {0, 0},    {0, 0},   {-29, -10},
     {0, 0},     {-4, 4},   {7, -20},   {3, -13}, {0, 0},    {22, -8},  {18, -11}, {6, -14}, {23, -10},
     {-28, -12}, {35, -27}, {0, 0},     {0, 0},   {2, -21},  {15, -13}, {11, -30}, {23, -5}, {10, -9},
     {0, -25},   {41, -43}, {-34, -19}, {0, 0},   {36, -12}, {-8, -8},  {12, -19}}},
   // Wikinger
   {{{0, 0},     {-5, 0},   {-5, 0},   {0, 0},   {-7, -5},  {0, 0},    {0, 0},    {0, 0},    {0, 0},
     {17, -10},  {20, 2},   {20, 0},   {20, -2}, {20, 0},   {0, 0},    {0, 0},    {13, -5},  {15, -6},
     {-7, 2},    {17, -8},  {-5, 0},   {-3, 2},  {-6, 0},   {-7, 0},   {-32, -2}, {-13, -3}, {-8, -2},
     {-22, -18}, {-25, -8}, {0, 0},    {0, 0},   {-8, -2},  {-17, -4}, {28, -16}, {-1, 0},   {8, -9},
     {16, -15},  {-2, -25}, {-29, -9}, {0, 0},   {-7, -32}, {-10, -1}, {-4, 5}}},
   // Babylonier
   {{{0, 0},    {19, -6},   {19, -20}, {0, 0},    {5, -15},   {0, 0},    {0, 0},    {0, 0},    {0, 0},
     {20, -10}, {15, -10},  {15, -10}, {15, -10}, {15, -10},  {0, 0},    {0, 0},    {13, -5},  {-5, -13},
     {15, -20}, {15, -1},   {11, -7},  {1, -22},  {7, -12},   {14, -16}, {21, -18}, {13, -11}, {5, -17},
     {-2, -29}, {18, -20},  {0, 0},    {0, 0},    {14, -13},  {3, -17},  {0, -18},  {12, -10}, {16, 0},
     {4, -16},  {-15, -11}, {-24, -9}, {0, 0},    {-32, -20}, {4, -16},  {10, -18}}}}};

/// Position der nubischen Feuer für alle 4 Bergwerke
/// (Granit, Kohle, Eisen, Gold)
constexpr std::array<DrawPoint, 4> SUPPRESS_UNUSED NUBIAN_MINE_FIRE = {{
  {31, -18},
  {34, -10},
  {30, -11},
  {32, -10},
}};

/// Hilfetexte für Gebäude
extern const helpers::EnumArray<const char*, BuildingType> BUILDING_HELP_STRINGS;
