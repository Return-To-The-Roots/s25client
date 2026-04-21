// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ai/aijh/debug/AIRuntimeProfiler.h"

#include "gameData/BuildingConsts.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <numeric>
#include <string_view>

namespace AIJH {

namespace {

using namespace std::chrono;

constexpr std::array<std::string_view, static_cast<unsigned>(AIRuntimeProfileSection::Count)> kSectionNames = {
  "RunGF",
  "RefreshBuildingQualities", "BuildingPlannerUpdate",      "ExecuteAIJob",           "ExecuteEventJobs",
  "ExecuteConstructionJobs",  "ExecuteConnectJobs",         "ExecuteGlobalBuildJobs", "ExecuteBuildJobs",
  "SelectTargetAttrition",    "AttritionGetPotential",      "AttritionRecaptureScan", "AttritionPickRecap",
  "AttritionForceAdv",        "AttritionNearDensity",       "AttritionFallbackBiting","EvaluateCaptureRisks",
  "TryToAttack",              "TrySeaAttack",               "CheckEconomicHotspots",  "UpdateTroopsLimit",
  "AdjustSettings",           "PlanNewBuildings",           "UpdateTroopsLimitScan",  "UpdateTroopsLimitScore",
  "UpdateTroopsLimitDist",    "UpdateTroopsLimitApply"};

double ToMilliseconds(const std::uint64_t nanoseconds) { return static_cast<double>(nanoseconds) / 1'000'000.0; }

} // namespace

AIRuntimeProfiler& AIRuntimeProfiler::Instance()
{
    static AIRuntimeProfiler profiler;
    return profiler;
}

AIRuntimeProfiler::AIRuntimeProfiler() : enabled_([] {
    const char* env = std::getenv("RTTR_AI_PROFILE");
    return env != nullptr && env[0] != '\0' && env[0] != '0';
}())
{
}

AIRuntimeProfiler::~AIRuntimeProfiler()
{
    if(!enabled_)
        return;

    std::array<unsigned, static_cast<unsigned>(AIRuntimeProfileSection::Count)> order{};
    std::iota(order.begin(), order.end(), 0u);
    std::sort(order.begin(), order.end(), [this](const unsigned lhs, const unsigned rhs) {
        return stats_[lhs].totalNs > stats_[rhs].totalNs;
    });

    std::fprintf(stderr, "\nAI runtime profile summary\n");
    std::fprintf(stderr, "%-24s %10s %12s %12s %12s %12s %12s\n", "section", "calls", "total ms", "avg us",
                 "max us", "avg work", "max work");
    for(const unsigned idx : order)
    {
        const SectionStats& stat = stats_[idx];
        if(stat.calls == 0)
            continue;

        const double avgUs = stat.calls == 0 ? 0.0 : static_cast<double>(stat.totalNs) / 1000.0 / stat.calls;
        const double maxUs = static_cast<double>(stat.maxNs) / 1000.0;
        const double avgWork = stat.calls == 0 ? 0.0 : static_cast<double>(stat.totalWorkUnits) / stat.calls;
        std::fprintf(stderr, "%-24.*s %10llu %12.3f %12.3f %12.3f %12.1f %12llu\n",
                     static_cast<int>(kSectionNames[idx].size()), kSectionNames[idx].data(),
                     static_cast<unsigned long long>(stat.calls), ToMilliseconds(stat.totalNs), avgUs, maxUs, avgWork,
                     static_cast<unsigned long long>(stat.maxWorkUnits));
    }

    std::array<BuildingType, helpers::NumEnumValues_v<BuildingType>> buildingOrder{};
    for(unsigned i = 0; i < buildingOrder.size(); ++i)
        buildingOrder[i] = static_cast<BuildingType>(i);
    std::sort(buildingOrder.begin(), buildingOrder.end(), [this](const BuildingType lhs, const BuildingType rhs) {
        return globalBuildJobStats_[lhs].totalNs > globalBuildJobStats_[rhs].totalNs;
    });

    std::fprintf(stderr, "\nGlobal build jobs by building type\n");
    std::fprintf(stderr, "%-24s %10s %12s %12s %12s\n", "building", "calls", "total ms", "avg us", "max us");
    for(const BuildingType buildingType : buildingOrder)
    {
        const SectionStats& stat = globalBuildJobStats_[buildingType];
        if(stat.calls == 0)
            continue;

        const double avgUs = static_cast<double>(stat.totalNs) / 1000.0 / stat.calls;
        const double maxUs = static_cast<double>(stat.maxNs) / 1000.0;
        std::fprintf(stderr, "%-24s %10llu %12.3f %12.3f %12.3f\n", BUILDING_NAMES[buildingType],
                     static_cast<unsigned long long>(stat.calls), ToMilliseconds(stat.totalNs), avgUs, maxUs);
    }
}

AIRuntimeSnapshot AIRuntimeProfiler::GetSnapshot() const
{
    AIRuntimeSnapshot snapshot;
    for(unsigned i = 0; i < static_cast<unsigned>(AIRuntimeProfileSection::Count); ++i)
        snapshot[i] = {stats_[i].calls, stats_[i].totalNs, stats_[i].maxNs, stats_[i].totalWorkUnits,
                       stats_[i].maxWorkUnits};
    return snapshot;
}

void AIRuntimeProfiler::AddSample(const AIRuntimeProfileSection section, const nanoseconds duration,
                                  const std::uint64_t workUnits)
{
    if(!enabled_)
        return;

    SectionStats& stat = stats_[static_cast<unsigned>(section)];
    const auto durationNs = static_cast<std::uint64_t>(duration.count());
    ++stat.calls;
    stat.totalNs += durationNs;
    stat.maxNs = std::max(stat.maxNs, durationNs);
    stat.totalWorkUnits += workUnits;
    stat.maxWorkUnits = std::max(stat.maxWorkUnits, workUnits);
}

ScopedAIRuntimeProfile::ScopedAIRuntimeProfile(const AIRuntimeProfileSection section, const std::uint64_t workUnits)
    : profiler_(&AIRuntimeProfiler::Instance()), section_(section), workUnits_(workUnits)
{
    if(profiler_->IsEnabled())
        start_ = steady_clock::now();
}

ScopedAIRuntimeProfile::~ScopedAIRuntimeProfile()
{
    if(!profiler_->IsEnabled())
        return;

    profiler_->AddSample(section_, duration_cast<nanoseconds>(steady_clock::now() - start_), workUnits_);
}

ScopedAIGlobalBuildJobProfile::ScopedAIGlobalBuildJobProfile(const BuildingType buildingType)
    : profiler_(&AIRuntimeProfiler::Instance()), buildingType_(buildingType)
{
    if(profiler_->IsEnabled())
        start_ = steady_clock::now();
}

ScopedAIGlobalBuildJobProfile::~ScopedAIGlobalBuildJobProfile()
{
    if(!profiler_->IsEnabled())
        return;

    profiler_->globalBuildJobStats_[buildingType_].calls++;
    const auto durationNs = static_cast<std::uint64_t>(duration_cast<nanoseconds>(steady_clock::now() - start_).count());
    auto& stat = profiler_->globalBuildJobStats_[buildingType_];
    stat.totalNs += durationNs;
    stat.maxNs = std::max(stat.maxNs, durationNs);
}

} // namespace AIJH
