// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/BuildingType.h"
#include "helpers/EnumArray.h"

#include <chrono>
#include <cstdint>

namespace AIJH {

enum class AIRuntimeProfileSection : unsigned
{
    RefreshBuildingQualities,
    BuildingPlannerUpdate,
    ExecuteAIJob,
    ExecuteEventJobs,
    ExecuteConstructionJobs,
    ExecuteConnectJobs,
    ExecuteGlobalBuildJobs,
    ExecuteBuildJobs,
    SelectAttackTargetAttrition,
    AttritionGetPotentialTargets,
    AttritionRecaptureScan,
    AttritionPickRecapture,
    AttritionForceAdvantageCheck,
    AttritionNearTroopsDensityCheck,
    AttritionFallbackBiting,
    EvaluateCaptureRisks,
    TryToAttack,
    TrySeaAttack,
    CheckEconomicHotspots,
    UpdateTroopsLimit,
    AdjustSettings,
    PlanNewBuildings,
    UpdateTroopsLimitScan,
    UpdateTroopsLimitScore,
    UpdateTroopsLimitDistribute,
    UpdateTroopsLimitApply,
    Count
};

class AIRuntimeProfiler
{
public:
    static AIRuntimeProfiler& Instance();

    bool IsEnabled() const { return enabled_; }
    void AddSample(AIRuntimeProfileSection section, std::chrono::nanoseconds duration, std::uint64_t workUnits = 0);

private:
    AIRuntimeProfiler();
    ~AIRuntimeProfiler();

    struct SectionStats
    {
        std::uint64_t calls = 0;
        std::uint64_t totalNs = 0;
        std::uint64_t maxNs = 0;
        std::uint64_t totalWorkUnits = 0;
        std::uint64_t maxWorkUnits = 0;
    };

    bool enabled_;
    SectionStats stats_[static_cast<unsigned>(AIRuntimeProfileSection::Count)]{};
    helpers::EnumArray<SectionStats, BuildingType> globalBuildJobStats_{};

    friend class ScopedAIGlobalBuildJobProfile;
};

class ScopedAIRuntimeProfile
{
public:
    explicit ScopedAIRuntimeProfile(AIRuntimeProfileSection section, std::uint64_t workUnits = 0);
    ~ScopedAIRuntimeProfile();

private:
    AIRuntimeProfiler* profiler_;
    AIRuntimeProfileSection section_;
    std::uint64_t workUnits_;
    std::chrono::steady_clock::time_point start_;
};

class ScopedAIGlobalBuildJobProfile
{
public:
    explicit ScopedAIGlobalBuildJobProfile(BuildingType buildingType);
    ~ScopedAIGlobalBuildJobProfile();

private:
    AIRuntimeProfiler* profiler_;
    BuildingType buildingType_;
    std::chrono::steady_clock::time_point start_;
};

} // namespace AIJH
