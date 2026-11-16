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

/// Aggressiv-verteidigender Soldat (jemand, der den Angreifer auf offenem Feld entgegenläuft)
class nofAggressiveDefender : public nofActiveSoldier
{
    /// Soldaten, der er entgegenrennen soll
    nofAttacker* attacker;
    /// Militärgebäude, das angegriffen wird
    nobBaseMilitary* attacked_goal;

    /// wenn man gelaufen ist
    void walked() override;
    /// Geht nach Haus für MAggressiveDefending-Mission
    void returnHomeMissionAggressiveDefending();
    /// Läuft wieter
    void missAggressiveDefendingWalk();
    /// Sucht sich für MissionAggressiveAttacking ein neues Ziel, wenns keins findet, gehts nach Hause
    void missionAggressiveDefendingLookForNewAggressor();
    /// Sagt den verschiedenen Zielen Bescheid, dass wir doch nicht mehr kommen können
    void informTargetsAboutCancelling() override;

    void cancelAtAttacker();

protected:
    SoldierState freeFightAborted() override;

    [[noreturn]] void handleDerivedEvent(unsigned) override { throw std::logic_error("No events expected"); }

public:
    nofAggressiveDefender(MapPoint pos, unsigned char player, nobBaseMilitary& home, unsigned char rank,
                          nofAttacker& attacker);
    nofAggressiveDefender(const nofPassiveSoldier& other, nofAttacker& attacker);
    nofAggressiveDefender(SerializedGameData& sgd, unsigned obj_id);

    ~nofAggressiveDefender() override;

    void destroy() override;
    void serialize(SerializedGameData& sgd) const override;

    GO_Type getGOT() const final { return GO_Type::NofAggressivedefender; }

    /// Wenn ein Heimat-Militärgebäude bei Missionseinsätzen zerstört wurde
    void homeDestroyed() override;
    /// Wenn er noch in der Warteschleife vom Ausgangsgebäude hängt und dieses zerstört wurde
    void homeDestroyedAtBegin() override;

    void cancelAtAttackedBld();

    /// Wenn ein Kampf gewonnen wurde
    void wonFighting() override;
    /// Wenn ein Kampf verloren wurde (Tod)
    void lostFighting() override;

    /// Gebäude, das vom aggressiv-verteidigenden Soldaten verteidigt werden sollte, wurde zerstört
    void attackedGoalDestroyed();
    /// Soldat, der angehalten ist, um auf seinen Angreifer-Kollegen zu warten, soll jetzt weiterlaufen, da er um
    /// das Angriffsgebäude schon wartet
    void missAggressiveDefendingContinueWalking();
    /// Wenn der jeweils andere Soldat, mit dem man kämpfen wollte, nicht mehr kommen kann
    void attackerLost();

    // Debugging
    const nofAttacker* getAttacker() const { return attacker; }
};
