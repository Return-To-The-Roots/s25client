// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <s25util/warningSuppression.h>
#include <array>
#include <cstdint>
#include <figures/nofArmored.h>

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
    Skinner,           // 35
    Tanner,            // 36
    LeatherWorker,     // 37
};

constexpr auto maxEnumValue(Job)
{
    return Job::LeatherWorker;
}

/// Job types of soldiers, weak ones first
constexpr std::array SUPPRESS_UNUSED SOLDIER_JOBS = {Job::Private, Job::PrivateFirstClass, Job::Sergeant, Job::Officer,
                                                     Job::General};
constexpr bool isSoldierJob(const Job job)
{
    return job >= Job::Private && job <= Job::General;
}

constexpr unsigned NUM_SOLDIER_RANKS = SOLDIER_JOBS.size();
constexpr unsigned getSoldierRank(Job soldierJob)
{
    return static_cast<uint8_t>(soldierJob) - static_cast<uint8_t>(Job::Private);
}

enum class ArmoredSoldier : uint8_t
{
    Private,           // 0
    PrivateFirstClass, // 1
    Sergeant,          // 2
    Officer,           // 3
    General,           // 4
};

constexpr bool isSoldier(const Job job)
{
    return job >= Job::Private && job <= Job::General;
}

constexpr auto maxEnumValue(ArmoredSoldier)
{
    return ArmoredSoldier::General;
}

constexpr ArmoredSoldier figureToAmoredSoldierEnum(const nofArmored* figure)
{
    return static_cast<ArmoredSoldier>(getSoldierRank(figure->GetJobType()));
}
