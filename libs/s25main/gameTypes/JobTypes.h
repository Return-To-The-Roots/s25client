// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <s25util/warningSuppression.h>
#include <array>
#include <cstdint>
#include <map>
#include <string>

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
const std::map<Job, std::string> JOB_NAMES_1 = {
    {Job::Helper, "Helper"},
    {Job::Woodcutter, "Woodcutter"},
    {Job::Fisher, "Fisher"},
    {Job::Forester, "Forester"},
    {Job::Carpenter, "Carpenter"},
    {Job::Stonemason, "Stonemason"},
    {Job::Hunter, "Hunter"},
    {Job::Farmer, "Farmer"},
    {Job::Miller, "Miller"},
    {Job::Baker, "Baker"},
    {Job::Butcher, "Butcher"},
    {Job::Miner, "Miner"},
    {Job::Brewer, "Brewer"},
    {Job::PigBreeder, "PigBreeder"},
    {Job::DonkeyBreeder, "DonkeyBreeder"},
    {Job::IronFounder, "IronFounder"},
    {Job::Minter, "Minter"},
    {Job::Metalworker, "Metalworker"},
    {Job::Armorer, "Armorer"},
    {Job::Builder, "Builder"},
    {Job::Planer, "Planer"},
    {Job::Private, "Private"},
    {Job::PrivateFirstClass, "PrivateFirstClass"},
    {Job::Sergeant, "Sergeant"},
    {Job::Officer, "Officer"},
    {Job::General, "General"},
    {Job::Geologist, "Geologist"},
    {Job::Shipwright, "Shipwright"},
    {Job::Scout, "Scout"},
    {Job::PackDonkey, "PackDonkey"},
    {Job::BoatCarrier, "BoatCarrier"},
    {Job::CharBurner, "CharBurner"},
    {Job::Winegrower, "Winegrower"},
    {Job::Vintner, "Vintner"},
    {Job::TempleServant, "TempleServant"}
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
