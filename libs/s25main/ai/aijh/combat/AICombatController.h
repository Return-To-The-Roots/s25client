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
    const nobBaseMilitary* SelectAttackTargetRandom() const;
    const nobBaseMilitary* SelectAttackTargetPrudent() const;
    const nobBaseMilitary* SelectAttackTargetBiting() const;
    const nobBaseMilitary* SelectAttackTargetAttrition() const;
    void TrySeaAttack();

    void UpdateCombatMode();
    bool CanAttackInDefenseMode(const nobBaseMilitary& target, unsigned attackersCount) const;
    bool IsLonelyEnemyStronghold(const nobBaseMilitary& target) const;
    double ComputeFulfillmentLevel(double* outTotalWeight = nullptr) const;
    double ComputeEnemyFrontlineWeight() const;
    std::vector<const nobBaseMilitary*> GetPotentialTargets(unsigned& hqOrHarborWithoutSoldiers) const;
    unsigned CalcPotentialAttackers(const nobBaseMilitary& target) const;
    void EvaluateCaptureRisks();
    double ComputeCaptureRisk(const nobMilitary& building) const;

    double GetCombatFulfillmentLevel() const { return combatFulfillmentLevel_; }
    double GetCombatAttackWeight() const { return combatAttackWeight_; }
    bool IsInDefenseMode() const { return attackMode_ == CombatMode::DefenseMode; }
    double GetCaptureRiskEstimate(const nobBaseMilitary& building) const;

private:
    enum class CombatMode
    {
        AttackMode,
        DefenseMode
    };

    AICombatContext& owner_;
    CombatMode attackMode_ = CombatMode::DefenseMode;
    TargetSelectionMode targetSelectionMode_ = TargetSelectionMode::Random;
    double combatFulfillmentLevel_ = 0.0;
    double combatAttackWeight_ = 0.0;
};

} // namespace AIJH
