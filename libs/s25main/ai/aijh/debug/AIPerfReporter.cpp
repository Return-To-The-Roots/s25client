// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AIPerfReporter.h"

#include "GamePlayer.h"
#include "ai/aijh/debug/StatsConfig.h"

#include <boost/format.hpp>
#include <fstream>
#include <iostream>

namespace {

constexpr unsigned kAIPerfLogPeriodGF = 1000;
bool perfCsvHeaderInitialized = false;

std::ofstream CreatePerfCsvFile()
{
    boost::format fmt("%s/%s.csv");
    fmt % STATS_CONFIG.statsPath % "ai_performance";
    std::ofstream file(fmt.str(), std::ios::app);

    if(!file)
        std::cerr << "Unable open ai_performance file for appending!" << std::endl;
    return file;
}

void InitializePerfCsvFile()
{
    if(perfCsvHeaderInitialized)
        return;

    std::ofstream perfFile = CreatePerfCsvFile();
    if(perfFile)
        perfFile << "GameFrame,ElapsedMillis,GlobalPositionSearchInvocations,GlobalPositionSearchCooldownSkips"
                 << std::endl;

    perfCsvHeaderInitialized = true;
}

} // namespace

namespace AIJH {

AIPerfReporter::AIPerfReporter(const AIStatsSource& owner) : owner_(owner) {}

void AIPerfReporter::MaybeLog(const unsigned gf)
{
    if(gf % kAIPerfLogPeriodGF != 0)
        return;

    const GamePlayer& player = owner_.GetPlayer();
    if(player.ps == PlayerState::Locked)
        return;

    InitializePerfCsvFile();

    std::ofstream perfFile = CreatePerfCsvFile();
    if(!perfFile)
        return;

    const auto now = std::chrono::steady_clock::now();
    const auto elapsedMillis =
      (lastLogTime_ == std::chrono::steady_clock::time_point{}) ?
        0 :
        std::chrono::duration_cast<std::chrono::milliseconds>(now - lastLogTime_).count();

    const uint64_t globalPositionSearchInvocations = owner_.GetGlobalPositionSearchInvocationCount();
    const uint64_t globalPositionSearchCooldownSkips = owner_.GetGlobalPositionSearchCooldownSkipCount();

    perfFile << gf;
    perfFile << "," << elapsedMillis;
    perfFile << "," << (globalPositionSearchInvocations - lastGlobalPositionSearchInvocations_);
    perfFile << "," << (globalPositionSearchCooldownSkips - lastGlobalPositionSearchCooldownSkips_);
    perfFile << std::endl;

    lastLogTime_ = now;
    lastGlobalPositionSearchInvocations_ = globalPositionSearchInvocations;
    lastGlobalPositionSearchCooldownSkips_ = globalPositionSearchCooldownSkips;
}

} // namespace AIJH
