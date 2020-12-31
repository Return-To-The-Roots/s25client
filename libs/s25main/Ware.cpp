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

#include "Ware.h"
#include "EventManager.h"
#include "GamePlayer.h"
#include "RoadSegment.h"
#include "SerializedGameData.h"
#include "buildings/noBaseBuilding.h"
#include "buildings/noBuilding.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobHarborBuilding.h"
#include "world/GameWorldGame.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noRoadNode.h"
#include "gameData/BuildingProperties.h"
#include "gameData/GameConsts.h"
#include "gameData/GoodConsts.h"
#include "gameData/ShieldConsts.h"
#include "s25util/Log.h"
#include <sstream>

Ware::Ware(const GoodType type, noBaseBuilding* goal, noRoadNode* location)
    : next_dir(RoadPathDirection::None), state(STATE_WAITINWAREHOUSE), location(location),
      type(convertShieldToNation(type,
                                 gwg->GetPlayer(location->GetPlayer()).nation)), // Use nation specific shield
      goal(goal), next_harbor(MapPoint::Invalid())
{
    RTTR_Assert(location);
    // Ware in den Index mit eintragen
    gwg->GetPlayer(location->GetPlayer()).RegisterWare(this);
    if(goal)
        goal->TakeWare(this);
}

Ware::~Ware() = default;

void Ware::Destroy()
{
    RTTR_Assert(!goal);
    RTTR_Assert(!location);
#if RTTR_ENABLE_ASSERTS
    for(unsigned p = 0; p < gwg->GetNumPlayers(); p++)
    {
        RTTR_Assert(!gwg->GetPlayer(p).IsWareRegistred(this));
        RTTR_Assert(!gwg->GetPlayer(p).IsWareDependent(this));
    }
#endif
}

void Ware::Serialize_Ware(SerializedGameData& sgd) const
{
    Serialize_GameObject(sgd);

    sgd.PushEnum<uint8_t>(next_dir);
    sgd.PushUnsignedChar(state);
    sgd.PushObject(location, false);
    sgd.PushEnum<uint8_t>(type);
    sgd.PushObject(goal, false);
    sgd.PushMapPoint(next_harbor);
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
    : GameObject(sgd, obj_id), next_dir(PopRoadPathDirection(sgd)), state(State(sgd.PopUnsignedChar())),
      location(sgd.PopObject<noRoadNode>(GOT_UNKNOWN)), type(sgd.Pop<GoodType>()),
      goal(sgd.PopObject<noBaseBuilding>(GOT_UNKNOWN)), next_harbor(sgd.PopMapPoint())
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
        next_dir = gwg->FindPathForWareOnRoads(*location, *goal, nullptr, &next_harbor);
    else
        next_dir = RoadPathDirection::None;

    // Evtl gibts keinen Weg mehr? Dann wieder zurück ins Lagerhaus (wenns vorher überhaupt zu nem Ziel ging)
    if(next_dir == RoadPathDirection::None && goal)
    {
        RTTR_Assert(location);
        // Tell goal about this
        NotifyGoalAboutLostWare();
        if(state == STATE_WAITFORSHIP)
        {
            // Ware was waiting for a ship so send the ware into the harbor
            RTTR_Assert(location->GetGOT() == GOT_NOB_HARBORBUILDING);
            state = STATE_WAITINWAREHOUSE;
            SetGoal(static_cast<nobHarborBuilding*>(location));
            // but not going by ship
            static_cast<nobHarborBuilding*>(goal)->WareDontWantToTravelByShip(this);
        } else
        {
            FindRouteToWarehouse();
        }
    } else
    {
        // If we waited in the harbor for the ship before and don't want to travel now
        // -> inform the harbor so that it can remove us from its list
        if(state == STATE_WAITFORSHIP && next_dir != RoadPathDirection::Ship)
        {
            RTTR_Assert(location);
            RTTR_Assert(location->GetGOT() == GOT_NOB_HARBORBUILDING);
            state = STATE_WAITINWAREHOUSE;
            static_cast<nobHarborBuilding*>(location)->WareDontWantToTravelByShip(this);
        }
    }
}

void Ware::GoalDestroyed()
{
    if(state == STATE_WAITINWAREHOUSE)
    {
        // Ware ist noch im Lagerhaus auf der Warteliste
        RTTR_Assert(false); // Should not happen. noBaseBuilding::WareNotNeeded handles this case!
        goal = nullptr; // just in case: avoid corruption although the ware itself might be lost (won't ever be carried
                        // again)
    }
    // Ist sie evtl. gerade mit dem Schiff unterwegs?
    else if(state == STATE_ONSHIP)
    {
        // Ziel zunächst auf nullptr setzen, was dann vom Zielhafen erkannt wird,
        // woraufhin dieser die Ware gleich in sein Inventar mit übernimmt
        goal = nullptr;
    }
    // Oder wartet sie im Hafen noch auf ein Schiff
    else if(state == STATE_WAITFORSHIP)
    {
        // Dann dem Hafen Bescheid sagen
        RTTR_Assert(location);
        RTTR_Assert(location->GetGOT() == GOT_NOB_HARBORBUILDING);
        // This also adds the ware to the harbors inventory
        static_cast<nobHarborBuilding*>(location)->CancelWareForShip(this);
        // Kill the ware
        gwg->GetPlayer(location->GetPlayer()).RemoveWare(this);
        goal = nullptr;
        location = nullptr;
        GetEvMgr().AddToKillList(this);
    } else
    {
        // Ware ist unterwegs, Lagerhaus finden und Ware dort einliefern
        RTTR_Assert(location);
        RTTR_Assert(location->GetPlayer() < MAX_PLAYERS);

        // Wird sie gerade aus einem Lagerhaus rausgetragen?
        if(location->GetGOT() == GOT_NOB_STOREHOUSE || location->GetGOT() == GOT_NOB_HARBORBUILDING
           || location->GetGOT() == GOT_NOB_HQ)
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
        else if(state == STATE_WAITATFLAG)
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
        } else if(state == STATE_CARRIED)
        {
            if(goal != location)
            {
                // find a warehouse for us (if we are entering a warehouse already set this as new goal (should only
                // happen if its a harbor for shipping as the building wasnt our goal))
                if(location->GetGOT() == GOT_NOB_STOREHOUSE || location->GetGOT() == GOT_NOB_HARBORBUILDING
                   || location->GetGOT() == GOT_NOB_HQ) // currently carried into a warehouse? -> add ware (pathfinding
                                                        // will not return this wh because of path lengths 0)
                {
                    if(location->GetGOT() != GOT_NOB_HARBORBUILDING)
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
    state = STATE_WAITATFLAG;
    location = flag;
}

void Ware::WaitInWarehouse(nobBaseWarehouse* wh)
{
    RTTR_Assert(wh);
    state = STATE_WAITINWAREHOUSE;
    location = wh;
}

void Ware::Carry(noRoadNode* nextGoal)
{
    RTTR_Assert(nextGoal);
    state = STATE_CARRIED;
    location = nextGoal;
}

/// Gibt dem Ziel der Ware bekannt, dass diese nicht mehr kommen kann
void Ware::NotifyGoalAboutLostWare()
{
    // Meinem Ziel Bescheid sagen, dass ich weg vom Fenster bin (falls ich ein Ziel habe!)
    if(goal)
    {
        goal->WareLost(this);
        goal = nullptr;
        next_dir = RoadPathDirection::None;
    }
}

/// Wenn die Ware vernichtet werden muss
void Ware::WareLost(const unsigned char player)
{
    location = nullptr;
    // Inventur verringern
    gwg->GetPlayer(player).DecreaseInventoryWare(type, 1);
    // Ziel der Ware Bescheid sagen
    NotifyGoalAboutLostWare();
    // Zentrale Registrierung der Ware löschen
    gwg->GetPlayer(player).RemoveWare(this);
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
    if(location->GetRoute(lastDir)->GetF2()->GetType() == NOP_BUILDING)
    {
        noBuilding* bld = static_cast<noBuilding*>(location->GetRoute(Direction::NORTHWEST)->GetF2());
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
    SetGoal(gwg->GetPlayer(location->GetPlayer()).FindWarehouseForWare(*this));

    if(goal)
    {
        // Find path if not already carried (will be called after arrival in that case)
        if(state != STATE_CARRIED)
        {
            if(location == goal)
                next_dir = RoadPathDirection::None; // Warehouse will detect this
            else
            {
                next_dir = gwg->FindPathForWareOnRoads(*location, *goal, nullptr, &next_harbor);
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
    unsigned length = 0xFFFFFFFF;
    RoadPathDirection possibledir = gwg->FindPathForWareOnRoads(*location, newgoal, &length);
    if(possibledir != RoadPathDirection::None) // there is a valid path to the goal? -> ordered!
    {
        // in case the ware is right in front of the goal building the ware has to be moved away 1 flag and then back
        // because non-warehouses cannot just carry in new wares they need a helper to do this
        if(possibledir == RoadPathDirection::NorthWest && newgoal.GetFlag()->GetPos() == location->GetPos())
        {
            for(const auto dir : helpers::EnumRange<Direction>{})
            {
                if(dir != Direction::NORTHWEST && location->GetRoute(dir))
                {
                    possibledir = toRoadPathDirection(dir);
                    break;
                }
            }
            if(possibledir == RoadPathDirection::NorthWest) // got no other route from the flag -> impossible
                return {0xFFFFFFFF, RoadPathDirection::None};
        }
        // at this point there either is a road to the goal
        // or we are at the flag of the goal and have a road to a different flag to bounce off of to get to the goal
        return {length, possibledir};
    }
    return {0xFFFFFFFF, RoadPathDirection::None};
}

/// this assumes that the ware is at a flag (todo: handle carried wares) and that there is a valid path to the goal
void Ware::SetNewGoalForLostWare(noBaseBuilding* newgoal)
{
    RTTR_Assert(newgoal);
    const auto newDir = CalcPathToGoal(*newgoal).dir;
    if(newDir != RoadPathDirection::None) // there is a valid path to the goal? -> ordered!
    {
        next_dir = newDir;
        SetGoal(newgoal);
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
    return gwg->FindPathForWareOnRoads(*location, *goal) != RoadPathDirection::None;
}

/// Informiert Ware, dass eine Schiffsreise beginnt
void Ware::StartShipJourney()
{
    state = STATE_ONSHIP;
    location = nullptr;
}

/// Informiert Ware, dass Schiffsreise beendet ist und die Ware nun in einem Hafengebäude liegt
void Ware::ShipJorneyEnded(nobHarborBuilding* hb)
{
    RTTR_Assert(hb);
    state = STATE_WAITINWAREHOUSE;
    location = hb;
}

/// Beginnt damit auf ein Schiff im Hafen zu warten
void Ware::WaitForShip(nobHarborBuilding* hb)
{
    RTTR_Assert(hb);
    state = STATE_WAITFORSHIP;
    location = hb;
}

std::string Ware::ToString() const
{
    std::stringstream s;
    s << "Ware(" << GetObjId() << "): type=" << GoodType2String(type) << ", location=" << location->GetX() << ","
      << location->GetY();
    return s.str();
}
