// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nofActiveSoldier.h"

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
    void Walked() override;
    /// Geht nach Haus für MAggressiveDefending-Mission
    void ReturnHomeMissionAggressiveDefending();
    /// Läuft wieter
    void MissAggressiveDefendingWalk();
    /// Sucht sich für MissionAggressiveAttacking ein neues Ziel, wenns keins findet, gehts nach Hause
    void MissionAggressiveDefendingLookForNewAggressor();
    /// Sagt den verschiedenen Zielen Bescheid, dass wir doch nicht mehr kommen können
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

    /// Wenn ein Heimat-Militärgebäude bei Missionseinsätzen zerstört wurde
    void HomeDestroyed() override;
    /// Wenn er noch in der Warteschleife vom Ausgangsgebäude hängt und dieses zerstört wurde
    void HomeDestroyedAtBegin() override;

    void CancelAtAttackedBld();

    /// Wenn ein Kampf gewonnen wurde
    void WonFighting() override;
    /// Wenn ein Kampf verloren wurde (Tod)
    void LostFighting() override;

    /// Gebäude, das vom aggressiv-verteidigenden Soldaten verteidigt werden sollte, wurde zerstört
    void AttackedGoalDestroyed();
    /// Soldat, der angehalten ist, um auf seinen Angreifer-Kollegen zu warten, soll jetzt weiterlaufen, da er um
    /// das Angriffsgebäude schon wartet
    void MissAggressiveDefendingContinueWalking();
    /// Wenn der jeweils andere Soldat, mit dem man kämpfen wollte, nicht mehr kommen kann
    void AttackerLost();
    /// Ich befinde mich noch im Lagerhaus in der Warteschlange und muss mein HQ etc. verteidigen
    /// Mission muss also abgebrochen werden
    void NeedForHomeDefence();

    // Debugging
    const nofAttacker* GetAttacker() const { return attacker; }
};
