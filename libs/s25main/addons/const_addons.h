// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "helpers/EnumWithString.h"
#include "s25util/enumUtils.h"

// Addon Author List
//
// 000 FloSoft
// 001 OLiver
// 002 Divan
// 003 jh
// 004 Kaffeepause (https://launchpad.net/~haw)
// 005 CS2001
// 006 quixui
// 007 KaiN (https://launchpad.net/~Szekta)
// 008 Spikeone
// 009 PoC
// 00A Marcus
// 00B Ribosom
// 00C Flamefire
// 00D Shawn8901
// 00E Jonathan
// 00F Jarno
// 010 aztimh

// Do not forget to add your Addon to GlobalGameSettings::registerAllAddons @ GlobalGameSettings.cpp!
// Never use a number twice!

// AAA = Author
// NNNNN = Number
//                                   AAANNNNN
//
// Add the #include for your AddonXXX.h in Addons.h!
//
ENUM_WITH_STRING(AddonId, LIMIT_CATAPULTS = 0x00000000, INEXHAUSTIBLE_MINES = 0x00000001, REFUND_MATERIALS = 0x00000002,
                 EXHAUSTIBLE_WATER = 0x00000003, REFUND_ON_EMERGENCY = 0x00000004, MANUAL_ROAD_ENLARGEMENT = 0x00000005,
                 CATAPULT_GRAPHICS = 0x00000006, METALWORKSBEHAVIORONZERO = 0x00000007,

                 DEMOLITION_PROHIBITION = 0x00100000, CHARBURNER = 0x00100001, TRADE = 0x00100002,

                 CHANGE_GOLD_DEPOSITS = 0x00200000, MAX_WATERWAY_LENGTH = 0x00200001,
                 CUSTOM_BUILD_SEQUENCE = 0x00200002, STATISTICS_VISIBILITY = 0x00200003,

                 DEFENDER_BEHAVIOR = 0x00300000, AI_DEBUG_WINDOW = 0x00300001,

                 NO_COINS_DEFAULT = 0x00400000,

                 ADJUST_MILITARY_STRENGTH = 0x00500000,

                 TOOL_ORDERING = 0x00600001,

                 MILITARY_AID = 0x00700000,

                 INEXHAUSTIBLE_GRANITEMINES = 0x00800000,

                 MAX_RANK = 0x00900000, SEA_ATTACK = 0x00900001, INEXHAUSTIBLE_FISH = 0x00900002,
                 MORE_ANIMALS = 0x00900003, BURN_DURATION = 0x00900004, NO_ALLIED_PUSH = 0x00900005,
                 BATTLEFIELD_PROMOTION = 0x00900006, HALF_COST_MIL_EQUIP = 0x00900007, MILITARY_CONTROL = 0x00900008,

                 SHIP_SPEED = 0x00A00000,

                 MILITARY_HITPOINTS = 0x00B00000,

                 NUM_SCOUTS_EXPLORATION = 0x00C00000,

                 FRONTIER_DISTANCE_REACHABLE = 0x00D0000, COINS_CAPTURED_BLD = 0x00D0001,
                 DEMOLISH_BLD_WO_RES = 0x00D0002,

                 PEACEFULMODE = 0x00E0000, DURABLE_GEOLOGIST_SIGNS = 0x00E0001, ECONOMY_MODE_GAME_LENGTH = 0x00E0002,

                 AUTOFLAGS = 0x00F00000,

                 WINE = 0x01000000)
//-V:AddonId:801

enum class AddonGroup : unsigned
{
    Military = 1,
    Economy = 2,
    GamePlay = 4,
    Other = 8,
    All = 0xFFFFFFFF
};

MAKE_BITSET_STRONG(AddonGroup);
