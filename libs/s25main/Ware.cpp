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
#include "gameData/GoodConsts.h"
#include "gameData/ShieldConsts.h"
#include "s25util/Log.h"
#include <sstream>

Ware::Ware(const GoodType type, noBaseBuilding* goal, noRoadNode* location)
    : next_dir(RoadPathDirection::None), state(State::WaitInWarehouse), location(location),
      type(convertShieldToNation(type,
                                 world->getPlayer(location->getPlayer()).nation)), // Use nation specific shield
      goal(goal), next_harbor(MapPoint::invalid())
{
    RTTR_Assert(location);
    // Ware in den Index mit eintragen
    world->getPlayer(location->getPlayer()).registerWare(*this);
    if(goal)
        goal->takeWare(this);
}

Ware::~Ware() = default;

void Ware::destroy()
{
    RTTR_Assert(!goal);
    RTTR_Assert(!location);
#if RTTR_ENABLE_ASSERTS
    for(unsigned p = 0; p < world->getNumPlayers(); p++)
    {
        RTTR_Assert(!world->getPlayer(p).isWareRegistred(*this));
        RTTR_Assert(!world->getPlayer(p).isWareDependent(*this));
    }
#endif
}

void Ware::serialize(SerializedGameData& sgd) const
{
    sgd.pushEnum<uint8_t>(next_dir);
    sgd.pushEnum<uint8_t>(state);
    sgd.pushObject(location);
    sgd.pushEnum<uint8_t>(type);
    sgd.pushObject(goal);
    helpers::pushPoint(sgd, next_harbor);
}

static RoadPathDirection popRoadPathDirection(SerializedGameData& sgd)
{
    if(sgd.getGameDataVersion() < 5)
    {
        const auto iDir = sgd.popUnsignedChar();
        if(iDir == 100)
            return RoadPathDirection::Ship;
        if(iDir == 0xFF)
            return RoadPathDirection::None;
        if(iDir > helpers::MaxEnumValue_v<Direction>)
            throw SerializedGameData::Error("Invalid RoadPathDirection");
        return RoadPathDirection(iDir);
    } else
        return sgd.pop<RoadPathDirection>();
}

Ware::Ware(SerializedGameData& sgd, const unsigned obj_id)
    : GameObject(sgd, obj_id), next_dir(popRoadPathDirection(sgd)), state(sgd.pop<State>()),
      location(sgd.popObject<noRoadNode>()), type([&]() {
          // GoodType::Nothing is moved because of adding new wares due addons
          auto ware = sgd.pop<GoodType>();
          if(sgd.getGameDataVersion() < 11)
          {
              // The old GoodType::Nothing is now GoodType::Grapes
              if(ware == GoodType::Grapes)
                  ware = GoodType::Nothing;
          } else if(sgd.getGameDataVersion() < 12)
          {
              // The old GoodType::Nothing is now GoodType::Skins
              if(ware == GoodType::Skins)
                  ware = GoodType::Nothing;
          }
          return ware;
      }()),
      goal(sgd.popObject<noBaseBuilding>()), next_harbor(sgd.popMapPoint())
{}

void Ware::setGoal(noBaseBuilding* newGoal)
{
    goal = newGoal;
    if(goal)
        goal->takeWare(this);
}

void Ware::recalcRoute()
{
    // Nächste Richtung nehmen
    if(location && goal)
        next_dir = world->findPathForWareOnRoads(*location, *goal, nullptr, &next_harbor);
    else
        next_dir = RoadPathDirection::None;

    // Evtl gibts keinen Weg mehr? Dann wieder zurück ins Lagerhaus (wenns vorher überhaupt zu nem Ziel ging)
    if(next_dir == RoadPathDirection::None && goal)
    {
        RTTR_Assert(location);
        // Tell goal about this
        notifyGoalAboutLostWare();
        if(state == State::WaitForShip)
        {
            // Ware was waiting for a ship so send the ware into the harbor
            RTTR_Assert(location->getGOT() == GO_Type::NobHarborbuilding);
            state = State::WaitInWarehouse;
            setGoal(static_cast<nobHarborBuilding*>(location));
            // but not going by ship
            static_cast<nobHarborBuilding*>(goal)->wareDontWantToTravelByShip(this);
        } else
        {
            // TODO(Replay) This should calculate the next dir even when carried
            findRouteToWarehouse();
        }
    } else
    {
        // If we waited in the harbor for the ship before and don't want to travel now
        // -> inform the harbor so that it can remove us from its list
        if(state == State::WaitForShip && next_dir != RoadPathDirection::Ship)
        {
            RTTR_Assert(location);
            RTTR_Assert(location->getGOT() == GO_Type::NobHarborbuilding);
            state = State::WaitInWarehouse;
            static_cast<nobHarborBuilding*>(location)->wareDontWantToTravelByShip(this);
        }
    }
}

void Ware::goalDestroyed()
{
    if(state == State::WaitInWarehouse)
    {
        // Ware ist noch im Lagerhaus auf der Warteliste
        RTTR_Assert(false); // Should not happen. noBaseBuilding::wareNotNeeded handles this case!
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
        RTTR_Assert(location->getGOT() == GO_Type::NobHarborbuilding);
        // This also adds the ware to the harbors inventory
        auto ownedWare = static_cast<nobHarborBuilding*>(location)->cancelWareForShip(this);
        // Kill the ware
        world->getPlayer(location->getPlayer()).removeWare(*this);
        goal = nullptr;
        location = nullptr;
        getEvMgr().addToKillList(std::move(ownedWare));
    } else
    {
        // Ware ist unterwegs, Lagerhaus finden und Ware dort einliefern
        RTTR_Assert(location);
        RTTR_Assert(location->getPlayer() < MAX_PLAYERS);

        // Currently carried out of a warehouse?
        if(nobBaseWarehouse::isStorehouseGOT(location->getGOT()))
        {
            if(location != goal)
            {
                setGoal(static_cast<noBaseBuilding*>(location));
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
            findRouteToWarehouse();
            if(oldNextDir != next_dir)
            {
                removeWareJobForDir(oldNextDir);
                if(next_dir != RoadPathDirection::None)
                {
                    RTTR_Assert(goal); // Having a nextDir implies having a goal
                    callCarrier();
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
                     location->getGOT())) // currently carried into a warehouse? -> add ware (pathfinding
                                          // will not return this wh because of path lengths 0)
                {
                    if(location->getGOT() != GO_Type::NobHarborbuilding)
                        LOG.write("WARNING: Ware::GoalDestroyed() -- ware is currently being carried into warehouse or "
                                  "hq that was not "
                                  "it's goal! ware id %i, type %s, player %i, wareloc %i,%i, goal loc %i,%i \n")
                          % getObjId() % WARE_NAMES[type] % location->getPlayer() % getLocation()->getX()
                          % getLocation()->getY() % goal->getX() % goal->getY();
                    setGoal(static_cast<noBaseBuilding*>(location));
                } else
                {
                    goal = nullptr;
                    findRouteToWarehouse();
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

void Ware::waitAtFlag(noFlag& flag)
{
    state = State::WaitAtFlag;
    location = &flag;
}

void Ware::waitInWarehouse(nobBaseWarehouse& wh)
{
    state = State::WaitInWarehouse;
    location = &wh;
}

void Ware::carry(noRoadNode& nextGoal)
{
    state = State::Carried;
    location = &nextGoal;
}

/// Gibt dem Ziel der Ware bekannt, dass diese nicht mehr kommen kann
void Ware::notifyGoalAboutLostWare()
{
    // Meinem Ziel Bescheid sagen, dass ich weg vom Fenster bin (falls ich ein Ziel habe!)
    if(goal)
    {
        goal->wareLost(*this);
        goal = nullptr;
        next_dir = RoadPathDirection::None;
    }
}

/// Wenn die Ware vernichtet werden muss
void Ware::wareLost(const unsigned char player)
{
    location = nullptr;
    // Inventur verringern
    world->getPlayer(player).decreaseInventoryWare(type, 1);
    // Ziel der Ware Bescheid sagen
    notifyGoalAboutLostWare();
    // Zentrale Registrierung der Ware löschen
    world->getPlayer(player).removeWare(*this);
}

void Ware::removeWareJobForDir(const RoadPathDirection lastNextDir)
{
    // lastNextDir war die letzte Richtung, in die die Ware eigentlich wollte,
    // aber nun nicht mehr will, deshalb muss dem Träger Bescheid gesagt werden

    // War's überhaupt ne richtige Richtung?
    if(lastNextDir == RoadPathDirection::None || lastNextDir == RoadPathDirection::Ship)
        return;
    Direction lastDir = toDirection(lastNextDir);
    // Existiert da noch ne Straße?
    if(!location->getRoute(lastDir))
        return;
    // Den Trägern Bescheid sagen
    location->getRoute(lastDir)->wareJobRemoved(nullptr);
    // Wenn nicht, könntes ja sein, dass die Straße in ein Lagerhaus führt, dann muss dort Bescheid gesagt werden
    if(location->getRoute(lastDir)->getF2()->getType() == NodalObjectType::Building)
    {
        auto* bld = static_cast<noBuilding*>(location->getRoute(Direction::NorthWest)->getF2());
        if(BuildingProperties::isWareHouse(bld->getBuildingType()))
            static_cast<nobBaseWarehouse*>(bld)->dontFetchNextWare();
    }
}

void Ware::callCarrier()
{
    RTTR_Assert(isWaitingAtFlag());
    RTTR_Assert(next_dir != RoadPathDirection::None && next_dir != RoadPathDirection::Ship);
    RTTR_Assert(location);
    location->getRoute(toDirection(next_dir))->addWareJob(location);
}

bool Ware::findRouteToWarehouse()
{
    RTTR_Assert(location);
    RTTR_Assert(!goal); // Goal should have been notified and therefore reset
    setGoal(world->getPlayer(location->getPlayer()).findWarehouseForWare(*this));

    if(goal)
    {
        // Find path if not already carried (will be called after arrival in that case)
        if(state != State::Carried)
        {
            if(location == goal)
                next_dir = RoadPathDirection::None; // Warehouse will detect this
            else
            {
                next_dir = world->findPathForWareOnRoads(*location, *goal, nullptr, &next_harbor);
                RTTR_Assert(next_dir != RoadPathDirection::None);
            }
        }
    } else
        next_dir = RoadPathDirection::None; // Make sure we are not going anywhere
    return goal != nullptr;
}

/// a lost ware got ordered
unsigned Ware::checkNewGoalForLostWare(const noBaseBuilding& newgoal) const
{
    if(!isWaitingAtFlag()) // todo: check all special cases for wares being carried right now and where possible allow
                           // them to be ordered
        return 0xFFFFFFFF;
    return calcPathToGoal(newgoal).length;
}

Ware::RouteParams Ware::calcPathToGoal(const noBaseBuilding& newgoal) const
{
    unsigned length;
    RoadPathDirection possibledir = world->findPathForWareOnRoads(assertNonNull(location), newgoal, &length);
    if(possibledir != RoadPathDirection::None) // there is a valid path to the goal? -> ordered!
    {
        // in case the ware is right in front of the goal building the ware has to be moved away 1 flag and then back
        // because non-warehouses cannot just carry in new wares they need a helper to do this
        if(possibledir == RoadPathDirection::NorthWest && newgoal.getFlagPos() == location->getPos())
        {
            // Not executed for road from flag to the warehouse as that is handled directly by the warehouse
            RTTR_Assert(!BuildingProperties::isWareHouse(newgoal.getBuildingType()));
            for(const auto dir : helpers::EnumRange<Direction>{})
            {
                // Bounce of in this direction
                if(dir != Direction::NorthWest && location->getRoute(dir))
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
void Ware::setNewGoalForLostWare(noBaseBuilding& newgoal)
{
    const auto newDir = calcPathToGoal(newgoal).dir;
    if(newDir != RoadPathDirection::None) // there is a valid path to the goal? -> ordered!
    {
        next_dir = newDir;
        setGoal(&newgoal);
        callCarrier();
    }
}

bool Ware::isRouteToGoal()
{
    RTTR_Assert(location);
    if(!goal)
        return false;
    if(location == goal)
        return true; // We are at our goal. All ok
    return world->findPathForWareOnRoads(*location, *goal) != RoadPathDirection::None;
}

/// Informiert Ware, dass eine Schiffsreise beginnt
void Ware::startShipJourney()
{
    state = State::OnShip;
    location = nullptr;
}

/// Informiert Ware, dass Schiffsreise beendet ist und die Ware nun in einem Hafengebäude liegt
void Ware::shipJorneyEnded(nobHarborBuilding* hb)
{
    RTTR_Assert(hb);
    state = State::WaitInWarehouse;
    location = hb;
}

/// Beginnt damit auf ein Schiff im Hafen zu warten
void Ware::waitForShip(nobHarborBuilding* hb)
{
    RTTR_Assert(hb);
    state = State::WaitForShip;
    location = hb;
}

std::string Ware::toString() const
{
    std::stringstream s;
    s << "Ware(" << getObjId() << "): type=" << goodType2String(type) << ", location=" << location->getX() << ","
      << location->getY();
    return s.str();
}
