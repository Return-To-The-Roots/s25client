// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Ware.h"
#include "EventManager.h"
#include "GamePlayer.h"
#include "RoadSegment.h"
#include "SerializedGameData.h"
#include "buildings/noBaseBuilding.h"
#include "buildings/noBuilding.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobHarborBuilding.h"
#include "world/GameWorld.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noRoadNode.h"
#include "gameData/BuildingProperties.h"
#include "gameData/GameConsts.h"
#include "gameData/GoodConsts.h"
#include "gameData/ShieldConsts.h"
#include "s25util/Log.h"
#include <sstream>

Ware::Ware(const GoodType type, noBaseBuilding* goal, noRoadNode* location)
    : next_dir(RoadPathDirection::None), state(State::WaitInWarehouse), location(location),
      type(convertShieldToNation(type,
                                 world->GetPlayer(location->GetPlayer()).nation)), // Use nation specific shield
      goal(goal), next_harbor(MapPoint::Invalid())
{
    RTTR_Assert(location);
    // Ware in den Index mit eintragen
    world->GetPlayer(location->GetPlayer()).RegisterWare(*this);
    if(goal)
        goal->TakeWare(this);
}

Ware::~Ware() = default;

void Ware::Destroy()
{
    RTTR_Assert(!goal);
    RTTR_Assert(!location);
#if RTTR_ENABLE_ASSERTS
    for(unsigned p = 0; p < world->GetNumPlayers(); p++)
    {
        RTTR_Assert(!world->GetPlayer(p).IsWareRegistred(*this));
        RTTR_Assert(!world->GetPlayer(p).IsWareDependent(*this));
    }
#endif
}

void Ware::Serialize(SerializedGameData& sgd) const
{
    sgd.PushEnum<uint8_t>(next_dir);
    sgd.PushEnum<uint8_t>(state);
    sgd.PushObject(location);
    sgd.PushEnum<uint8_t>(type);
    sgd.PushObject(goal);
    helpers::pushPoint(sgd, next_harbor);
}

static RoadPathDirection PopRoadPathDirection(SerializedGameData& sgd)
{
    if(sgd.GetGameDataVersion() < 5)
    {
        const auto iDir = sgd.PopUnsignedChar();
        if(iDir == 100)
            return RoadPathDirection::Ship;
        if(iDir == 0xFF)
            return RoadPathDirection::None;
        if(iDir > helpers::MaxEnumValue_v<Direction>)
            throw SerializedGameData::Error("Invalid RoadPathDirection");
        return RoadPathDirection(iDir);
    } else
        return sgd.Pop<RoadPathDirection>();
}

Ware::Ware(SerializedGameData& sgd, const unsigned obj_id)
    : GameObject(sgd, obj_id), next_dir(PopRoadPathDirection(sgd)), state(sgd.Pop<State>()),
      location(sgd.PopObject<noRoadNode>()), type(sgd.Pop<GoodType>()), goal(sgd.PopObject<noBaseBuilding>()),
      next_harbor(sgd.PopMapPoint())
{}

void Ware::SetGoal(noBaseBuilding* newGoal)
{
    goal = newGoal;
    if(goal)
        goal->TakeWare(this);
}

void Ware::RecalcRoute()
{
    // Nächste Richtung nehmen
    if(location && goal)
        next_dir = world->FindPathForWareOnRoads(*location, *goal, nullptr, &next_harbor);
    else
        next_dir = RoadPathDirection::None;

    // Evtl gibts keinen Weg mehr? Dann wieder zurück ins Lagerhaus (wenns vorher überhaupt zu nem Ziel ging)
    if(next_dir == RoadPathDirection::None && goal)
    {
        RTTR_Assert(location);
        // Tell goal about this
        NotifyGoalAboutLostWare();
        if(state == State::WaitForShip)
        {
            // Ware was waiting for a ship so send the ware into the harbor
            RTTR_Assert(location->GetGOT() == GO_Type::NobHarborbuilding);
            state = State::WaitInWarehouse;
            SetGoal(static_cast<nobHarborBuilding*>(location));
            // but not going by ship
            static_cast<nobHarborBuilding*>(goal)->WareDontWantToTravelByShip(this);
        } else
        {
            // TODO(Replay) This should calculate the next dir even when carried
            FindRouteToWarehouse();
        }
    } else
    {
        // If we waited in the harbor for the ship before and don't want to travel now
        // -> inform the harbor so that it can remove us from its list
        if(state == State::WaitForShip && next_dir != RoadPathDirection::Ship)
        {
            RTTR_Assert(location);
            RTTR_Assert(location->GetGOT() == GO_Type::NobHarborbuilding);
            state = State::WaitInWarehouse;
            static_cast<nobHarborBuilding*>(location)->WareDontWantToTravelByShip(this);
        }
    }
}

void Ware::GoalDestroyed()
{
    if(state == State::WaitInWarehouse)
    {
        // Ware ist noch im Lagerhaus auf der Warteliste
        RTTR_Assert(false); // Should not happen. noBaseBuilding::WareNotNeeded handles this case!
        goal = nullptr; // just in case: avoid corruption although the ware itself might be lost (won't ever be carried
                        // again)
    }
    // Ist sie evtl. gerade mit dem Schiff unterwegs?
    else if(state == State::OnShip)
    {
        // Ziel zunächst auf nullptr setzen, was dann vom Zielhafen erkannt wird,
        // woraufhin dieser die Ware gleich in sein Inventar mit übernimmt
        goal = nullptr;
    }
    // Oder wartet sie im Hafen noch auf ein Schiff
    else if(state == State::WaitForShip)
    {
        // Dann dem Hafen Bescheid sagen
        RTTR_Assert(location);
        RTTR_Assert(location->GetGOT() == GO_Type::NobHarborbuilding);
        // This also adds the ware to the harbors inventory
        auto ownedWare = static_cast<nobHarborBuilding*>(location)->CancelWareForShip(this);
        // Kill the ware
        world->GetPlayer(location->GetPlayer()).RemoveWare(*this);
        goal = nullptr;
        location = nullptr;
        GetEvMgr().AddToKillList(std::move(ownedWare));
    } else
    {
        // Ware ist unterwegs, Lagerhaus finden und Ware dort einliefern
        RTTR_Assert(location);
        RTTR_Assert(location->GetPlayer() < MAX_PLAYERS);

        // Currently carried out of a warehouse?
        if(nobBaseWarehouse::isStorehouseGOT(location->GetGOT()))
        {
            if(location != goal)
            {
                SetGoal(static_cast<noBaseBuilding*>(location));
            } else // at the goal (which was just destroyed) and get carried out right now? -> we are about to get
                   // destroyed...
            {
                goal = nullptr;
                next_dir = RoadPathDirection::None;
            }
        }
        // Wenn sie an einer Flagge liegt, muss der Weg neu berechnet werden und dem Träger Bescheid gesagt werden
        else if(state == State::WaitAtFlag)
        {
            goal = nullptr;
            const auto oldNextDir = next_dir;
            FindRouteToWarehouse();
            if(oldNextDir != next_dir)
            {
                RemoveWareJobForDir(oldNextDir);
                if(next_dir != RoadPathDirection::None)
                {
                    RTTR_Assert(goal); // Having a nextDir implies having a goal
                    CallCarrier();
                } else
                    RTTR_Assert(!goal); // Can only have a goal with a valid path
            }
        } else if(state == State::Carried)
        {
            if(goal != location)
            {
                // find a warehouse for us (if we are entering a warehouse already set this as new goal (should only
                // happen if its a harbor for shipping as the building wasn't our goal))
                if(nobBaseWarehouse::isStorehouseGOT(
                     location->GetGOT())) // currently carried into a warehouse? -> add ware (pathfinding
                                          // will not return this wh because of path lengths 0)
                {
                    if(location->GetGOT() != GO_Type::NobHarborbuilding)
                        LOG.write("WARNING: Ware::GoalDestroyed() -- ware is currently being carried into warehouse or "
                                  "hq that was not "
                                  "it's goal! ware id %i, type %s, player %i, wareloc %i,%i, goal loc %i,%i \n")
                          % GetObjId() % WARE_NAMES[type] % location->GetPlayer() % GetLocation()->GetX()
                          % GetLocation()->GetY() % goal->GetX() % goal->GetY();
                    SetGoal(static_cast<noBaseBuilding*>(location));
                } else
                {
                    goal = nullptr;
                    FindRouteToWarehouse();
                }
            } else
            {
                // too late to do anything our road will be removed and ware destroyed when the carrier starts walking
                // about
                goal = nullptr;
            }
        }
    }
}

void Ware::WaitAtFlag(noFlag* flag)
{
    RTTR_Assert(flag);
    state = State::WaitAtFlag;
    location = flag;
}

void Ware::WaitInWarehouse(nobBaseWarehouse* wh)
{
    RTTR_Assert(wh);
    state = State::WaitInWarehouse;
    location = wh;
}

void Ware::Carry(noRoadNode* nextGoal)
{
    RTTR_Assert(nextGoal);
    state = State::Carried;
    location = nextGoal;
}

/// Gibt dem Ziel der Ware bekannt, dass diese nicht mehr kommen kann
void Ware::NotifyGoalAboutLostWare()
{
    // Meinem Ziel Bescheid sagen, dass ich weg vom Fenster bin (falls ich ein Ziel habe!)
    if(goal)
    {
        goal->WareLost(*this);
        goal = nullptr;
        next_dir = RoadPathDirection::None;
    }
}

/// Wenn die Ware vernichtet werden muss
void Ware::WareLost(const unsigned char player)
{
    location = nullptr;
    // Inventur verringern
    world->GetPlayer(player).DecreaseInventoryWare(type, 1);
    // Ziel der Ware Bescheid sagen
    NotifyGoalAboutLostWare();
    // Zentrale Registrierung der Ware löschen
    world->GetPlayer(player).RemoveWare(*this);
}

void Ware::RemoveWareJobForDir(const RoadPathDirection last_next_dir)
{
    // last_next_dir war die letzte Richtung, in die die Ware eigentlich wollte,
    // aber nun nicht mehr will, deshalb muss dem Träger Bescheid gesagt werden

    // War's überhaupt ne richtige Richtung?
    if(last_next_dir == RoadPathDirection::None || last_next_dir == RoadPathDirection::Ship)
        return;
    Direction lastDir = toDirection(last_next_dir);
    // Existiert da noch ne Straße?
    if(!location->GetRoute(lastDir))
        return;
    // Den Trägern Bescheid sagen
    location->GetRoute(lastDir)->WareJobRemoved(nullptr);
    // Wenn nicht, könntes ja sein, dass die Straße in ein Lagerhaus führt, dann muss dort Bescheid gesagt werden
    if(location->GetRoute(lastDir)->GetF2()->GetType() == NodalObjectType::Building)
    {
        auto* bld = static_cast<noBuilding*>(location->GetRoute(Direction::NorthWest)->GetF2());
        if(BuildingProperties::IsWareHouse(bld->GetBuildingType()))
            static_cast<nobBaseWarehouse*>(bld)->DontFetchNextWare();
    }
}

void Ware::CallCarrier()
{
    RTTR_Assert(IsWaitingAtFlag());
    RTTR_Assert(next_dir != RoadPathDirection::None && next_dir != RoadPathDirection::Ship);
    RTTR_Assert(location);
    location->GetRoute(toDirection(next_dir))->AddWareJob(location);
}

bool Ware::FindRouteToWarehouse()
{
    RTTR_Assert(location);
    RTTR_Assert(!goal); // Goal should have been notified and therefore reset
    SetGoal(world->GetPlayer(location->GetPlayer()).FindWarehouseForWare(*this));

    if(goal)
    {
        // Find path if not already carried (will be called after arrival in that case)
        if(state != State::Carried)
        {
            if(location == goal)
                next_dir = RoadPathDirection::None; // Warehouse will detect this
            else
            {
                next_dir = world->FindPathForWareOnRoads(*location, *goal, nullptr, &next_harbor);
                RTTR_Assert(next_dir != RoadPathDirection::None);
            }
        }
    } else
        next_dir = RoadPathDirection::None; // Make sure we are not going anywhere
    return goal != nullptr;
}

/// a lost ware got ordered
unsigned Ware::CheckNewGoalForLostWare(const noBaseBuilding& newgoal) const
{
    if(!IsWaitingAtFlag()) // todo: check all special cases for wares being carried right now and where possible allow
                           // them to be ordered
        return 0xFFFFFFFF;
    return CalcPathToGoal(newgoal).length;
}

Ware::RouteParams Ware::CalcPathToGoal(const noBaseBuilding& newgoal) const
{
    RTTR_Assert(location);
    unsigned length;
    RoadPathDirection possibledir = world->FindPathForWareOnRoads(*location, newgoal, &length);
    if(possibledir != RoadPathDirection::None) // there is a valid path to the goal? -> ordered!
    {
        // in case the ware is right in front of the goal building the ware has to be moved away 1 flag and then back
        // because non-warehouses cannot just carry in new wares they need a helper to do this
        if(possibledir == RoadPathDirection::NorthWest && newgoal.GetFlagPos() == location->GetPos())
        {
            // Not executed for road from flag to the warehouse as that is handled directly by the warehouse
            RTTR_Assert(!BuildingProperties::IsWareHouse(newgoal.GetBuildingType()));
            for(const auto dir : helpers::EnumRange<Direction>{})
            {
                // Bounce of in this direction
                if(dir != Direction::NorthWest && location->GetRoute(dir))
                    return {1, toRoadPathDirection(dir)};
            }
            // got no other route from the flag -> impossible
            return {0xFFFFFFFF, RoadPathDirection::None};
        }
        return {length, possibledir};
    }
    return {0xFFFFFFFF, RoadPathDirection::None};
}

/// this assumes that the ware is at a flag (todo: handle carried wares) and that there is a valid path to the goal
void Ware::SetNewGoalForLostWare(noBaseBuilding& newgoal)
{
    const auto newDir = CalcPathToGoal(newgoal).dir;
    if(newDir != RoadPathDirection::None) // there is a valid path to the goal? -> ordered!
    {
        next_dir = newDir;
        SetGoal(&newgoal);
        CallCarrier();
    }
}

bool Ware::IsRouteToGoal()
{
    RTTR_Assert(location);
    if(!goal)
        return false;
    if(location == goal)
        return true; // We are at our goal. All ok
    return world->FindPathForWareOnRoads(*location, *goal) != RoadPathDirection::None;
}

/// Informiert Ware, dass eine Schiffsreise beginnt
void Ware::StartShipJourney()
{
    state = State::OnShip;
    location = nullptr;
}

/// Informiert Ware, dass Schiffsreise beendet ist und die Ware nun in einem Hafengebäude liegt
void Ware::ShipJorneyEnded(nobHarborBuilding* hb)
{
    RTTR_Assert(hb);
    state = State::WaitInWarehouse;
    location = hb;
}

/// Beginnt damit auf ein Schiff im Hafen zu warten
void Ware::WaitForShip(nobHarborBuilding* hb)
{
    RTTR_Assert(hb);
    state = State::WaitForShip;
    location = hb;
}

std::string Ware::ToString() const
{
    std::stringstream s;
    s << "Ware(" << GetObjId() << "): type=" << GoodType2String(type) << ", location=" << location->GetX() << ","
      << location->GetY();
    return s.str();
}
