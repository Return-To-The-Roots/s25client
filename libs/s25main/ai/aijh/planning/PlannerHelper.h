#pragma once

#include "gameTypes/BuildingType.h"

namespace AIJH {
class AIWorldView;
}

extern unsigned maxWorkers(const AIJH::AIWorldView& aijh, BuildingType type);
extern unsigned maxFishers(const AIJH::AIWorldView& aijh);
extern unsigned maxHunters(const AIJH::AIWorldView& aijh);
extern unsigned maxBakers(const AIJH::AIWorldView& aijh);
extern unsigned maxButcher(const AIJH::AIWorldView& aijh);
extern unsigned maxFarmer(const AIJH::AIWorldView& aijh);
extern unsigned maxWoodcutter(const AIJH::AIWorldView& aijh);
extern unsigned maxIronFounder(const AIJH::AIWorldView& aijh);
