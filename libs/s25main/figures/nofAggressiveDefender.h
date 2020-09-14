// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

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

    /// The derived classes regain control after a fight of nofActiveSoldier
    void FreeFightEnded() override;

public:
    nofAggressiveDefender(MapPoint pos, unsigned char player, nobBaseMilitary* home, unsigned char rank, nofAttacker* attacker);
    nofAggressiveDefender(nofPassiveSoldier* other, nofAttacker* attacker);
    nofAggressiveDefender(SerializedGameData& sgd, unsigned obj_id);

    ~nofAggressiveDefender() override;

    /// Aufräummethoden
protected:
    void Destroy_nofAggressiveDefender();

public:
    void Destroy() override { Destroy_nofAggressiveDefender(); }

    /// Serialisierungsfunktionen
protected:
    void Serialize_nofAggressiveDefender(SerializedGameData& sgd) const;
    [[noreturn]] void HandleDerivedEvent(unsigned) override { throw std::logic_error("No events expected"); }

public:
    void Serialize(SerializedGameData& sgd) const override { Serialize_nofAggressiveDefender(sgd); }

    GO_Type GetGOT() const override { return GOT_NOF_AGGRESSIVEDEFENDER; }

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
