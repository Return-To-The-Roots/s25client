//
// Created by Codex on 19.02.26.
//

#ifndef GLOBALPOSITIONFINDER_H
#define GLOBALPOSITIONFINDER_H

#include "gameTypes/BuildingType.h"
#include "gameTypes/BuildingQuality.h"
#include "gameTypes/MapCoordinates.h"
#include <optional>

namespace AIJH {
class AIPlayerJH;

class GlobalPositionFinder
{
public:
    GlobalPositionFinder(AIPlayerJH& aijh);

    MapPoint FindBestPosition(BuildingType bt);

private:
    bool CheckProximity(BuildingType type, const MapPoint& pt) const;
    bool ValidFishInRange(MapPoint pt) const;
    bool ValidStoneinRange(MapPoint pt) const;
    std::optional<int> GetPointRating(BuildingType type, const MapPoint& pt) const;

    AIPlayerJH& aijh;
};
} // namespace AIJH

#endif // GLOBALPOSITIONFINDER_H
