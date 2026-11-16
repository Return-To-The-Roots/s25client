// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
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
    void handleDerivedEvent(unsigned id) override;

    // informieren, wenn ...
    void goalReached() override; // das Ziel erreicht wurde

    /// wenn man gelaufen ist
    [[noreturn]] void walked() override;
    /// Prüft die Gesundheit des Soldaten und meldet, falls erforderlich, ein Heilungs-Event an
    void heal();

public:
    nofPassiveSoldier(MapPoint pos, unsigned char player, nobBaseMilitary* goal, nobMilitary* home, unsigned char rank);
    explicit nofPassiveSoldier(const nofSoldier& soldier);
    nofPassiveSoldier(SerializedGameData& sgd, unsigned obj_id);

    ~nofPassiveSoldier() override;

    void destroy() override;
    void serialize(SerializedGameData& sgd) const override;

    GO_Type getGOT() const final { return GO_Type::NofPassivesoldier; }

    // Zeichnet den Soldaten
    void draw(DrawPoint drawPt) override;

    /// Sagt einem in einem Militärgebäude sitzenden Soldaten, dass er raus nach Hause gehen soll
    void leaveBuilding();

    /// Befördert einen Soldaten
    void upgrade();

    /// Soldat befindet sich auf dem Hinweg zum Militärgebäude und wird nich länger gebraucht
    void notNeeded();
    /// Tells the soldier it is not in its home building anymore (e.g. died, or converted to attacker)
    void leftBuilding() { homeBld = nullptr; }
    nobMilitary* getHome() const;
};
