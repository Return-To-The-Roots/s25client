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

#include "nofSoldier.h"
#include <cstdint>

class SerializedGameData;
class nobBaseMilitary;

/// Base class for all 3 types of "active" soldiers (i.e. who are in the free world to fight and don't
/// walk on the roads just to fill buildings)
/// Attackers, defenders and aggressive defenders
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
        Fighting,        // Fighting

        AttackingWalkingToGoal,         // Attacker is walking to his attacked destination
        AttackingWaitingAroundBuilding, // Attacker is waiting around the building for his fight at the flag
                                        // against the defender(s)
        AttackingWaitingForDefender,    // Waiting at the flag until the defender emerges from the building
        AttackingCapturingFirst,        // Captures the hostile building as first person
        AttackingCapturingNext,         // The next soldiers capture the building in this state
        AttackingAttackingFlag,         // Goes to the flag to fight the defender
        AttackingFightingVsDefender,    // Fighting against a defender at the flag

        SeaattackingGoToHarbor,   // Goes from his home military building to the start harbor
        SeaattackingWaitInHarbor, // Waiting in the start harbor for the ship
        SeaattackingOnShip,       // On the ship to the destination
        SeaattackingReturnToShip, // Returns to the ship at the destination environment

        AggressivedefendingWalkingToAggressor, // Follow the attacker in order to fight against him

        DefendingWaiting,    // Waiting at the flag for further attackers
        DefendingWalkingTo,  // Goes to the flag before the fight
        DefendingWalkingFrom // Goes into the building after the fight
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

    /// Examines hostile people on roads and expels them
    void ExpelEnemies();

    /// Handle walking for nofActiveSoldier speciefic sates
    void Walked() override;

    /// Looks for enemies nearby which want to fight with this soldier
    /// Returns true if it found one
    bool FindEnemiesNearby(unsigned char excludedOwner = 255);
    /// Informs this soldier that another soldier starts meeting him
    void MeetEnemy(nofActiveSoldier* other, MapPoint figh_spot);
    /// Handle state "meet enemy" after each walking step
    void MeetingEnemy();
    /// Looks for an appropriate fighting spot between the two soldiers
    /// Returns true if successful
    bool GetFightSpotNear(nofActiveSoldier* other, MapPoint* fight_spot);
    /// increase rank (used by addon CombatPromotion)
    void IncreaseRank();
    /// The derived classes regain control after a fight of nofActiveSoldier
    virtual void FreeFightEnded();

private:
    /// Is informed when...
    void GoalReached() override; // ... he reached his "working place" (i.e. his military building)

    /// Gets the visual range radius of this soldier
    unsigned GetVisualRange() const override;

public:
    nofActiveSoldier(MapPoint pos, unsigned char player, nobBaseMilitary* home, unsigned char rank,
                     SoldierState init_state);
    nofActiveSoldier(const nofSoldier& other, SoldierState init_state);
    nofActiveSoldier(SerializedGameData& sgd, unsigned obj_id);

    /// Tidy up
protected:
    void Destroy_nofActiveSoldier()
    {
        RTTR_Assert(!enemy);
        Destroy_nofSoldier();
    }

public:
    void Destroy() override { Destroy_nofActiveSoldier(); }

    /// Serializer
protected:
    void Serialize_nofActiveSoldier(SerializedGameData& sgd) const;

public:
    void Serialize(SerializedGameData& sgd) const override { Serialize_nofActiveSoldier(sgd); }

    /// Draw soldier (for all types of soldiers done by this base class!)
    void Draw(DrawPoint drawPt) override;

    /// Informs the different things that we are not coming anymore
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

    /// Determines if this soldier is ready for a spontaneous  fight
    bool IsReadyForFight() const;
    /// Informs a waiting soldier about the start of a fight
    void FightingStarted();

    /// Gets the current state
    SoldierState GetState() const { return state; }
    /// Sets the home (building) to nullptr e.g. after the soldier was removed from the homes list but it was not
    /// destroyed
    void ResetHome() { building = nullptr; }
    void FightVsDefenderStarted() { state = SoldierState::AttackingFightingVsDefender; }

    // For debugging
    const nofActiveSoldier* GetEnemy() const { return enemy; }
};
