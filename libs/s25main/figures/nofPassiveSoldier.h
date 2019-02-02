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
#ifndef NOF_PASSIVESOLDIER_H_
#define NOF_PASSIVESOLDIER_H_

#include "nofSoldier.h"

class SerializedGameData;
class nobBaseMilitary;

/// Soldaten, die nur in Militärgebäude warten bzw. vom HQ dareinkommen und noch keine spezielle Funktion
/// übernehmen
class nofPassiveSoldier : public nofSoldier
{
private:
    /// "Heilungs-Event"
    const GameEvent* healing_event;

private:
    /// Eventhandling
    void HandleDerivedEvent(unsigned id) override;

    // informieren, wenn ...
    void GoalReached() override; // das Ziel erreicht wurde

    /// wenn man gelaufen ist
    void Walked() override;
    /// Prüft die Gesundheit des Soldaten und meldet, falls erforderlich, ein Heilungs-Event an
    void Heal();

public:
    nofPassiveSoldier(const nofSoldier& soldier);
    nofPassiveSoldier(const MapPoint pt, unsigned char player, nobBaseMilitary* const goal, nobBaseMilitary* const home,
                      const unsigned char rank);
    nofPassiveSoldier(SerializedGameData& sgd, unsigned obj_id);

    ~nofPassiveSoldier() override;

    /// Aufräummethoden
protected:
    void Destroy_nofPassiveSoldier();

public:
    void Destroy() override { Destroy_nofPassiveSoldier(); }

    /// Serialisierungsfunktionen
protected:
    void Serialize_nofPassiveSoldier(SerializedGameData& sgd) const;

public:
    void Serialize(SerializedGameData& sgd) const override { Serialize_nofPassiveSoldier(sgd); }

    GO_Type GetGOT() const override { return GOT_NOF_PASSIVESOLDIER; }

    // Zeichnet den Soldaten
    void Draw(DrawPoint drawPt) override;

    /// wenn Militärgebäude abgerissen wurde und sich der Soldat im Gebäude befand
    void InBuildingDestroyed();
    /// Sagt einem in einem Militärgebäude sitzenden Soldaten, dass er raus nach Hause gehen soll
    void LeaveBuilding();

    /// Befördert einen Soldaten
    void Upgrade();

    /// Soldat befindet sich auf dem Hinweg zum Militärgebäude und wird nich länger gebraucht
    void NotNeeded();
    /// Tells the soldier it is not in its home building anymore (e.g. died, or converted to attacker)
    void LeftBuilding() { building = nullptr; }
};

#endif // !NOF_PASSIVESOLDIER_H_
