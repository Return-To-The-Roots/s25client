// $Id: nofAttacker.h 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef NOF_ATTACKER_H_
#define NOF_ATTACKER_H_

#include "nofActiveSoldier.h"

class nofDefender;
class nofAggressiveDefender;
class nofPassiveSoldier;
class nobHarborBuilding;
class nobMilitary;

/// Angreifender Soldat
class nofAttacker : public nofActiveSoldier
{
        // Unsere Feind-Freunde ;)
        friend class nofAggressiveDefender;
        friend class nofDefender;

    private:

        /// Building which is attacked by the soldier
        nobBaseMilitary* attacked_goal;
        /// Soll er von nem Verteidiger gejagt werden? (wenn nicht wurde er schon gejagt oder er soll
        /// wegen den Militäreinstellungen nicht gejagt werden
        bool should_haunted;
        /// In welchem Radius steht der Soldat, wenn er um eine Fahne herum wartet?
        unsigned short radius;
        /// Nach einer bestimmten Zeit, in der der Angreifer an der Flagge des Gebäudes steht, blockt er den Weg
        /// nur benutzt bei STATE_ATTACKING_WAITINGFORDEFENDER
        EventManager::EventPointer blocking_event;

        /// Für Seeangreifer: Stelle, wo sich der Hafen befindet, von wo aus sie losfahren sollen
        MapCoord harbor_x, harbor_y;
        /// Für Seeangreifer: Landepunkt, wo sich das Schiff befindet, mit dem der Angreifer
        /// ggf. wieder nach Hause fahren kann
        MapCoord ship_x, ship_y;
        unsigned ship_obj_id;

    private:

        /// wenn man gelaufen ist
        void Walked();
        /// Geht nach Hause für Attacking-Missoin
        void ReturnHomeMissionAttacking();
        /// Läuft weiter
        void MissAttackingWalk();
        /// Ist am Militärgebäude angekommen
        void ReachedDestination();
        /// Versucht, eine aggressiven Verteidiger für uns zu bestellen
        void TryToOrderAggressiveDefender();
        /// Doesn't find a defender at the flag -> Send defenders or capture it
        void ContinueAtFlag();

        /// Geht zum STATE_ATTACKING_WAITINGFORDEFENDER über und meldet gleichzeitig ein Block-Event an
        void SwitchStateAttackingWaitingForDefender();





        /// Für Schiffsangreifer: Sagt dem Schiff Bescheid, dass wir nicht mehr kommen
        void CancelAtShip();
        /// Behandelt das Laufen zurück zum Schiff
        void HandleState_SeaAttack_ReturnToShip();

        /// The derived classes regain control after a fight of nofActiveSoldier
        void FreeFightEnded();

    public:
        /// Sagt den verschiedenen Zielen Bescheid, dass wir doch nicht mehr kommen können
        void InformTargetsAboutCancelling();
        /// Normaler Konstruktor für Angreifer
        nofAttacker(nofPassiveSoldier* other, nobBaseMilitary* const attacked_goal);
        /// Konstruktor für Schiffs-Angreifer, die zuerst einmal zu einem Hafen laufen müssen
        nofAttacker(nofPassiveSoldier* other, nobBaseMilitary* const attacked_goal,
                    const nobHarborBuilding* const harbor);
        nofAttacker(SerializedGameData* sgd, const unsigned obj_id);
        ~nofAttacker();

        /// Aufräummethoden
    protected:  void Destroy_nofAttacker();
    public:     void Destroy() { Destroy_nofAttacker(); }

        /// Serialisierungsfunktionen
    protected:  void Serialize_nofAttacker(SerializedGameData* sgd) const;
    public:     void Serialize(SerializedGameData* sgd) const { Serialize_nofAttacker(sgd); }

        GO_Type GetGOT() const { return GOT_NOF_ATTACKER; }

        void HandleDerivedEvent(const unsigned int id);
        /// Blockt der Angreifer noch?
        bool IsBlockingRoads() const;

        /// Wenn ein Heimat-Militärgebäude bei Missionseinsätzen zerstört wurde
        void HomeDestroyed();
        /// Wenn er noch in der Warteschleife vom Ausgangsgebäude hängt und dieses zerstört wurde
        void HomeDestroyedAtBegin();
        /// Sagt dem Heimatgebäude Bescheid, dass er nicht mehr nach Hause kommen wird
        void CancelAtHomeMilitaryBuilding();

        /// Wenn ein Kampf gewonnen wurde
        void WonFighting();
        /// Wenn ein Kampf verloren wurde (Tod)
        void LostFighting();

        /// Sagt einem angreifenden Soldaten, dass es das Ziel nich mehr gibt
        void AttackedGoalDestroyed();
        /// aggressiv Verteidigender Soldat kann nich mehr kommen
        void AggressiveDefenderLost();
        /// Der Angreifer, der gerade um die Fahne wartet soll zur Fahne laufen, wo der Verteidiger wartet und dort kämpfen
        bool AttackFlag(nofDefender* defender);
        /// Der Platz von einem anderen Kampf wurde frei --> angreifender Soldat soll hinlaufen (KEIN Verteidiger da!)
        void AttackFlag();
        /// Ein um-die-Flagge-wartender Angreifer soll, nachdem das Militärgebäude besetzt wurde, ebenfalls mit reingehen
        void CaptureBuilding();
        /// Siehe oben, wird nach jeder Wegeinheit aufgerufen
        void CapturingWalking();
        /// Ein um-die-Flagge-wartender Angreifer soll wieder nach Hause gehen, da das eingenommene Gebäude bereits
        /// voll besetzt ist
        void CapturedBuildingFull();
        /// Gibt den Radius von einem um-eine-Fahne-herum-wartenden angreifenden Soldaten zurück
        unsigned GetRadius() const { return radius; }
        /// Ein um-die-Flagge-Swartender Angreifer soll auf einen frei gewordenen Platz nachrücken, damit keine
        /// Lücken entstehen
        void StartSucceeding(const unsigned short x, const unsigned short y, const unsigned short new_radius, const unsigned char dir);
        /// Siehe oben, wird nach jeder Wegeinheit aufgerufen
        void SucceedingWalk();
        /// Try to start capturing although he is still far away from the destination
        /// Returns true if successful
        bool TryToStartFarAwayCapturing(nobMilitary* dest);

        /// aggressiv-verteidigender Soldat will mit einem Angreifer kämpfen oder umgekehrt
        void LetsFight(nofAggressiveDefender* other);

        /// Fragt, ob ein Angreifender Soldat vor dem Gebäude wartet und kämpfen will
        bool IsAttackerReady() const { return (state == STATE_ATTACKING_WAITINGAROUNDBUILDING); }

        /// Liefert das angegriffene Gebäude zurück
        nobBaseMilitary* GetAttackedGoal() const { return attacked_goal; }

        /// Startet den Angriff am Landungspunkt vom Schiff
        void StartAttackOnOtherIsland(const MapCoord ship_x, const MapCoord ship_y, const unsigned ship_id);
        /// Sagt Schiffsangreifern, dass sie mit dem Schiff zurück fahren
        void StartReturnViaShip();
        /// Sea attacker enters harbor and finds no shipping route or no longer has a valid target: return home soon on a road
        void SeaAttackFailedBeforeLaunch();
        /// notify sea attackers that they wont return home
        void HomeHarborLost();
        /// Sagt Bescheid, dass sich die Angreifer nun auf dem Schiff befinden
        void SeaAttackStarted()
        { state = STATE_SEAATTACKING_ONSHIP; }
        /// Fragt einen Schiffs-Angreifer auf dem Schiff, ob er schon einmal
        /// draußen war und gekämpft hat
        bool IsSeaAttackCompleted() const { return (state != STATE_SEAATTACKING_ONSHIP); }
        /// Bricht einen Seeangriff ab
        void CancelSeaAttack();


};


#endif // !NOF_ATTACKER_H_
