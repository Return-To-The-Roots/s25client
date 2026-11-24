// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
    Ware(const Ware&) = delete;

    ~Ware() override;

    void destroy() override;
    void serialize(SerializedGameData& sgd) const override;

    GO_Type getGOT() const final { return GO_Type::Ware; }

    RoadPathDirection getNextDir() const { return next_dir; }
    /// Gibt nächsten Hafen zurück, falls vorhanden
    MapPoint getNextHarbor() const { return next_harbor; }
    noBaseBuilding* getGoal() const { return goal; }
    /// Sets the new goal and notifies it
    void setGoal(noBaseBuilding* newGoal);
    /// Berechnet den Weg neu zu ihrem Ziel
    void recalcRoute();
    /// set new next dir
    void setNextDir(RoadPathDirection newNextDir) { next_dir = newNextDir; }
    void setNextDir(Direction newNextDir) { next_dir = toRoadPathDirection(newNextDir); }
    /// Wird aufgerufen, wenn es das Ziel der Ware nicht mehr gibt und sie wieder "nach Hause" getragen werden muss
    void goalDestroyed();
    /// Changes the state of the ware
    void waitAtFlag(noFlag& flag);
    void waitInWarehouse(nobBaseWarehouse& wh);
    void carry(noRoadNode& nextGoal);
    /// Gibt dem Ziel der Ware bekannt, dass diese nicht mehr kommen kann
    void notifyGoalAboutLostWare();
    /// Wenn die Ware vernichtet werden muss
    void wareLost(unsigned char player);
    /// Gibt Status der Ware zurück
    bool isWaitingAtFlag() const { return (state == State::WaitAtFlag); }
    bool isWaitingInWarehouse() const { return (state == State::WaitInWarehouse); }
    bool isWaitingForShip() const { return (state == State::WaitForShip); }
    bool isCarried() const { return (state == State::Carried); }
    /// Sagt dem Träger Bescheid, dass sie in die aktuelle (next_dir) Richtung nicht mehr getragen werden will
    void removeWareJobForDir(RoadPathDirection lastNextDir);
    /// Überprüft, ob es noch ein Weg zum Ziel gibt
    bool isRouteToGoal();
    /// Tells the ware that it should look for a warehouse to go to and notifies that (if found)
    /// Sets nextDir to the next direction or invalid if no warehouse found
    bool findRouteToWarehouse();
    /// Tells a carrier that we want to be carried
    void callCarrier();
    /// a building is looking for a ware - check if this lost ware can be send to the building and then do it
    unsigned checkNewGoalForLostWare(const noBaseBuilding& newgoal) const;
    void setNewGoalForLostWare(noBaseBuilding& newgoal);
    /// Gibt Ort der Ware zurück
    noRoadNode* getLocation() { return location; }
    const noRoadNode* getLocation() const { return location; }
    /// Ist die Ware eine LostWare (Ware, die kein Ziel mehr hat und irgendwo sinnlos rumliegt)?
    bool isLostWare() const { return ((goal == nullptr) && state != State::OnShip); }
    /// Informiert Ware, dass eine Schiffsreise beginnt
    void startShipJourney();
    /// Informiert Ware, dass Schiffsreise beendet ist und die Ware nun in einem Hafengebäude liegt
    void shipJorneyEnded(nobHarborBuilding* hb);
    /// Beginnt damit auf ein Schiff im Hafen zu warten
    void waitForShip(nobHarborBuilding* hb);

    std::string toString() const override;

    static std::string goodType2String(GoodType value)
    {
        switch(value)
        {
            case GoodType::Armor: return ("GoodType::Armor");
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
            case GoodType::Grapes: return ("GoodType::Grapes");
            case GoodType::Ham: return ("GoodType::Ham");
            case GoodType::Hammer: return ("GoodType::Hammer");
            case GoodType::Iron: return ("GoodType::Iron");
            case GoodType::IronOre: return ("GoodType::IronOre");
            case GoodType::Leather: return ("GoodType::Leather");
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
            case GoodType::Skins: return ("GoodType::Skins");
            case GoodType::Stones: return ("GoodType::Stones");
            case GoodType::Sword: return ("GoodType::Sword");
            case GoodType::Tongs: return ("GoodType::Tongs");
            case GoodType::Water: return ("GoodType::Water");
            case GoodType::WaterEmpty: return ("GoodType::WaterEmpty");
            case GoodType::Wine: return ("GoodType::Wine");
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
    RouteParams calcPathToGoal(const noBaseBuilding& newgoal) const;
};
