// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ai/AIResource.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/BuildingQuality.h"
#include "gameTypes/MapCoordinates.h"

namespace AIJH {

class AIPlayerJH;

class AIWorldQueries
{
public:
    explicit AIWorldQueries(AIPlayerJH& owner) : owner_(owner) {}

    MapPoint SimpleFindPosition(const MapPoint& pt, BuildingQuality size, unsigned radius) const;
    MapPoint FindPositionForBuildingAround(BuildingType type, const MapPoint& around);
    unsigned GetAvailableResources(AISurfaceResource resource) const;
    unsigned GetDensity(MapPoint pt, AIResource res, int radius);
    bool HuntablesinRange(MapPoint pt, unsigned min);
    bool ValidTreeinRange(MapPoint pt);
    bool ValidStoneinRange(MapPoint pt);
    unsigned BQsurroundcheck(MapPoint pt, unsigned range, bool includeexisting, unsigned limit = 0);
    bool HarborPosRelevant(unsigned harborid, bool onlyempty = false) const;
    bool NoEnemyHarbor();
    bool IsInvalidShipyardPosition(MapPoint pt);
    bool ValidFishInRange(MapPoint pt);
    unsigned GetNumAIRelevantSeaIds() const;
    unsigned GetProductivity(BuildingType type) const;

private:
    AIPlayerJH& owner_;
};

} // namespace AIJH
