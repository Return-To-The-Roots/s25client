// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef NOF_ACTIVESOLDIER_H_
#define NOF_ACTIVESOLDIER_H_

#include "nofSoldier.h"

/// Base class for all 3 types of "active" soldiers (i.e. who are in the free world to fight and don't
/// walk on the roads just to fill buildings)
/// Attackers, defenders and aggressive defenders
class nofActiveSoldier : public nofSoldier
{
    public:

        friend class noFighting;

        /// State of each soldier
        enum SoldierState
        {
            STATE_FIGUREWORK = 0, /// Go to work etc., all which is done by noFigure
            STATE_WALKINGHOME, /// Walking home after work to the military building
            STATE_MEETENEMY, /// Prepare fighting with an enemy
            STATE_WAITINGFORFIGHT, /// Standing still and waiting for a fight
            STATE_FIGHTING, // Fighting

            STATE_ATTACKING_WALKINGTOGOAL, // Attacker is walking to his attacked destination
            STATE_ATTACKING_WAITINGAROUNDBUILDING, // Attacker is waiting around the building for his fight at the flag against the defender(s)
            STATE_ATTACKING_WAITINGFORDEFENDER, // Waiting at the flag until the defender emerges from the building
            STATE_ATTACKING_CAPTURINGFIRST, // Captures the hostile building as first person
            STATE_ATTACKING_CAPTURINGNEXT, // The next soldiers capture the building in this state
            STATE_ATTACKING_ATTACKINGFLAG, // Goes to the flag to fight the defender
            STATE_ATTACKING_FIGHTINGVSDEFENDER, // Fighting against a defender at the flag

            STATE_SEAATTACKING_GOTOHARBOR, // Goes from his home military building to the start harbor
            STATE_SEAATTACKING_WAITINHARBOR, // Waiting in the start harbor for the ship
            STATE_SEAATTACKING_ONSHIP, // On the ship to the destination
            STATE_SEAATTACKING_RETURNTOSHIP, // Returns to the ship at the destination environment

            STATE_AGGRESSIVEDEFENDING_WALKINGTOAGGRESSOR, // Follow the attacker in order to fight against him

            STATE_DEFENDING_WAITING, // Waiting at the flag for further attackers
            STATE_DEFENDING_WALKINGTO, // Goes to the flag before the fight
            STATE_DEFENDING_WALKINGFROM // Goes into the building after the fight

        };

    protected:

        /// State of the soldier, always has to be a valid value
        enum SoldierState state;

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
        virtual void Walked();

        /// Looks for enemies nearby which want to fight with this soldier
        /// Returns true if it found one
        bool FindEnemiesNearby(unsigned char excludedOwner=255);
        /// Informs this soldier that another soldier starts meeting him
        void MeetEnemy(nofActiveSoldier* other, const MapPoint figh_spot);
        /// Handle state "meet enemy" after each walking step
        void MeetingEnemy();
        /// Looks for an appropriate fighting spot between the two soldiers
        /// Returns true if successful
        bool GetFightSpotNear(nofActiveSoldier* other, MapPoint * fight_spot);
		/// increase rank (used by addon CombatPromotion)
		void IncreaseRank();
        /// The derived classes regain control after a fight of nofActiveSoldier
        virtual void FreeFightEnded() = 0;

    private:

        /// Is informed when...
        void GoalReached(); // ... he reached his "working place" (i.e. his military building)

        /// Gets the visual range radius of this soldier
        virtual unsigned GetVisualRange() const;

    public:

        /// Constructors
        nofActiveSoldier(const MapPoint pt, const unsigned char player,
                         nobBaseMilitary* const home, const unsigned char rank, const SoldierState init_state);
        /// (Copy-)Constructor
        nofActiveSoldier(const nofSoldier& other, const SoldierState init_state);
        /// Deserializer
        nofActiveSoldier(SerializedGameData* sgd, const unsigned obj_id);

        /// Tidy up
    protected:  void Destroy_nofActiveSoldier() { Destroy_nofSoldier(); }
    public:     void Destroy() { Destroy_nofActiveSoldier(); }

        /// Serializer
    protected:  void Serialize_nofActiveSoldier(SerializedGameData* sgd) const;
    public:     void Serialize(SerializedGameData* sgd) const { Serialize_nofActiveSoldier(sgd); }

        /// Draw soldier (for all types of soldiers done by this base class!)
        void Draw(int x, int y);

        /// Event handling
        virtual void HandleDerivedEvent(const unsigned int id);

        /// Informs the different things that we are not coming anymore
        virtual void InformTargetsAboutCancelling() = 0;
        /// Is called when our home military building was destroyed
        virtual void HomeDestroyed() = 0;
        /// When the soldier is still hanging in the going-out waiting queue in the home military building
        virtual void HomeDestroyedAtBegin() = 0;
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
};

#endif // !NOF_ACTIVESOLDIER_H_
