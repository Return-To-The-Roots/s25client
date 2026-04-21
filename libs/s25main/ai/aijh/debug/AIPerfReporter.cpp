// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AIPerfReporter.h"

#include "GamePlayer.h"
#include "ai/aijh/debug/AIRuntimeProfiler.h"
#include "ai/aijh/debug/StatsConfig.h"

#include <boost/format.hpp>
#include <fstream>
#include <iostream>

namespace {

constexpr unsigned kAIPerfLogPeriodGF = 1000;
bool perfCsvHeaderInitialized = false;

struct CsvSection
{
    AIJH::AIRuntimeProfileSection section;
    const char* name;
};

constexpr std::array<CsvSection, 11> kCsvSections = {{
  {AIJH::AIRuntimeProfileSection::RunGF, "RunGF"},
  {AIJH::AIRuntimeProfileSection::RefreshBuildingQualities, "RefreshBuildingQualities"},
  {AIJH::AIRuntimeProfileSection::BuildingPlannerUpdate, "BuildingPlannerUpdate"},
  {AIJH::AIRuntimeProfileSection::ExecuteAIJob, "ExecuteAIJob"},
  {AIJH::AIRuntimeProfileSection::EvaluateCaptureRisks, "EvaluateCaptureRisks"},
  {AIJH::AIRuntimeProfileSection::TryToAttack, "TryToAttack"},
  {AIJH::AIRuntimeProfileSection::TrySeaAttack, "TrySeaAttack"},
  {AIJH::AIRuntimeProfileSection::CheckEconomicHotspots, "CheckEconomicHotspots"},
  {AIJH::AIRuntimeProfileSection::UpdateTroopsLimit, "UpdateTroopsLimit"},
  {AIJH::AIRuntimeProfileSection::AdjustSettings, "AdjustSettings"},
  {AIJH::AIRuntimeProfileSection::PlanNewBuildings, "PlanNewBuildings"},
}};

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
    {
        perfFile << "PlayerID,GameFrame,ElapsedMillis,WindowGameFrames";
        for(const auto& csvSection : kCsvSections)
        {
            perfFile << "," << csvSection.name << "_AvgUsPerGF";
            perfFile << "," << csvSection.name << "_AvgUsPerCall";
            perfFile << "," << csvSection.name << "_Calls";
        }
        perfFile << std::endl;
    }

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

    const AIRuntimeSnapshot currentSnapshot = AIRuntimeProfiler::Instance().GetSnapshot();
    const unsigned windowGameFrames = gf - lastLoggedGF_;

    perfFile << player.GetPlayerId();
    perfFile << "," << gf;
    perfFile << "," << elapsedMillis;
    perfFile << "," << windowGameFrames;

    for(const auto& csvSection : kCsvSections)
    {
        const unsigned idx = static_cast<unsigned>(csvSection.section);
        const uint64_t deltaCalls = currentSnapshot[idx].calls - prevSnapshot_[idx].calls;
        const uint64_t deltaTotalNs = currentSnapshot[idx].totalNs - prevSnapshot_[idx].totalNs;
        const double avgUsPerGF =
          windowGameFrames > 0 ? static_cast<double>(deltaTotalNs) / 1000.0 / windowGameFrames : 0.0;
        const double avgUsPerCall =
          deltaCalls > 0 ? static_cast<double>(deltaTotalNs) / 1000.0 / deltaCalls : 0.0;
        perfFile << "," << avgUsPerGF;
        perfFile << "," << avgUsPerCall;
        perfFile << "," << deltaCalls;
    }

    perfFile << std::endl;

    lastLogTime_ = now;
    lastLoggedGF_ = gf;
    prevSnapshot_ = currentSnapshot;
}

} // namespace AIJH
