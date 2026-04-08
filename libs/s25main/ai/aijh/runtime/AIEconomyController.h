// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/GoodTypes.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/SettingsTypes.h"
#include "helpers/EnumArray.h"

class nobBaseWarehouse;

namespace AIJH {

class AIPlayerJH;

class AIEconomyController
{
public:
    explicit AIEconomyController(AIPlayerJH& owner) : owner_(owner) {}

    void PlanNewBuildings(unsigned gf);
    MapPoint FindBestPosition(BuildingType bt);
    void AddMilitaryBuildJob(MapPoint pt);
    void AddGlobalBuildJob(BuildingType type);
    void AddBuildJob(BuildingType type, MapPoint pt, bool front, bool searchPosition);
    void AddBuildJobAroundEveryWarehouse(BuildingType bt);
    void AddBuildJobAroundEveryMilBld(BuildingType bt);
    void SetSendingForUpgradeWarehouse(nobBaseWarehouse* upgradewarehouse);
    void SetGatheringForUpgradeWarehouse(nobBaseWarehouse* upgradewarehouse);
    void DistributeGoodsByBlocking(GoodType good, unsigned limit);
    void DistributeMaxRankSoldiersByBlocking(unsigned limit, nobBaseWarehouse* upwh);
    unsigned SoldierAvailable(int rank = -1);
    void InitStoreAndMilitarylists();
    int UpdateUpgradeBuilding();
    nobBaseWarehouse* GetUpgradeBuildingWarehouse();
    void InitDistribution();
    void CheckForester();
    void CheckGraniteMine();
    void ExecuteLuaConstructionOrder(MapPoint pt, BuildingType bt, bool forced = false);
    unsigned AmountInStorage(GoodType good) const;
    unsigned AmountInStorage(Job job) const;
    void AdjustSettings();
    void AdjustDistribution();
    unsigned CalcMilSettings();

    void RecordProducedGood(GoodType good) { ++goodsProduced_[good]; }
    const helpers::EnumArray<unsigned, GoodType>& GetProducedGoods() const { return goodsProduced_; }

private:
    AIPlayerJH& owner_;
    MapPoint upgradeBldPos_ = MapPoint::Invalid();
    helpers::EnumArray<unsigned, GoodType> goodsProduced_{};
    uint8_t metalworksIronDistributionBase_ = 0;
    Distributions distributionAdjusterBase_{};
};

} // namespace AIJH
