// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "DrawPoint.h"
#include "helpers/MultiArray.h"
#include "gameTypes/JobTypes.h"
#include "gameData/NationConsts.h"
#include <array>

/// Size of the military squares (in nodes) that the world is divided into for
/// garrison calculations
constexpr uint16_t MILITARY_SQUARE_SIZE = 20;

/// Maximum distances used for the "near" and "middle" military building ranges
constexpr unsigned MAX_MILITARY_DISTANCE_NEAR = 18;
constexpr unsigned MAX_MILITARY_DISTANCE_MIDDLE = 26;

/// highest military rank - currently ranks 0-4 available
constexpr unsigned MAX_MILITARY_RANK = NUM_SOLDIER_RANKS - 1u;
/// Number of military buildings
constexpr unsigned NUM_MILITARY_BLDS = 4;

/// Base attacking distance (allows bringing all available soldiers)
constexpr unsigned BASE_ATTACKING_DISTANCE = 17;

/// Extended distance beyond the base range; each additional step removes one
/// soldier from the attacking army
constexpr unsigned EXTENDED_ATTACKING_DISTANCE = 1;

/// Maximum marching distance for attackers on the way to a target
constexpr unsigned MAX_ATTACKING_RUN_DISTANCE = 40;

/// Distance between two opponents that triggers them to walk toward each other
constexpr unsigned MEET_FOR_FIGHT_DISTANCE = 5;

/// Garrison sizes per military building type and nation
constexpr helpers::EnumArray<std::array<int, NUM_MILITARY_BLDS>, Nation> NUM_TROOPS = {
  {{2, 3, 6, 9}, {2, 3, 6, 9}, {2, 3, 6, 9}, {2, 3, 6, 9}, {2, 3, 6, 9}}};

/// Amount of gold stored per military building type and nation
constexpr helpers::EnumArray<std::array<int, NUM_MILITARY_BLDS>, Nation> NUM_GOLDS = {
  {{1, 2, 4, 6}, {1, 2, 4, 6}, {1, 2, 4, 6}, {1, 2, 4, 6}, {1, 2, 4, 6}}};

/// Territory radius contributed by each military building type
constexpr std::array<unsigned, NUM_MILITARY_BLDS> SUPPRESS_UNUSED MILITARY_RADIUS = {{8, 9, 10, 11}};
// Territory radius for individual harbor (construction) sites
constexpr unsigned HARBOR_RADIUS = 8;
constexpr unsigned HQ_RADIUS = 9;

/// Offset of the troop flag per nation and type from the buildings origin
constexpr helpers::EnumArray<std::array<DrawPoint, NUM_MILITARY_BLDS>, Nation> TROOPS_FLAG_OFFSET = {
  {{{{24, -41}, {19, -41}, {31, -88}, {35, -67}}},
   {{{-9, -49}, {14, -59}, {16, -63}, {0, -44}}},
   {{{-24, -36}, {9, -62}, {-2, -80}, {23, -75}}},
   {{{-5, -50}, {-5, -51}, {-9, -74}, {-12, -58}}},
   {{{-22, -37}, {-2, -51}, {20, -70}, {-46, -64}}}}};

/// Offset of the troop flag per nation from the HQs origin
constexpr helpers::EnumArray<DrawPoint, Nation> TROOPS_FLAG_HQ_OFFSET = {
  {{-12, -102}, {-19, -94}, {-18, -112}, {20, -54}, {-33, -81}}};

/// Offset of the border indicator flag per nation from the buildings origin
constexpr helpers::EnumArray<std::array<DrawPoint, NUM_MILITARY_BLDS>, Nation> BORDER_FLAG_OFFSET = {
  {{{{-6, -36}, {7, -48}, {-18, -28}, {-47, -64}}},
   {{{17, -45}, {-3, -49}, {-30, -25}, {22, -53}}},
   {{{28, -19}, {29, -18}, {-27, -12}, {-49, -62}}},
   {{{24, -19}, {24, -19}, {17, -52}, {-37, -32}}},
   {{{8, -26}, {13, -36}, {-1, -59}, {-10, -61}}}}};

/// Maximum hitpoints per soldier rank
constexpr std::array<uint8_t, NUM_SOLDIER_RANKS> HITPOINTS = {3, 4, 5, 6, 7};

/// Max distance for an attacker to reach a building and join in capturing
constexpr unsigned MAX_FAR_AWAY_CAPTURING_DISTANCE = 15;

/// Additional vision range granted by military buildings (added to border range)
constexpr unsigned VISUALRANGE_MILITARY = 3;
/// Absolute vision radius of lookout towers
constexpr unsigned VISUALRANGE_LOOKOUTTOWER = 20;
/// Vision radius of scouts
constexpr unsigned VISUALRANGE_SCOUT = 3;
/// Vision radius of soldiers
constexpr unsigned VISUALRANGE_SOLDIER = 2;
/// Vision radius of ships
constexpr unsigned VISUALRANGE_SHIP = 2;
/// Vision radius of exploration ships
constexpr unsigned VISUALRANGE_EXPLORATION_SHIP = 12;

/// Promotion wait time for soldiers ( =UPGRADE_TIME + rand(UPGRADE_TIME_RANDOM) )
constexpr unsigned UPGRADE_TIME = 100;
constexpr unsigned UPGRADE_TIME_RANDOM = 300;
/// Recovery time for soldiers resting in buildings per hitpoint regained
// ( =CONVALESCE_TIME + rand(CONVALESCE_TIME_RANDOM) )
constexpr unsigned CONVALESCE_TIME = 500;
constexpr unsigned CONVALESCE_TIME_RANDOM = 500;

/// Maximum distance between a harbor and the military building that may be
/// attacked from the sea
constexpr unsigned SEAATTACK_DISTANCE = 15;

/// Combat animation frame IDs per soldier rank (stored in ROM_BOBS.LST)
struct FightAnimation
{
    // Attack animation (8 frames)
    std::array<uint16_t, 8> attacking;
    // Three different defend animations with 8 frames each
    uint16_t defending[3][8];
};

/// Available for both facing directions, every rank, and every nation
extern const helpers::EnumArray<helpers::MultiArray<FightAnimation, NUM_SOLDIER_RANKS, 2>, Nation> FIGHT_ANIMATIONS;

/// Highlight sprite IDs for hit soldiers per nation
extern const helpers::EnumArray<std::array<uint16_t, NUM_SOLDIER_RANKS>, Nation> HIT_SOLDIERS;

/// Frame index at which victims flash when struck by attackers (per rank)
constexpr std::array<uint16_t, NUM_SOLDIER_RANKS> SUPPRESS_UNUSED HIT_MOMENT = {{4, 4, 4, 4, 6}};
