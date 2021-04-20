// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nofActiveSoldier.h"

class nofAttacker;
class nofPassiveSoldier;
class SerializedGameData;
class nobBaseMilitary;

/// Verteidiger, der rauskommt, wenn ein Angreifer an die Flagge kommt
class nofDefender : public nofActiveSoldier
{
    /// angreifender Soldat an der Flagge
    nofAttacker* attacker;

    /// wenn man gelaufen ist
    void Walked() override;

    /// The derived classes regain control after a fight of nofActiveSoldier
    [[noreturn]] void FreeFightEnded() override;

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

    /// Der Verteidiger geht gerade rein und es kommt ein neuer Angreifer an die Flagge, hiermit wird der Ver-
    /// teidiger darüber informiert, damit er dann gleich wieder umdrehen kann
    void NewAttacker(nofAttacker& attacker) { this->attacker = &attacker; }
    /// Der Angreifer konnte nicht mehr an die Flagge kommen
    void AttackerArrested();

    /// Wenn ein Heimat-Militärgebäude bei Missionseinsätzen zerstört wurde
    void HomeDestroyed() override;
    /// Wenn er noch in der Warteschleife vom Ausgangsgebäude hängt und dieses zerstört wurde
    void HomeDestroyedAtBegin() override;
    /// Wenn ein Kampf gewonnen wurde
    void WonFighting() override;
    /// Wenn ein Kampf verloren wurde (Tod)
    void LostFighting() override;

    /// Is the defender waiting at the flag for an attacker?
    bool IsWaitingAtFlag() const { return (state == SoldierState::DefendingWaiting); }
    bool IsFightingAtFlag() const { return (state == SoldierState::Fighting); }
    /// Informs the defender that a fight between him and an attacker has started
    void FightStarted() { state = SoldierState::Fighting; }

    // Debugging
    const nofAttacker* GetAttacker() const { return attacker; }
};
