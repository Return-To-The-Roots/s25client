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
#ifndef NOF_ATTACKER_H_
#define NOF_ATTACKER_H_

#include "nofActiveSoldier.h"
#include <vector>

class nofDefender;
class nofAggressiveDefender;
class nofPassiveSoldier;
class nobHarborBuilding;
class nobMilitary;
class noShip;
class SerializedGameData;
class nobBaseMilitary;
class GameEvent;

/// Angreifender Soldat
class nofAttacker : public nofActiveSoldier
{
    /// Building which is attacked by the soldier
    nobBaseMilitary* attacked_goal;
    /// Did we got hunted by an aggressive defender?
    bool mayBeHunted;
    /// 0: Will not send, 1: Asked and will send eventually, 2: Not decided
    std::vector<uint8_t> canPlayerSendAggDefender;
    /// Defender who is currently chasing after this soldier
    nofAggressiveDefender* huntingDefender;
    /// In welchem Radius steht der Soldat, wenn er um eine Fahne herum wartet?
    unsigned short radius;
    /// Nach einer bestimmten Zeit, in der der Angreifer an der Flagge des Gebäudes steht, blockt er den Weg
    /// nur benutzt bei STATE_ATTACKING_WAITINGFORDEFENDER
    const GameEvent* blocking_event;

    /// Für Seeangreifer: Stelle, wo sich der Hafen befindet, von wo aus sie losfahren sollen
    MapPoint harborPos;
    /// Für Seeangreifer: Landepunkt, wo sich das Schiff befindet, mit dem der Angreifer
    /// ggf. wieder nach Hause fahren kann
    MapPoint shipPos;
    unsigned ship_obj_id;

    /// wenn man gelaufen ist
    void Walked() override;
    /// Geht nach Hause für Attacking-Missoin
    void ReturnHomeMissionAttacking();
    /// Läuft weiter
    void MissAttackingWalk();
    /// Ist am Militärgebäude angekommen
    void ReachedDestination();
    /// Versucht, eine aggressiven Verteidiger für uns zu bestellen
    void TryToOrderAggressiveDefender();
    /// Actually do order an aggressive defender
    void OrderAggressiveDefender();

    /// Doesn't find a defender at the flag -> Send defenders or capture it
    void ContinueAtFlag();

    /// Geht zum STATE_ATTACKING_WAITINGFORDEFENDER über und meldet gleichzeitig ein Block-Event an
    void SwitchStateAttackingWaitingForDefender();

    /// Für Schiffsangreifer: Sagt dem Schiff Bescheid, dass wir nicht mehr kommen
    void CancelAtShip();
    void CancelAtHuntingDefender();
    /// Behandelt das Laufen zurück zum Schiff
    void HandleState_SeaAttack_ReturnToShip();

    /// The derived classes regain control after a fight of nofActiveSoldier
    void FreeFightEnded() override;

public:
    /// Sagt den verschiedenen Zielen Bescheid, dass wir doch nicht mehr kommen können
    void InformTargetsAboutCancelling() override;

    void RemoveFromAttackedGoal();

    /// Normaler Konstruktor für Angreifer
    nofAttacker(nofPassiveSoldier* other, nobBaseMilitary* const attacked_goal);
    /// Konstruktor für Schiffs-Angreifer, die zuerst einmal zu einem Hafen laufen müssen
    nofAttacker(nofPassiveSoldier* other, nobBaseMilitary* const attacked_goal, const nobHarborBuilding* const harbor);
    nofAttacker(SerializedGameData& sgd, unsigned obj_id);
    ~nofAttacker() override;

    /// Aufräummethoden
protected:
    void Destroy_nofAttacker();

public:
    void Destroy() override { Destroy_nofAttacker(); }

    /// Serialisierungsfunktionen
protected:
    void Serialize_nofAttacker(SerializedGameData& sgd) const;

public:
    void Serialize(SerializedGameData& sgd) const override { Serialize_nofAttacker(sgd); }

    GO_Type GetGOT() const override { return GOT_NOF_ATTACKER; }
    const nofAggressiveDefender* GetHuntingDefender() const { return huntingDefender; }

    void HandleDerivedEvent(unsigned id) override;
    /// Blockt der Angreifer noch?
    bool IsBlockingRoads() const;

    /// Wenn ein Heimat-Militärgebäude bei Missionseinsätzen zerstört wurde
    void HomeDestroyed() override;
    /// Wenn er noch in der Warteschleife vom Ausgangsgebäude hängt und dieses zerstört wurde
    void HomeDestroyedAtBegin() override;

    /// Wenn ein Kampf gewonnen wurde
    void WonFighting() override;
    /// Wenn ein Kampf verloren wurde (Tod)
    void LostFighting() override;

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
    /// Ein um-die-Flagge-wartender Angreifer soll auf einen frei gewordenen Platz nachrücken, damit keine
    /// Lücken entstehen
    void StartSucceeding(const MapPoint pt, unsigned short new_radius);
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
    void StartAttackOnOtherIsland(const MapPoint shipPos, unsigned ship_id);
    /// Sagt Schiffsangreifern, dass sie mit dem Schiff zurück fahren
    void StartReturnViaShip(noShip& ship);
    /// Sea attacker enters harbor and finds no shipping route or no longer has a valid target: return home soon on a road
    void SeaAttackFailedBeforeLaunch();
    /// notify sea attackers that they wont return home
    void HomeHarborLost();
    /// Sagt Bescheid, dass sich die Angreifer nun auf dem Schiff befinden
    void SeaAttackStarted() { state = STATE_SEAATTACKING_ONSHIP; }
    /// Fragt einen Schiffs-Angreifer auf dem Schiff, ob er schon einmal
    /// draußen war und gekämpft hat
    bool IsSeaAttackCompleted() const { return (state != STATE_SEAATTACKING_ONSHIP); }
    /// Bricht einen Seeangriff ab
    void CancelSeaAttack();
};

#endif // !NOF_ATTACKER_H_
