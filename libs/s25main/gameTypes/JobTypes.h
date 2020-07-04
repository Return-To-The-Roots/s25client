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

#ifndef JobTypes_h__
#define JobTypes_h__

#include "helpers/MaxEnumValue.h"
#include <s25util/warningSuppression.h>
#include <array>

enum Job
{
    JOB_HELPER,            // 0
    JOB_WOODCUTTER,        // 1
    JOB_FISHER,            // 2
    JOB_FORESTER,          // 3
    JOB_CARPENTER,         // 4
    JOB_STONEMASON,        // 5
    JOB_HUNTER,            // 6
    JOB_FARMER,            // 7
    JOB_MILLER,            // 8
    JOB_BAKER,             // 9
    JOB_BUTCHER,           // 10
    JOB_MINER,             // 11
    JOB_BREWER,            // 12
    JOB_PIGBREEDER,        // 13
    JOB_DONKEYBREEDER,     // 14
    JOB_IRONFOUNDER,       // 15
    JOB_MINTER,            // 16
    JOB_METALWORKER,       // 17
    JOB_ARMORER,           // 18
    JOB_BUILDER,           // 19
    JOB_PLANER,            // 20
    JOB_PRIVATE,           // 21
    JOB_PRIVATEFIRSTCLASS, // 22
    JOB_SERGEANT,          // 23
    JOB_OFFICER,           // 24
    JOB_GENERAL,           // 25
    JOB_GEOLOGIST,         // 26
    JOB_SHIPWRIGHT,        // 27
    JOB_SCOUT,             // 28
    JOB_PACKDONKEY,        // 29
    JOB_BOATCARRIER,       // 30
    JOB_CHARBURNER,        // 31
    JOB_NOTHING            // 32
};

DEFINE_MAX_ENUM_VALUE(Job, JOB_CHARBURNER)
/// Number of job types
constexpr unsigned NUM_JOB_TYPES = helpers::NumEnumValues_v<Job>;

constexpr unsigned NUM_SOLDIER_RANKS = 5;
/// Job types of soldiers, weak ones first
static const std::array<Job, NUM_SOLDIER_RANKS> SUPPRESS_UNUSED SOLDIER_JOBS = {
  {JOB_PRIVATE, JOB_PRIVATEFIRSTCLASS, JOB_SERGEANT, JOB_OFFICER, JOB_GENERAL}};

#endif // JobTypes_h__
