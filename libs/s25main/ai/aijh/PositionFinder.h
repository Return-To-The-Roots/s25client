//
// Created by pavel on 03.06.25.
//

#ifndef POSITIONFINDER_H
#define POSITIONFINDER_H
#include "AIPlayerJH.h"
#include "ai/RatedPoint.h"
#include "gameTypes/BuildingType.h"
#include "ai/aijh/AIMap.h"

namespace AIJH {
class AIPlayerJH;

class PositionFinder {
public:
    PositionFinder(AIPlayerJH& aijh);

    MapPoint FindBestPosition(BuildingType bt);
    RatedPoint FindPositionAround(BuildingType type, const MapPoint& around, int searchRadius);
    RatedPointSet FindBestPositions(const MapPoint& pt, AIResource res, BuildingQuality size, unsigned radius,
                                          int minimum);
private:
    AIPlayerJH& aijh;
};
}

#endif //POSITIONFINDER_H
