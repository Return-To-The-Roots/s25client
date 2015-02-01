// $Id: const_addons.h 9596 2015-02-01 09:41:54Z marcus $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef CONST_ADDONS_H_INCLUDED
#define CONST_ADDONS_H_INCLUDED

#pragma once

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

// Do not forget to add your Addon to GlobalGameSettings::reset @ GlobalGameSettings.cpp!
// Never use a number twice!

enum AddonId
{
    // AAA = Author
    // NNNNN = Number
    //                                   AAANNNNN
    ADDON_LIMIT_CATAPULTS            = 0x00000000,
    ADDON_INEXHAUSTIBLE_MINES        = 0x00000001,
    ADDON_REFUND_MATERIALS           = 0x00000002,
    ADDON_EXHAUSTIBLE_WELLS          = 0x00000003,
    ADDON_REFUND_ON_EMERGENCY        = 0x00000004,
    ADDON_MANUAL_ROAD_ENLARGEMENT    = 0x00000005,
    ADDON_CATAPULT_GRAPHICS          = 0x00000006,

    ADDON_DEMOLITION_PROHIBITION     = 0x00100000,
    ADDON_CHARBURNER                 = 0x00100001,
    ADDON_TRADE                      = 0x00100002,

    ADDON_CHANGE_GOLD_DEPOSITS       = 0x00200000,
    ADDON_MAX_WATERWAY_LENGTH        = 0x00200001,
    ADDON_CUSTOM_BUILD_SEQUENCE      = 0x00200002,
    ADDON_STATISTICS_VISIBILITY      = 0x00200003,

    ADDON_DEFENDER_BEHAVIOR          = 0x00300000,
    ADDON_AI_DEBUG_WINDOW            = 0x00300001,

    ADDON_NO_COINS_DEFAULT           = 0x00400000,

    ADDON_ADJUST_MILITARY_STRENGTH   = 0x00500000,

    ADDON_TOOL_ORDERING              = 0x00600001,

    ADDON_MILITARY_AID               = 0x00700000,

    ADDON_INEXHAUSTIBLE_GRANITEMINES = 0x00800000,

    ADDON_MAX_RANK                   = 0x00900000,
    ADDON_SEA_ATTACK                 = 0x00900001,
    ADDON_INEXHAUSTIBLE_FISH         = 0x00900002,
	ADDON_MORE_ANIMALS               = 0x00900003,
	ADDON_BURN_DURATION              = 0x00900004,
	ADDON_NO_ALLIED_PUSH             = 0x00900005,
	ADDON_BATTLEFIELD_PROMOTION      = 0x00900006,
	ADDON_HALF_COST_MIL_EQUIP        = 0x00900007,
	ADDON_MILITARY_CONTROL           = 0x00900008,

    ADDON_SHIP_SPEED                 = 0x00A00000
};

enum AddonGroup
{
    ADDONGROUP_ALL = 1,
    ADDONGROUP_MILITARY = 2,
    ADDONGROUP_ECONOMY = 4,
    ADDONGROUP_GAMEPLAY = 8,
    ADDONGROUP_OTHER = 16
};

#endif // !CONST_ADDONS_H_INCLUDED
