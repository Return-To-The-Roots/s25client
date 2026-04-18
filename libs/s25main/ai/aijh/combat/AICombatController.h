// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ai/aijh/combat/AICombatContext.h"

#include <vector>

class noFlag;
class nobBaseMilitary;
class nobMilitary;

namespace AIJH {

class AICombatController
{
public:
    using TargetSelectionMode = AICombatTargetSelectionMode;

    explicit AICombatController(AICombatContext& owner);

    void SetTargetSelectionMode(TargetSelectionMode mode) { targetSelectionMode_ = mode; }

    void TryToAttack();
    const nobBaseMilitary* SelectAttackTarget(TargetSelectionMode mode) const;
    const nobBaseMilitary* SelectAttackTargetPrudent() const;
    const nobBaseMilitary* SelectAttackTargetBiting() const;
    const nobBaseMilitary* SelectAttackTargetAttrition() const;
    void TrySeaAttack();

    double ComputeFulfillmentLevel(double* outTotalWeight = nullptr) const;
    double ComputeEnemyFrontlineWeight() const;
    std::vector<const nobBaseMilitary*> GetPotentialTargets(unsigned& hqOrHarborWithoutSoldiers) const;
    unsigned CalcPotentialAttackers(const nobBaseMilitary& target) const;
    void EvaluateCaptureRisks();
    double ComputeCaptureRisk(const nobMilitary& building) const;

    double GetCaptureRiskEstimate(const nobBaseMilitary& building) const;

private:
    AICombatContext& owner_;
    TargetSelectionMode targetSelectionMode_ = TargetSelectionMode::Prudent;
};

} // namespace AIJH
