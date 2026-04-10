// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/Direction.h"
#include "gameTypes/MapCoordinates.h"
#include <cstdint>
#include <vector>

class GameWorldBase;
enum class RoadType : uint8_t;

namespace RoadEventLogger {

enum class RoadConstructionFailureReason
{
    Unspecified,
    RouteTooShort,
    InvalidStartFlag,
    BlockedRoute,
    EndFlagWrongOwner,
    EndFlagCannotBePlaced,
};

enum class RoadDemolitionReason
{
    Unspecified,
    Manual,
    FlagDestroyed,
    BuildingDestroyed,
    Capture,
    TerritoryRecalc,
    Split,
};

enum class FlagBuildReason
{
    Unspecified,
    Manual,
    RoadEndpoint,
    AutoFlags,
    BuildingFront,
    Split,
};

enum class FlagDemolitionReason
{
    Unspecified,
    Manual,
    TerritoryRecalc,
};

class ScopedRoadDemolitionContext
{
public:
    ScopedRoadDemolitionContext(RoadDemolitionReason reason, unsigned initiatorPlayerId = 0);
    ~ScopedRoadDemolitionContext();

private:
    RoadDemolitionReason previousReason_;
    unsigned previousInitiatorPlayerId_;
};

class ScopedFlagDemolitionContext
{
public:
    ScopedFlagDemolitionContext(FlagDemolitionReason reason, unsigned initiatorPlayerId = 0);
    ~ScopedFlagDemolitionContext();

private:
    FlagDemolitionReason previousReason_;
    unsigned previousInitiatorPlayerId_;
};

RoadDemolitionReason GetCurrentRoadDemolitionReason();
unsigned GetCurrentRoadDemolitionInitiatorPlayerId();
FlagDemolitionReason GetCurrentFlagDemolitionReason();
unsigned GetCurrentFlagDemolitionInitiatorPlayerId();

void LogRoadConstructed(unsigned gf, const GameWorldBase& world, unsigned char playerId, MapPoint start,
                        const std::vector<Direction>& route, RoadType roadType, bool createdEndFlag);
void LogRoadConstructionFailed(unsigned gf, const GameWorldBase& world, unsigned char playerId, MapPoint start,
                               const std::vector<Direction>& route, RoadType roadType,
                               RoadConstructionFailureReason reason);
void LogRoadDemolished(unsigned gf, const GameWorldBase& world, unsigned char playerId, MapPoint start, MapPoint end,
                       const std::vector<Direction>& route, RoadType roadType);
void LogFlagBuilt(unsigned gf, const GameWorldBase& world, unsigned char playerId, MapPoint pt, FlagBuildReason reason);
void LogFlagDemolished(unsigned gf, const GameWorldBase& world, unsigned char playerId, MapPoint pt);

} // namespace RoadEventLogger
