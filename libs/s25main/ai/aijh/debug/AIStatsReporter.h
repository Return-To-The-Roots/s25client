// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/BuildingType.h"
#include "gameTypes/MapCoordinates.h"
#include <iosfwd>
#include <string>
#include <vector>

class nobBaseMilitary;

namespace AIJH {

class AIPlayerJH;

class AIStatsReporter
{
public:
    explicit AIStatsReporter(AIPlayerJH& owner);

    void TrackCombatStart(const nobBaseMilitary& target);
    void LogFinishedCombats(unsigned gf) const;
    void InitializeCombatsLogFile() const;
    void SaveStats(unsigned gf) const;
    void SaveDebugStats(unsigned gf) const;

private:
    struct ActiveCombat
    {
        MapPoint pos;
        unsigned targetObjId;
        unsigned char defenderPlayer;
        BuildingType buildingType;
        unsigned startGf;
        bool sawAggressor = false;
    };

    enum class CombatLogState
    {
        Pending,
        Success,
        Failure
    };

    std::string GetCombatsLogPath() const;
    std::string FormatPlayerLabel(unsigned playerIdx) const;
    void LogPlayerMetadata(std::ofstream& combatsFile) const;
    CombatLogState EvaluateCombatState(ActiveCombat& combat, unsigned gf) const;
    bool HasOwnAggressors(const nobBaseMilitary& building) const;

    AIPlayerJH& owner_;
    mutable std::vector<ActiveCombat> activeCombats_;
    mutable unsigned lastStatsFrame_ = 0;
    mutable bool combatsLogInitialized_ = false;
};

} // namespace AIJH
