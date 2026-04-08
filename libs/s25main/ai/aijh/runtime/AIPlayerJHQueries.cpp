// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AIPlayerJH.h"

#include "ai/aijh/runtime/AIWorldQueries.h"

namespace AIJH {

MapPoint AIPlayerJH::SimpleFindPosition(const MapPoint& pt, BuildingQuality size, unsigned radius) const
{
    return worldQueries_->SimpleFindPosition(pt, size, radius);
}

MapPoint AIPlayerJH::FindPositionForBuildingAround(BuildingType type, const MapPoint& around)
{
    return worldQueries_->FindPositionForBuildingAround(type, around);
}

unsigned AIPlayerJH::GetAvailableResources(AISurfaceResource resource) const
{
    return worldQueries_->GetAvailableResources(resource);
}

unsigned AIPlayerJH::GetDensity(MapPoint pt, AIResource res, int radius)
{
    return worldQueries_->GetDensity(pt, res, radius);
}

bool AIPlayerJH::HuntablesinRange(const MapPoint pt, unsigned min)
{
    return worldQueries_->HuntablesinRange(pt, min);
}

bool AIPlayerJH::ValidTreeinRange(const MapPoint pt)
{
    return worldQueries_->ValidTreeinRange(pt);
}

bool AIPlayerJH::ValidStoneinRange(const MapPoint pt)
{
    return worldQueries_->ValidStoneinRange(pt);
}

unsigned AIPlayerJH::BQsurroundcheck(const MapPoint pt, unsigned range, bool includeexisting, unsigned limit)
{
    return worldQueries_->BQsurroundcheck(pt, range, includeexisting, limit);
}

bool AIPlayerJH::HarborPosRelevant(unsigned harborid, bool onlyempty) const
{
    return worldQueries_->HarborPosRelevant(harborid, onlyempty);
}

bool AIPlayerJH::NoEnemyHarbor()
{
    return worldQueries_->NoEnemyHarbor();
}

bool AIPlayerJH::IsInvalidShipyardPosition(const MapPoint pt)
{
    return worldQueries_->IsInvalidShipyardPosition(pt);
}

bool AIPlayerJH::ValidFishInRange(const MapPoint pt)
{
    return worldQueries_->ValidFishInRange(pt);
}

unsigned AIPlayerJH::GetNumAIRelevantSeaIds() const
{
    return worldQueries_->GetNumAIRelevantSeaIds();
}

unsigned AIPlayerJH::GetProductivity(BuildingType type) const
{
    return worldQueries_->GetProductivity(type);
}

} // namespace AIJH
