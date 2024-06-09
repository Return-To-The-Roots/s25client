// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <s25util/warningSuppression.h>
#include <array>
#include <cstdint>

enum class Job : uint8_t
{
    Helper,            // 0
    Woodcutter,        // 1
    Fisher,            // 2
    Forester,          // 3
    Carpenter,         // 4
    Stonemason,        // 5
    Hunter,            // 6
    Farmer,            // 7
    Miller,            // 8
    Baker,             // 9
    Butcher,           // 10
    Miner,             // 11
    Brewer,            // 12
    PigBreeder,        // 13
    DonkeyBreeder,     // 14
    IronFounder,       // 15
    Minter,            // 16
    Metalworker,       // 17
    Armorer,           // 18
    Builder,           // 19
    Planer,            // 20
    Private,           // 21
    PrivateFirstClass, // 22
    Sergeant,          // 23
    Officer,           // 24
    General,           // 25
    Geologist,         // 26
    Shipwright,        // 27
    Scout,             // 28
    PackDonkey,        // 29
    BoatCarrier,       // 30
    CharBurner,        // 31
    Winegrower,        // 32
    Vintner,           // 33
    TempleServant,     // 34
};

constexpr auto maxEnumValue(Job)
{
    return Job::TempleServant;
}

constexpr unsigned NUM_SOLDIER_RANKS = 5;
/// Job types of soldiers, weak ones first
static const std::array<Job, NUM_SOLDIER_RANKS> SUPPRESS_UNUSED SOLDIER_JOBS = {
  {Job::Private, Job::PrivateFirstClass, Job::Sergeant, Job::Officer, Job::General}};
constexpr unsigned getSoldierRank(Job soldierJob)
{
    return static_cast<uint8_t>(soldierJob) - static_cast<uint8_t>(Job::Private);
}
