// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ai/AIResource.h"
#include "helpers/EnumArray.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/JobTypes.h"
#include <cstdint>

class GamePlayer;

namespace AIJH {

class BuildingPlanner;

class AIStatsSource
{
public:
    virtual ~AIStatsSource() = default;

    virtual const GamePlayer& GetPlayer() const = 0;
    virtual const BuildingPlanner& GetBldPlanner() const = 0;
    virtual unsigned GetAvailableResources(AISurfaceResource resource) const = 0;
    virtual unsigned AmountInStorage(GoodType good) const = 0;
    virtual unsigned AmountInStorage(Job job) const = 0;
    virtual unsigned GetProductivity(BuildingType type) const = 0;
    virtual const helpers::EnumArray<unsigned, GoodType>& GetProducedGoods() const = 0;
    virtual uint64_t GetGlobalPositionSearchInvocationCount() const = 0;
    virtual uint64_t GetGlobalPositionSearchCooldownSkipCount() const = 0;
};

} // namespace AIJH
