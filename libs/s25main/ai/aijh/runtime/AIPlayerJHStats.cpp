// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AIPlayerJH.h"

#include "ai/aijh/debug/AIPerfReporter.h"
#include "ai/aijh/debug/AIStatsReporter.h"

namespace AIJH {

void AIPlayerJH::TrackCombatStart(const nobBaseMilitary& target)
{
    statsReporter_->TrackCombatStart(target);
}

void AIPlayerJH::InitializeCombatsLogFile() const
{
    statsReporter_->InitializeCombatsLogFile();
}

void AIPlayerJH::LogFinishedCombats(unsigned gf) const
{
    statsReporter_->LogFinishedCombats(gf);
}

void AIPlayerJH::saveStats(unsigned gf) const
{
    statsReporter_->SaveStats(gf);
}

void AIPlayerJH::saveDebugStats(unsigned gf) const
{
    statsReporter_->SaveDebugStats(gf);
}

void AIPlayerJH::RecordGlobalPositionSearchInvocation()
{
    ++globalPositionSearchInvocations_;
}

void AIPlayerJH::RecordGlobalPositionSearchCooldownSkip()
{
    ++globalPositionSearchCooldownSkips_;
}

} // namespace AIJH
