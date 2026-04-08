// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/MapCoordinates.h"

#include <vector>

class nobBaseMilitary;
class nobMilitary;

namespace AIJH {

class AIPlayerJH;

class AIMilitaryLogistics
{
public:
    explicit AIMilitaryLogistics(AIPlayerJH& owner) : owner_(owner) {}

    void RememberLostMilitaryBuilding(MapPoint pt);
    void ForgetLostMilitaryBuilding(MapPoint pt);
    void PruneRecentlyLostBuildings();
    bool IsRecentlyLostMilitaryBuilding(MapPoint pt) const;
    void MilUpgradeOptim();
    bool HasFrontierBuildings();
    void UpdateTroopsLimit();
    double ComputeFulfillmentLevel(double* outTotalWeight = nullptr) const;
    double ComputeEnemyFrontlineWeight() const;
    double GetCombatFulfillmentLevel() const;
    double GetCombatAttackWeight() const;
    bool IsInDefenseMode() const;
    void UpdateCombatMode();
    bool CanAttackInDefenseMode(const nobBaseMilitary& target, unsigned attackersCount) const;
    bool IsLonelyEnemyStronghold(const nobBaseMilitary& target) const;
    void TryToAttack();
    unsigned CalcPotentialAttackers(const nobBaseMilitary& target) const;
    double GetCaptureRiskEstimate(const nobBaseMilitary& building) const;
    void EvaluateCaptureRisks();
    double ComputeCaptureRisk(const nobMilitary& building) const;
    void TrySeaAttack();

private:
    struct RecentlyLostBuilding
    {
        MapPoint pos;
        unsigned gf;
    };

    AIPlayerJH& owner_;
    std::vector<RecentlyLostBuilding> recentlyLostBuildings_;
};

} // namespace AIJH
