// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/BuildingType.h"

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
    AIPlayerJH& owner_;
    mutable unsigned lastStatsFrame_ = 0;
};

} // namespace AIJH
