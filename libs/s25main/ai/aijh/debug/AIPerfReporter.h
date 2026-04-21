// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ai/aijh/debug/AIStatsSource.h"
#include <chrono>

namespace AIJH {

class AIPerfReporter
{
public:
    explicit AIPerfReporter(const AIStatsSource& owner);

    void MaybeLog(unsigned gf);

private:
    const AIStatsSource& owner_;
    std::chrono::steady_clock::time_point lastLogTime_{};
    uint64_t lastGlobalPositionSearchInvocations_ = 0;
    uint64_t lastGlobalPositionSearchCooldownSkips_ = 0;
};

} // namespace AIJH
