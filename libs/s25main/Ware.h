// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "GameObject.h"
#include "RTTR_Assert.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/RoadPathDirection.h"

class noBaseBuilding;
class nobHarborBuilding;
class nobBaseWarehouse;
class noRoadNode;
class noFlag;
class SerializedGameData;

// Die Klasse Ware kennzeichnet eine Ware, die von einem Träger transportiert wird bzw gerade an einer Flagge liegt
class Ware : public GameObject
{
    /// Die Richtung von der Fahne auf dem Weg, auf dem die Ware transportiert werden will als nächstes
    RoadPathDirection next_dir;
    /// In welchem Status die Ware sich gerade befindet
    enum class State : uint8_t
    {
        WaitInWarehouse, // Ware wartet im Lagerhaus noch auf Auslagerun
        WaitAtFlag,      // Ware liegt an einer Fahne und wartet auf den Träger, der kommt
        Carried,         // Ware wird von einem Träger getragen
        WaitForShip,     // Ware wartet im Hafengebäude auf das Schiff, das sie abholt
        OnShip           // Ware befindet sich auf einem Schiff
    } state;
    friend constexpr auto maxEnumValue(State) { return State::OnShip; }
    /// Auf welcher Flagge, in welchem Gebäude die Ware gerade ist (bei Carried ist es die Flagge, zu der die Ware
    /// getragen wird!)
    noRoadNode* location;

public:
    /// Was für eine Ware
    const GoodType type;

private:
    /// Wo die Ware mal hin soll
    noBaseBuilding* goal;
    /// Nächster Hafenpunkt, der ggf. angesteuert werden soll
    MapPoint next_harbor;

public:
    Ware(GoodType type, noBaseBuilding* goal, noRoadNode* location);
    Ware(SerializedGameData& sgd, unsigned obj_id);

    ~Ware() override;

    void Destroy() override;
    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const override { return GO_Type::Ware; }

    RoadPathDirection GetNextDir() const { return next_dir; }
    /// Gibt nächsten Hafen zurück, falls vorhanden
    MapPoint GetNextHarbor() const { return next_harbor; }
    noBaseBuilding* GetGoal() const { return goal; }
    /// Sets the new goal and notifies it
    void SetGoal(noBaseBuilding* newGoal);
    /// Berechnet den Weg neu zu ihrem Ziel
    void RecalcRoute();
    /// set new next dir
    void SetNextDir(RoadPathDirection newNextDir) { next_dir = newNextDir; }
    void SetNextDir(Direction newNextDir) { next_dir = toRoadPathDirection(newNextDir); }
    /// Wird aufgerufen, wenn es das Ziel der Ware nicht mehr gibt und sie wieder "nach Hause" getragen werden muss
    void GoalDestroyed();
    /// Changes the state of the ware
    void WaitAtFlag(noFlag* flag);
    void WaitInWarehouse(nobBaseWarehouse* wh);
    void Carry(noRoadNode* nextGoal);
    /// Gibt dem Ziel der Ware bekannt, dass diese nicht mehr kommen kann
    void NotifyGoalAboutLostWare();
    /// Wenn die Ware vernichtet werden muss
    void WareLost(unsigned char player);
    /// Gibt Status der Ware zurück
    bool IsWaitingAtFlag() const { return (state == State::WaitAtFlag); }
    bool IsWaitingInWarehouse() const { return (state == State::WaitInWarehouse); }
    bool IsWaitingForShip() const { return (state == State::WaitForShip); }
    /// Sagt dem Träger Bescheid, dass sie in die aktuelle (next_dir) Richtung nicht mehr getragen werden will
    void RemoveWareJobForDir(RoadPathDirection last_next_dir);
    /// Überprüft, ob es noch ein Weg zum Ziel gibt
    bool IsRouteToGoal();
    /// Tells the ware that it should look for a warehouse to go to and notifies that (if found)
    /// Sets nextDir to the next direction or invalid of no warehouse found
    bool FindRouteToWarehouse();
    /// Tells a carrier that we want to be carried
    void CallCarrier();
    /// a building is looking for a ware - check if this lost ware can be send to the building and then do it
    unsigned CheckNewGoalForLostWare(const noBaseBuilding& newgoal) const;
    void SetNewGoalForLostWare(noBaseBuilding* newgoal);
    /// Gibt Ort der Ware zurück
    noRoadNode* GetLocation() { return location; }
    const noRoadNode* GetLocation() const { return location; }
    /// Ist die Ware eine LostWare (Ware, die kein Ziel mehr hat und irgendwo sinnlos rumliegt)?
    bool IsLostWare() const { return ((goal ? false : true) && state != State::OnShip); }
    /// Informiert Ware, dass eine Schiffsreise beginnt
    void StartShipJourney();
    /// Informiert Ware, dass Schiffsreise beendet ist und die Ware nun in einem Hafengebäude liegt
    void ShipJorneyEnded(nobHarborBuilding* hb);
    /// Beginnt damit auf ein Schiff im Hafen zu warten
    void WaitForShip(nobHarborBuilding* hb);

    std::string ToString() const override;

    static std::string GoodType2String(GoodType value)
    {
        switch(value)
        {
            case GoodType::Axe: return ("GoodType::Axe");
            case GoodType::Beer: return ("GoodType::Beer");
            case GoodType::Boards: return ("GoodType::Boards");
            case GoodType::Boat: return ("GoodType::Boat");
            case GoodType::Bow: return ("GoodType::Bow");
            case GoodType::Bread: return ("GoodType::Bread");
            case GoodType::Cleaver: return ("GoodType::Cleaver");
            case GoodType::Coal: return ("GoodType::Coal");
            case GoodType::Coins: return ("GoodType::Coins");
            case GoodType::Crucible: return ("GoodType::Crucible");
            case GoodType::Fish: return ("GoodType::Fish");
            case GoodType::Flour: return ("GoodType::Flour");
            case GoodType::Gold: return ("GoodType::Gold");
            case GoodType::Grain: return ("GoodType::Grain");
            case GoodType::Ham: return ("GoodType::Ham");
            case GoodType::Hammer: return ("GoodType::Hammer");
            case GoodType::Iron: return ("GoodType::Iron");
            case GoodType::IronOre: return ("GoodType::IronOre");
            case GoodType::Meat: return ("GoodType::Meat");
            case GoodType::Nothing: return ("GoodType::Nothing");
            case GoodType::PickAxe: return ("GoodType::PickAxe");
            case GoodType::RodAndLine: return ("GoodType::RodAndLine");
            case GoodType::Rollingpin: return ("GoodType::Rollingpin");
            case GoodType::Saw: return ("GoodType::Saw");
            case GoodType::Scythe: return ("GoodType::Scythe");
            case GoodType::ShieldAfricans: return ("GoodType::ShieldAfricans");
            case GoodType::ShieldJapanese: return ("GoodType::ShieldJapanese");
            case GoodType::ShieldRomans: return ("GoodType::ShieldRomans");
            case GoodType::ShieldVikings: return ("GoodType::ShieldVikings");
            case GoodType::Shovel: return ("GoodType::Shovel");
            case GoodType::Stones: return ("GoodType::Stones");
            case GoodType::Sword: return ("GoodType::Sword");
            case GoodType::Tongs: return ("GoodType::Tongs");
            case GoodType::Water: return ("GoodType::Water");
            case GoodType::WaterEmpty: return ("GoodType::WaterEmpty");
            case GoodType::Wood: return ("GoodType::Wood");
        }
        RTTR_Assert(false);
        return ("unknown");
    }

private:
    struct RouteParams
    {
        unsigned length;
        RoadPathDirection dir;
    };
    RouteParams CalcPathToGoal(const noBaseBuilding& newgoal) const;
};
