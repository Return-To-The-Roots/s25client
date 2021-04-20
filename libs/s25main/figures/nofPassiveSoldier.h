// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nofSoldier.h"

class SerializedGameData;
class nobBaseMilitary;
class nobMilitary;

/// Soldaten, die nur in Militärgebäude warten bzw. vom HQ dareinkommen und noch keine spezielle Funktion
/// übernehmen
class nofPassiveSoldier : public nofSoldier
{
private:
    /// "Heilungs-Event"
    const GameEvent* healing_event;

    /// Eventhandling
    void HandleDerivedEvent(unsigned id) override;

    // informieren, wenn ...
    void GoalReached() override; // das Ziel erreicht wurde

    /// wenn man gelaufen ist
    [[noreturn]] void Walked() override;
    /// Prüft die Gesundheit des Soldaten und meldet, falls erforderlich, ein Heilungs-Event an
    void Heal();

public:
    nofPassiveSoldier(const nofSoldier& soldier);
    nofPassiveSoldier(MapPoint pos, unsigned char player, nobBaseMilitary* goal, nobMilitary* home, unsigned char rank);
    nofPassiveSoldier(SerializedGameData& sgd, unsigned obj_id);

    ~nofPassiveSoldier() override;

    void Destroy() override;
    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const final { return GO_Type::NofPassivesoldier; }

    // Zeichnet den Soldaten
    void Draw(DrawPoint drawPt) override;

    /// Sagt einem in einem Militärgebäude sitzenden Soldaten, dass er raus nach Hause gehen soll
    void LeaveBuilding();

    /// Befördert einen Soldaten
    void Upgrade();

    /// Soldat befindet sich auf dem Hinweg zum Militärgebäude und wird nich länger gebraucht
    void NotNeeded();
    /// Tells the soldier it is not in its home building anymore (e.g. died, or converted to attacker)
    void LeftBuilding() { building = nullptr; }
    nobMilitary* getHome() const;
};
