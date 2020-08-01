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

#ifndef WARE_H_
#define WARE_H_

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
    enum State
    {
        STATE_WAITINWAREHOUSE = 0, // Ware wartet im Lagerhaus noch auf Auslagerun
        STATE_WAITATFLAG,          // Ware liegt an einer Fahne und wartet auf den Träger, der kommt
        STATE_CARRIED,             // Ware wird von einem Träger getragen
        STATE_WAITFORSHIP,         // Ware wartet im Hafengebäude auf das Schiff, das sie abholt
        STATE_ONSHIP               // Ware befindet sich auf einem Schiff
    } state;
    /// Auf welcher Flagge, in welchem Gebäude die Ware gerade ist (bei STATE_CARRIED ist es die Flagge, zu der die Ware getragen wird!)
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

    /// Serialisierungsfunktionen
protected:
    void Serialize_Ware(SerializedGameData& sgd) const;

public:
    void Serialize(SerializedGameData& sgd) const override { Serialize_Ware(sgd); }

    GO_Type GetGOT() const override { return GOT_WARE; }

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
    bool IsWaitingAtFlag() const { return (state == STATE_WAITATFLAG); }
    bool IsWaitingInWarehouse() const { return (state == STATE_WAITINWAREHOUSE); }
    bool IsWaitingForShip() const { return (state == STATE_WAITFORSHIP); }
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
    bool IsLostWare() const { return ((goal ? false : true) && state != STATE_ONSHIP); }
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
            case GD_AXE: return ("GD_AXE");
            case GD_BEER: return ("GD_BEER");
            case GD_BOARDS: return ("GD_BOARDS");
            case GD_BOAT: return ("GD_BOAT");
            case GD_BOW: return ("GD_BOW");
            case GD_BREAD: return ("GD_BREAD");
            case GD_CLEAVER: return ("GD_CLEAVER");
            case GD_COAL: return ("GD_COAL");
            case GD_COINS: return ("GD_COINS");
            case GD_CRUCIBLE: return ("GD_CRUCIBLE");
            case GD_FISH: return ("GD_FISH");
            case GD_FLOUR: return ("GD_FLOUR");
            case GD_GOLD: return ("GD_GOLD");
            case GD_GRAIN: return ("GD_GRAIN");
            case GD_HAM: return ("GD_HAM");
            case GD_HAMMER: return ("GD_HAMMER");
            case GD_IRON: return ("GD_IRON");
            case GD_IRONORE: return ("GD_IRONORE");
            case GD_MEAT: return ("GD_MEAT");
            case GD_NOTHING: return ("GD_NOTHING");
            case GD_PICKAXE: return ("GD_PICKAXE");
            case GD_RODANDLINE: return ("GD_RODANDLINE");
            case GD_ROLLINGPIN: return ("GD_ROLLINGPIN");
            case GD_SAW: return ("GD_SAW");
            case GD_SCYTHE: return ("GD_SCYTHE");
            case GD_SHIELDAFRICANS: return ("GD_SHIELDAFRICANS");
            case GD_SHIELDJAPANESE: return ("GD_SHIELDJAPANESE");
            case GD_SHIELDROMANS: return ("GD_SHIELDROMANS");
            case GD_SHIELDVIKINGS: return ("GD_SHIELDVIKINGS");
            case GD_SHOVEL: return ("GD_SHOVEL");
            case GD_STONES: return ("GD_STONES");
            case GD_SWORD: return ("GD_SWORD");
            case GD_TONGS: return ("GD_TONGS");
            case GD_WATER: return ("GD_WATER");
            case GD_WATEREMPTY: return ("GD_WATEREMPTY");
            case GD_WOOD: return ("GD_WOOD");
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

#endif
