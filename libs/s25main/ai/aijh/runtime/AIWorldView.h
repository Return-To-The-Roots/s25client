// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ai/AIResource.h"
#include "ai/aijh/runtime/AIMap.h"
#include "ai/aijh/runtime/AIResourceMap.h"
#include "gameTypes/BuildingQuality.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/Direction.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/MapCoordinates.h"

class AIInterface;
class GamePlayer;
class GameWorldBase;
class GlobalGameSettings;
struct AIConfig;

namespace AIJH {

class BuildingPlanner;

class AIWorldView
{
public:
    virtual ~AIWorldView() = default;

    virtual AIInterface& GetInterface() = 0;
    virtual const AIInterface& GetInterface() const = 0;
    virtual const AIConfig& GetConfig() const = 0;
    virtual const GameWorldBase& GetWorld() const = 0;
    virtual const GamePlayer& GetPlayer() const = 0;
    virtual const GlobalGameSettings& GetGameSettings() const = 0;
    virtual unsigned char GetPlayerId() const = 0;

    virtual Node& GetAINode(MapPoint pt) = 0;
    virtual const Node& GetAINode(MapPoint pt) const = 0;
    virtual AIResourceMap& GetResMap(AIResource res) = 0;
    virtual const AIResourceMap& GetResMap(AIResource res) const = 0;
    virtual int GetResMapValue(MapPoint pt, AIResource res) const = 0;

    virtual unsigned BQsurroundcheck(MapPoint pt, unsigned range, bool includeexisting, unsigned limit = 0) = 0;
    virtual MapPoint FindBestPosition(const MapPoint& pt, AIResource res, BuildingQuality size, unsigned radius,
                                      int minimum = 1) = 0;
    virtual MapPoint FindPositionForBuildingAround(BuildingType type, const MapPoint& around) = 0;
    virtual unsigned GetAvailableResources(AISurfaceResource resource) const = 0;
    virtual unsigned GetNumAIRelevantSeaIds() const = 0;
    virtual unsigned GetProductivity(BuildingType type) const = 0;
    virtual bool IsInvalidShipyardPosition(MapPoint pt) = 0;
    virtual const BuildingPlanner& GetBldPlanner() const = 0;
};

} // namespace AIJH
