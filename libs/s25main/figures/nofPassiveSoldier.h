// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nofSoldier.h"

class SerializedGameData;
class nobBaseMilitary;
class nobMilitary;

/// Soldiers who wait inside military buildings or arrive from the HQ and do not yet take on a special role
class nofPassiveSoldier : public nofSoldier
{
private:
    /// "Healing event"
    const GameEvent* healing_event;

    /// Event handling
    void HandleDerivedEvent(unsigned id) override;

    // Notify when ...
    void GoalReached() override; // the goal has been reached

    /// When the soldier has walked
    [[noreturn]] void Walked() override;
    /// Checks the soldier's health and schedules a healing event if required
    void Heal();
    /// Determine the healing interval based on the building's original owner
    unsigned GetHealingInterval() const;

public:
    nofPassiveSoldier(MapPoint pos, unsigned char player, nobBaseMilitary* goal, nobMilitary* home, unsigned char rank);
    explicit nofPassiveSoldier(const nofSoldier& soldier);
    nofPassiveSoldier(SerializedGameData& sgd, unsigned obj_id);

    ~nofPassiveSoldier() override;

    void Destroy() override;
    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const final { return GO_Type::NofPassivesoldier; }

    // Draws the soldier
    void Draw(DrawPoint drawPt) override;

    /// Tells a soldier sitting in a military building to leave and go home
    void LeaveBuilding();

    /// Promotes a soldier
    void Upgrade();

    /// Soldier is on the way to the military building and is no longer needed
    void NotNeeded();
    /// Tells the soldier it is not in its home building anymore (e.g. died, or converted to attacker)
    void LeftBuilding() { building = nullptr; }
    nobMilitary* getHome() const;
};
