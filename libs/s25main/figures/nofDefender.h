// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nofActiveSoldier.h"

class nofAttacker;
class nofPassiveSoldier;
class SerializedGameData;
class nobBaseMilitary;

/// Defender coming out of the building when an attacker is at the flag
class nofDefender : public nofActiveSoldier
{
    /// Attacker at the flag
    nofAttacker* attacker;

    /// Walked to the next map point
    void Walked() override;

    /// Hand back control to derived class after a fight of nofActiveSoldier, not possible for defenders
    [[noreturn]] SoldierState FreeFightAborted() override
    {
        throw std::logic_error("Should not participate in free fights");
    }

protected:
    void HandleDerivedEvent [[noreturn]] (unsigned) override { throw std::logic_error("No events expected"); }

public:
    nofDefender(MapPoint pos, unsigned char player, nobBaseMilitary& home, unsigned char rank, nofAttacker& attacker);
    nofDefender(const nofPassiveSoldier& other, nofAttacker& attacker);
    nofDefender(SerializedGameData& sgd, unsigned obj_id);

    void Destroy() override
    {
        RTTR_Assert(!attacker);
        nofActiveSoldier::Destroy();
    }
    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const final { return GO_Type::NofDefender; }

    /// Inform that a new attacker arrived at the flag while we are going on such that we can come right back
    void NewAttacker(nofAttacker& attacker) { this->attacker = &attacker; }
    /// The attacker won't come to the flag anymore
    void AttackerArrested();

    /// Home military building destroyed during mission
    void HomeDestroyed() override;
    /// Home military building destroyed while waiting to go out
    void HomeDestroyedAtBegin() override;
    /// Fight was won
    void WonFighting() override;
    /// Fight was lost -> Die
    void LostFighting() override;

    /// Is the defender waiting at the flag for an attacker?
    bool IsWaitingAtFlag() const { return (state == SoldierState::DefendingWaiting); }
    bool IsFightingAtFlag() const { return (state == SoldierState::Fighting); }
    /// Inform the defender that a fight between him and an attacker has started
    void FightStarted() { state = SoldierState::Fighting; }

    // For debugging
    const nofAttacker* GetAttacker() const { return attacker; }
};
