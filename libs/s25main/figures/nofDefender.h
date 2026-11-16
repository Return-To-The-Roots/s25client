// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nofActiveSoldier.h"
#include <stdexcept>

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
    void walked() override;

    /// Hand back control to derived class after a fight of nofActiveSoldier, not possible for defenders
    [[noreturn]] SoldierState freeFightAborted() override
    {
        throw std::logic_error("Should not participate in free fights");
    }

protected:
    void handleDerivedEvent [[noreturn]] (unsigned) override { throw std::logic_error("No events expected"); }

public:
    nofDefender(MapPoint pos, unsigned char player, nobBaseMilitary& home, unsigned char rank, nofAttacker& attacker);
    nofDefender(const nofPassiveSoldier& other, nofAttacker& attacker);
    nofDefender(SerializedGameData& sgd, unsigned obj_id);

    void destroy() override
    {
        RTTR_Assert(!attacker);
        nofActiveSoldier::destroy();
    }
    void Serialize(SerializedGameData& sgd) const override;

    GO_Type getGOT() const final { return GO_Type::NofDefender; }

    /// Inform that a new attacker arrived at the flag while we are going in such that we can come right back
    void newAttacker(nofAttacker& attacker) { this->attacker = &attacker; }
    /// The attacker won't come to the flag anymore
    void attackerArrested();

    /// Home military building destroyed during mission
    void homeDestroyed() override;
    /// Home military building destroyed while waiting to go out
    void homeDestroyedAtBegin() override;
    /// Fight was won
    void wonFighting() override;
    /// Fight was lost -> Die
    void lostFighting() override;

    /// Is the defender waiting at the flag for an attacker?
    bool isWaitingAtFlag() const { return (state == SoldierState::DefendingWaiting); }
    bool isFightingAtFlag() const { return (state == SoldierState::Fighting); }
    /// Inform the defender that a fight between him and an attacker has started
    void fightStarted() { state = SoldierState::Fighting; }

    // For debugging
    const nofAttacker* getAttacker() const { return attacker; }
};
