// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ai/aijh/debug/AIStatsSource.h"
#include "gameTypes/BuildingType.h"

class nobBaseMilitary;

namespace AIJH {

class AIStatsReporter
{
public:
    explicit AIStatsReporter(const AIStatsSource& owner);

    void TrackCombatStart(const nobBaseMilitary& target);
    void LogFinishedCombats(unsigned gf) const;
    void InitializeCombatsLogFile() const;
    void SaveStats(unsigned gf) const;
    void SaveDebugStats(unsigned gf) const;

private:
    const AIStatsSource& owner_;
    mutable unsigned lastStatsFrame_ = 0;
};

} // namespace AIJH
