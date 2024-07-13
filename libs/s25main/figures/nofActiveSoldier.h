// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nofSoldier.h"
#include <cstdint>
#include <optional>

class SerializedGameData;
class nobBaseMilitary;

/// Base class for all 3 types of "active" soldiers: Attackers, defenders and aggressive defenders
/// i.e. who are in the free world to fight and don't walk on the roads just to fill buildings
class nofActiveSoldier : public nofSoldier
{
public:
    /// State of each soldier
    enum class SoldierState : uint8_t
    {
        FigureWork,      /// Go to work etc., all which is done by noFigure
        WalkingHome,     /// Walking home after work to the military building
        MeetEnemy,       /// Prepare fighting with an enemy
        WaitingForFight, /// Standing still and waiting for a fight
        Fighting,        /// Fighting

        AttackingWalkingToGoal,         /// Attacker is walking to his attacked destination
        AttackingWaitingAroundBuilding, /// Attacker is waiting around the building for his fight at the flag
                                        /// against the defender(s)
        AttackingWaitingForDefender,    /// Waiting at the flag until the defender emerges from the building
        AttackingCapturingFirst,        /// Captures the hostile building as first person
        AttackingCapturingNext,         /// The next soldiers capture the building in this state
        AttackingAttackingFlag,         /// Goes to the flag to fight the defender
        AttackingFightingVsDefender,    /// Fighting against a defender at the flag

        SeaattackingGoToHarbor,   /// Goes from his home military building to the start harbor
        SeaattackingWaitInHarbor, /// Waiting in the start harbor for the ship
        SeaattackingOnShip,       /// On the ship to the destination
        SeaattackingReturnToShip, /// Returns to the ship at the destination environment

        AggressivedefendingWalkingToAggressor, /// Follow the attacker in order to fight against him

        DefendingWaiting,    /// Waiting at the flag for further attackers
        DefendingWalkingTo,  /// Goes to the flag before the fight
        DefendingWalkingFrom /// Goes into the building after the fight
    };
    friend constexpr auto maxEnumValue(SoldierState) { return nofActiveSoldier::SoldierState::DefendingWalkingFrom; }

protected:
    /// State of the soldier, always has to be a valid value
    SoldierState state;

private:
    /// Current enemy when fighting in the nofActiveSoldier modes (and only in this case!)
    nofActiveSoldier* enemy;
    /// Meeting point for fighting against the enemy
    MapPoint fightSpot_;

protected:
    /// Start returning home
    void ReturnHome();
    /// Walking home, called after each walking step
    void WalkingHome();

    /// Examine hostile people on roads and expels them
    void ExpelEnemies() const;

    /// Handle walking for nofActiveSoldier specific states
    void Walked() override;

    /// Look for a nearby enemy which wants to fight with this soldier
    /// Return true if one found
    bool FindEnemiesNearby(const std::optional<unsigned char>& excludedOwner = std::nullopt);
    /// Inform this soldier that another soldier starts meeting him
    void MeetEnemy(nofActiveSoldier* other, MapPoint figh_spot);
    /// Handle state "meet enemy" after each walking step
    void MeetingEnemy();
    /// Look for an appropriate fighting spot between the two soldiers
    /// Return true on success
    bool GetFightSpotNear(nofActiveSoldier* other, MapPoint* fight_spot);
    /// increase rank if possible
    void IncreaseRank();
    /// Hand back control to derived class after a fight
    virtual void FreeFightEnded();

private:
    /// Soldier reached his military building
    void GoalReached() override;

    /// Get how much of the map/FoW the soldier discovers
    unsigned GetVisualRange() const override;

public:
    nofActiveSoldier(MapPoint pos, unsigned char player, nobBaseMilitary& home, unsigned char rank,
                     SoldierState init_state);
    nofActiveSoldier(const nofSoldier& other, SoldierState init_state);
    nofActiveSoldier(SerializedGameData& sgd, unsigned obj_id);
    nofActiveSoldier(const nofActiveSoldier&) = delete;

    void Destroy() override
    {
        RTTR_Assert(!enemy);
        nofSoldier::Destroy();
    }
    void Serialize(SerializedGameData& sgd) const override;

    /// Draw soldier (for all types of soldiers done by this base class!)
    void Draw(DrawPoint drawPt) override;

    /// Inform the different things that we are not coming anymore
    virtual void InformTargetsAboutCancelling();
    /// Is called when our home military building was destroyed
    virtual void HomeDestroyed() = 0;
    /// When the soldier is still hanging in the going-out waiting queue in the home military building
    virtual void HomeDestroyedAtBegin() = 0;
    /// Takes a hit from another soldier. Reduces hitpoints by one
    void TakeHit();
    /// When a fight was won
    virtual void WonFighting() = 0;
    /// When a fight was lost
    virtual void LostFighting() = 0;

    /// Determine if this soldier is ready for a spontaneous  fight
    bool IsReadyForFight() const;
    /// Inform a waiting soldier about the start of a fight
    void FightingStarted();

    /// Get the current state
    SoldierState GetState() const { return state; }
    /// Set the home (building) to nullptr
    /// e.g. after the soldier was removed from the homes list but it was not destroyed
    void ResetHome() { building = nullptr; }
    void FightVsDefenderStarted() { state = SoldierState::AttackingFightingVsDefender; }

    // For debugging
    const nofActiveSoldier* GetEnemy() const { return enemy; }
};
