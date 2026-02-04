// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nofActiveSoldier.h"

class nofAttacker;
class nofPassiveSoldier;
class SerializedGameData;
class nobBaseMilitary;

/// Aggressive defending soldier (runs out onto the field to meet the attacker)
class nofAggressiveDefender : public nofActiveSoldier
{
    /// Attacker the defender is supposed to intercept
    nofAttacker* attacker;
    /// Targeted military building
    nobBaseMilitary* attacked_goal;
    /// Apply the temporary defender bonus hitpoint if applicable
    void ApplyAggressiveDefenderBonusHitpoints();

    /// Handle movement ticks
    void Walked() override;
    /// Return home for the aggressive defending mission
    void ReturnHomeMissionAggressiveDefending();
    /// Continue moving
    void MissAggressiveDefendingWalk();
    /// Look for a new attacker target; go home if none is found
    void MissionAggressiveDefendingLookForNewAggressor();
    /// Notify all mission targets that we cannot show up
    void InformTargetsAboutCancelling() override;

    void CancelAtAttacker();

protected:
    SoldierState FreeFightAborted() override;

    [[noreturn]] void HandleDerivedEvent(unsigned) override { throw std::logic_error("No events expected"); }

public:
    nofAggressiveDefender(MapPoint pos, unsigned char player, nobBaseMilitary& home, unsigned char rank,
                          nofAttacker& attacker);
    nofAggressiveDefender(const nofPassiveSoldier& other, nofAttacker& attacker);
    nofAggressiveDefender(SerializedGameData& sgd, unsigned obj_id);

    ~nofAggressiveDefender() override;

    void Destroy() override;
    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const final { return GO_Type::NofAggressivedefender; }

    /// Home military building destroyed while on a mission
    void HomeDestroyed() override;
    /// Home building destroyed while still waiting in its exit queue
    void HomeDestroyedAtBegin() override;

    void CancelAtAttackedBld();

    /// Wenn ein Kampf gewonnen wurde
    void WonFighting() override;
    /// Wenn ein Kampf verloren wurde (Tod)
    void LostFighting() override;

    /// Building the aggressive defender was protecting got destroyed
    void AttackedGoalDestroyed();
    /// Tell the defender who waited for the attacker to keep moving because the attacker already waits at the building
    void MissAggressiveDefendingContinueWalking();
    /// Counterpart soldier we wanted to fight cannot arrive anymore
    void AttackerLost();
    /// Still waiting inside the warehouse but must defend HQ etc., so the mission is aborted
    void NeedForHomeDefence();

    // Debugging
    const nofAttacker* GetAttacker() const { return attacker; }
};
