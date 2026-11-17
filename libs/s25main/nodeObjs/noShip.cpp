// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "noShip.h"
#include "EventManager.h"
#include "GameEvent.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "Ware.h"
#include "addons/const_addons.h"
#include "buildings/nobHarborBuilding.h"
#include "figures/noFigure.h"
#include "figures/nofAttacker.h"
#include "helpers/EnumArray.h"
#include "helpers/containerUtils.h"
#include "helpers/pointerContainerUtils.h"
#include "network/GameClient.h"
#include "notifications/ExpeditionNote.h"
#include "notifications/ShipNote.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "pathfinding/ShipPathData.h"
#include "postSystem/ShipPostMsg.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include "gameData/BuildingConsts.h"
#include "gameData/ShipNames.h"
#include "s25util/Log.h"
#include <array>

/// Zeit zum Beladen des Schiffes
const unsigned LOADING_TIME = 200;
/// Zeit zum Entladen des Schiffes
const unsigned UNLOADING_TIME = 200;

/// Maximaler Weg, der zurückgelegt werden kann bei einem Erkundungsschiff
const unsigned MAX_EXPLORATION_EXPEDITION_DISTANCE = 100;
/// Zeit (in GF), die das Schiff bei der Erkundungs-Expedition jeweils an einem Punkt ankert
const unsigned EXPLORATION_EXPEDITION_WAITING_TIME = 300;

/// Positionen der Flaggen am Schiff für die 6 unterschiedlichen Richtungen jeweils
constexpr std::array<helpers::EnumArray<DrawPoint, Direction>, 2> SHIPS_FLAG_POS = {{
  {{{-3, -77}, {-6, -71}, {-3, -71}, {-1, -71}, {5, -63}, {-1, -70}}}, // Standing (sails down)
  {{{3, -70}, {0, -64}, {3, -64}, {-1, -70}, {5, -63}, {5, -63}}}      // Driving
}};

noShip::noShip(const MapPoint pos, const unsigned char player)
    : noMovable(NodalObjectType::Ship, pos), ownerId_(player), state(State::Idle), goal_dir(0),
      name(RANDOM_ELEMENT(ship_names[world->getPlayer(player).nation])), curRouteIdx(0), lost(false),
      remaining_sea_attackers(0), covered_distance(0)
{
    // Meer ermitteln, auf dem dieses Schiff fährt
    for(const auto dir : helpers::EnumRange<Direction>{})
    {
        SeaId seaId = world->getNeighbourNode(pos, dir).seaId;
        if(seaId)
            this->seaId_ = seaId;
    }

    // Auf irgendeinem Meer müssen wir ja sein
    RTTR_Assert(seaId_);
}

noShip::~noShip() = default;

void noShip::serialize(SerializedGameData& sgd) const
{
    noMovable::serialize(sgd);

    sgd.pushUnsignedChar(ownerId_);
    sgd.pushEnum<uint8_t>(state);
    sgd.pushUnsignedShort(seaId_.value());
    sgd.pushUnsignedInt(goalHarbor.value());
    sgd.pushUnsignedChar(goal_dir);
    sgd.pushString(name);
    sgd.pushUnsignedInt(curRouteIdx);
    sgd.pushBool(lost);
    sgd.pushUnsignedInt(remaining_sea_attackers);
    sgd.pushUnsignedInt(homeHarbor.value());
    sgd.pushUnsignedInt(covered_distance);
    helpers::pushContainer(sgd, route_);
    sgd.pushObjectContainer(figures);
    sgd.pushObjectContainer(wares, true);
}

noShip::noShip(SerializedGameData& sgd, const unsigned obj_id)
    : noMovable(sgd, obj_id), ownerId_(sgd.popUnsignedChar()), state(sgd.pop<State>()), seaId_(sgd.popUnsignedShort()),
      goalHarbor(sgd.popUnsignedInt()), goal_dir(sgd.popUnsignedChar()),
      name(sgd.getGameDataVersion() < 2 ? sgd.popLongString() : sgd.popString()), curRouteIdx(sgd.popUnsignedInt()),
      route_(sgd.getGameDataVersion() < 7 ? sgd.popUnsignedInt() : 0), lost(sgd.popBool()),
      remaining_sea_attackers(sgd.popUnsignedInt()), homeHarbor(sgd.popUnsignedInt()),
      covered_distance(sgd.popUnsignedInt())
{
    helpers::popContainer(sgd, route_, sgd.getGameDataVersion() < 7);
    sgd.popObjectContainer(figures);
    sgd.popObjectContainer(wares, GO_Type::Ware);
}

void noShip::destroy()
{
    RTTR_Assert(figures.empty());
    RTTR_Assert(wares.empty());
    world->getNotifications().publish(ShipNote(ShipNote::Destroyed, ownerId_, pos));
    // Schiff wieder abmelden
    world->getPlayer(ownerId_).removeShip(this);
}

void noShip::draw(DrawPoint drawPt)
{
    unsigned flagDrawingType = 1;

    // Sind wir verloren? Dann immer stehend zeichnen
    if(lost)
    {
        drawFixed(drawPt, true);
        return;
    }

    switch(state)
    {
        default: break;
        case State::Idle:
        case State::SeaattackWaiting:
        {
            drawFixed(drawPt, false);
            flagDrawingType = 0;
        }
        break;

        case State::Gotoharbor:
        {
            drawDriving(drawPt);
        }
        break;
        case State::ExpeditionLoading:
        case State::ExpeditionUnloading:
        case State::TransportLoading:
        case State::TransportUnloading:
        case State::SeaattackLoading:
        case State::SeaattackUnloading:
        case State::ExplorationexpeditionLoading:
        case State::ExplorationexpeditionUnloading:
        {
            drawFixed(drawPt, false);
        }
        break;
        case State::ExplorationexpeditionWaiting:
        case State::ExpeditionWaiting:
        {
            drawFixed(drawPt, true);
        }
        break;
        case State::ExpeditionDriving:
        case State::TransportDriving:
        case State::SeaattackDrivingToDestination:
        case State::ExplorationexpeditionDriving:
        {
            drawDrivingWithWares(drawPt);
        }
        break;
        case State::SeaattackReturnDriving:
        {
            if(!figures.empty() || !wares.empty())
                drawDrivingWithWares(drawPt);
            else
                drawDriving(drawPt);
        }
        break;
    }

    LOADER.getPlayerImage("boot_z", 40 + GAMECLIENT.getGlobalAnimation(6, 1, 1, getObjId()))
      ->drawFull(drawPt + SHIPS_FLAG_POS[flagDrawingType][getCurMoveDir()], COLOR_WHITE,
                 world->getPlayer(ownerId_).color);
    // Second, white flag, only when on expedition, always swinging in the opposite direction
    if(state >= State::ExpeditionLoading && state <= State::ExpeditionDriving)
        LOADER.getPlayerImage("boot_z", 40 + GAMECLIENT.getGlobalAnimation(6, 1, 1, getObjId() + 4))
          ->drawFull(drawPt + SHIPS_FLAG_POS[flagDrawingType][getCurMoveDir()]);
}

/// Zeichnet das Schiff stehend mit oder ohne Waren
void noShip::drawFixed(DrawPoint drawPt, const bool drawWares)
{
    LOADER.getImageN("boot_z", rttr::enumCast(getCurMoveDir() + 3u) * 2 + 1)->drawFull(drawPt, COLOR_SHADOW);
    LOADER.getImageN("boot_z", rttr::enumCast(getCurMoveDir() + 3u) * 2)->drawFull(drawPt);

    if(drawWares)
        /// Waren zeichnen
        LOADER.getImageN("boot_z", 30 + rttr::enumCast(getCurMoveDir() + 3u))->drawFull(drawPt);
}

/// Zeichnet normales Fahren auf dem Meer ohne irgendwelche Güter
void noShip::drawDriving(DrawPoint& drawPt)
{
    // Interpolieren zwischen beiden Knotenpunkten
    drawPt += calcWalkingRelative();

    LOADER.getImageN("boot_z", 13 + rttr::enumCast(getCurMoveDir() + 3u) * 2)->drawFull(drawPt, COLOR_SHADOW);
    LOADER.getImageN("boot_z", 12 + rttr::enumCast(getCurMoveDir() + 3u) * 2)->drawFull(drawPt);
}

/// Zeichnet normales Fahren auf dem Meer mit Gütern
void noShip::drawDrivingWithWares(DrawPoint& drawPt)
{
    drawDriving(drawPt);
    /// Waren zeichnen
    LOADER.getImageN("boot_z", 30 + rttr::enumCast(getCurMoveDir() + 3u))->drawFull(drawPt);
}

void noShip::handleEvent(const unsigned id)
{
    RTTR_Assert(current_ev);
    RTTR_Assert(current_ev->id == id);
    current_ev = nullptr;

    if(id == 0)
    {
        // Move event
        // neue Position einnehmen
        walk();
        // entscheiden, was als nächstes zu tun ist
        driven();
    } else
    {
        switch(state)
        {
            default:
                RTTR_Assert(false);
                LOG.write("Bug detected: Invalid state in ship event");
                break;
            case State::ExpeditionLoading:
                // Schiff ist nun bereit und Expedition kann beginnen
                state = State::ExpeditionWaiting;

                // Spieler benachrichtigen
                sendPostMessage(ownerId_, std::make_unique<ShipPostMsg>(getEvMgr().getCurrentGF(),
                                                                        _("A ship is ready for an expedition."),
                                                                        PostCategory::Economy, *this));
                world->getNotifications().publish(ExpeditionNote(ExpeditionNote::Waiting, ownerId_, pos));
                break;
            case State::ExplorationexpeditionLoading:
            case State::ExplorationexpeditionWaiting:
                // Schiff ist nun bereit und Expedition kann beginnen
                continueExplorationExpedition();
                break;
            case State::ExpeditionUnloading:
            {
                // Hafen herausfinden
                noBase* hb = goalHarbor ? world->getNO(world->getHarborPoint(goalHarbor)) : nullptr;

                if(hb && hb->getGOT() == GO_Type::NobHarborbuilding)
                {
                    GoodsAndPeopleCounts goods;
                    goods.goods[GoodType::Boards] = BUILDING_COSTS[BuildingType::HarborBuilding].boards;
                    goods.goods[GoodType::Stones] = BUILDING_COSTS[BuildingType::HarborBuilding].stones;
                    goods.people[Job::Builder] = 1;
                    static_cast<nobBaseWarehouse*>(hb)->addToInventory(goods, false);
                    // Wieder idlen und ggf. neuen Job suchen
                    startIdling();
                    world->getPlayer(ownerId_).getJobForShip(*this);
                } else
                {
                    // target harbor for unloading doesnt exist anymore -> set state to driving and handle the new state
                    state = State::ExpeditionDriving;
                    handleStateExpeditionDriving();
                }
                break;
            }
            case State::ExplorationexpeditionUnloading:
            {
                // Hafen herausfinden
                noBase* hb = goalHarbor ? world->getNO(world->getHarborPoint(goalHarbor)) : nullptr;

                unsigned oldVisualRange = getVisualRange();

                if(hb && hb->getGOT() == GO_Type::NobHarborbuilding)
                {
                    // Späher wieder entladen
                    const auto people = PeopleCounts::make(Job::Scout, world->getGGS().getNumScoutsExpedition());
                    static_cast<nobBaseWarehouse*>(hb)->addToInventory(people, false);
                    // Wieder idlen und ggf. neuen Job suchen
                    startIdling();
                    world->getPlayer(ownerId_).getJobForShip(*this);
                } else
                {
                    // target harbor for unloading doesnt exist anymore -> set state to driving and handle the new state
                    state = State::ExplorationexpeditionDriving;
                    handleStateExplorationExpeditionDriving();
                }

                // Sichtbarkeiten neu berechnen
                world->recalcVisibilitiesAroundPoint(pos, oldVisualRange, ownerId_, nullptr);

                break;
            }
            case State::TransportLoading: startTransport(); break;
            case State::TransportUnloading:
            case State::SeaattackUnloading:
            {
                // Hafen herausfinden
                RTTR_Assert(state == State::SeaattackUnloading || remaining_sea_attackers == 0);
                noBase* hb = goalHarbor ? world->getNO(world->getHarborPoint(goalHarbor)) : nullptr;
                if(hb && hb->getGOT() == GO_Type::NobHarborbuilding)
                {
                    static_cast<nobHarborBuilding*>(hb)->receiveGoodsFromShip(figures, wares);
                    figures.clear();
                    wares.clear();

                    state = State::TransportUnloading;
                    // Hafen bescheid sagen, dass er das Schiff nun nutzen kann
                    static_cast<nobHarborBuilding*>(hb)->shipArrived(*this);

                    // Hafen hat keinen Job für uns?
                    if(state == State::TransportUnloading)
                    {
                        // Wieder idlen und ggf. neuen Job suchen
                        startIdling();
                        world->getPlayer(ownerId_).getJobForShip(*this);
                    }
                } else
                {
                    // target harbor for unloading doesnt exist anymore -> set state to driving and handle the new state
                    if(state == State::TransportUnloading)
                        findUnloadGoal(State::TransportDriving);
                    else
                        findUnloadGoal(State::SeaattackReturnDriving);
                }
                break;
            }
            case State::SeaattackLoading: startSeaAttack(); break;
            case State::SeaattackWaiting:
            {
                // Nächsten Soldaten nach draußen beordern
                if(figures.empty())
                    break;

                // Evtl. ist ein Angreifer schon fertig und wieder an Board gegangen
                // der darf dann natürlich nicht noch einmal raus, sonst kann die schöne Reise
                // böse enden
                if(static_cast<nofAttacker&>(*figures.front()).isSeaAttackCompleted())
                    break;

                auto& attacker = world->addFigure(pos, std::move(figures.front()));
                figures.pop_front();

                current_ev = getEvMgr().addEvent(this, 30, 1);
                static_cast<nofAttacker&>(attacker).startAttackOnOtherIsland(pos, getObjId());
                break;
            }
        }
    }
}

void noShip::startDriving(const Direction dir)
{
    const std::array<unsigned, 5> SHIP_SPEEDS = {35, 25, 20, 10, 5};

    startMoving(dir, SHIP_SPEEDS[world->getGGS().getSelection(AddonId::SHIP_SPEED)]);
}

void noShip::driven()
{
    MapPoint enemy_territory_discovered(MapPoint::invalid());
    world->recalcMovingVisibilities(pos, ownerId_, getVisualRange(), getCurMoveDir(), &enemy_territory_discovered);

    // Feindliches Territorium entdeckt?
    if(enemy_territory_discovered.isValid())
    {
        // Send message if necessary
        if(world->getPlayer(ownerId_).shipDiscoveredHostileTerritory(enemy_territory_discovered))
            sendPostMessage(ownerId_, std::make_unique<PostMsg>(getEvMgr().getCurrentGF(),
                                                                _("A ship disovered an enemy territory"),
                                                                PostCategory::Military, enemy_territory_discovered));
    }

    switch(state)
    {
        case State::Gotoharbor: handleStateGoToHarbor(); break;
        case State::ExpeditionDriving: handleStateExpeditionDriving(); break;
        case State::ExplorationexpeditionDriving: handleStateExplorationExpeditionDriving(); break;
        case State::TransportDriving: handleStateTransportDriving(); break;
        case State::SeaattackDrivingToDestination: handleStateSeaAttackDriving(); break;
        case State::SeaattackReturnDriving: handleStateSeaAttackReturn(); break;
        default: RTTR_Assert(false); break;
    }
}

bool noShip::isLoading() const
{
    return state == State::ExpeditionLoading || state == State::ExplorationexpeditionLoading
           || state == State::TransportLoading || state == State::SeaattackLoading;
}

bool noShip::isUnloading() const
{
    return state == State::ExpeditionUnloading || state == State::ExplorationexpeditionUnloading
           || state == State::TransportUnloading || state == State::SeaattackUnloading;
}

bool noShip::isOnBoard(const noFigure& figure) const
{
    return helpers::containsPtr(figures, &figure);
}

/// Gibt Sichtradius dieses Schiffes zurück
unsigned noShip::getVisualRange() const
{
    // Erkundungsschiffe haben einen größeren Sichtbereich
    if(state >= State::ExplorationexpeditionLoading && state <= State::ExplorationexpeditionDriving)
        return VISUALRANGE_EXPLORATION_SHIP;
    else
        return VISUALRANGE_SHIP;
}

/// Fährt zum Hafen, um dort eine Mission (Expedition) zu erledigen
void noShip::goToHarbor(const nobHarborBuilding& hb, const std::vector<Direction>& route)
{
    RTTR_Assert(state == State::Idle); // otherwise we might carry wares etc
    RTTR_Assert(figures.empty());
    RTTR_Assert(wares.empty());
    RTTR_Assert(remaining_sea_attackers == 0);

    state = State::Gotoharbor;

    goalHarbor = world->getNode(hb.getPos()).harborId;
    RTTR_Assert(goalHarbor);

    // Route merken
    this->route_ = route;
    curRouteIdx = 1;

    // losfahren
    startDriving(route[0]);
}

/// Startet eine Expedition
void noShip::startExpedition(HarborId homeHarborId)
{
    /// Schiff wird "beladen", also kurze Zeit am Hafen stehen, bevor wir bereit sind
    state = State::ExpeditionLoading;
    current_ev = getEvMgr().addEvent(this, LOADING_TIME, 1);
    RTTR_Assert(homeHarborId);
    RTTR_Assert(pos == world->getCoastalPoint(homeHarborId, seaId_));
    homeHarbor = homeHarborId;
    goalHarbor = homeHarborId; // This is current goal (commands are relative to current goal)
}

/// Startet eine Erkundungs-Expedition
void noShip::startExplorationExpedition(HarborId homeHarborId)
{
    /// Schiff wird "beladen", also kurze Zeit am Hafen stehen, bevor wir bereit sind
    state = State::ExplorationexpeditionLoading;
    current_ev = getEvMgr().addEvent(this, LOADING_TIME, 1);
    covered_distance = 0;
    RTTR_Assert(homeHarborId);
    RTTR_Assert(pos == world->getCoastalPoint(homeHarborId, seaId_));
    homeHarbor = homeHarborId;
    goalHarbor = homeHarborId; // This is current goal (commands are relative to current goal)
    // Sichtbarkeiten neu berechnen
    world->makeVisibleAroundPoint(pos, getVisualRange(), ownerId_);
}

/// Fährt weiter zu einem Hafen
noShip::Result noShip::driveToHarbour()
{
    if(!goalHarbor)
        return Result::HarborDoesntExist;
    const MapPoint goal = world->getHarborPoint(goalHarbor);

    // Existiert der Hafen überhaupt noch?
    if(world->getGOT(goal) != GO_Type::NobHarborbuilding)
        return Result::HarborDoesntExist;

    return driveToHarbourPlace();
}

/// Fährt weiter zu Hafenbauplatz
noShip::Result noShip::driveToHarbourPlace()
{
    if(!goalHarbor)
        return Result::HarborDoesntExist;

    // Sind wir schon da?
    if(curRouteIdx == route_.size())
        return Result::GoalReached;

    MapPoint goalRoutePos;

    // Route überprüfen
    if(!world->checkShipRoute(pos, route_, curRouteIdx, &goalRoutePos))
    {
        // Route kann nicht mehr passiert werden --> neue Route suchen
        if(!world->findShipPathToHarbor(pos, goalHarbor, seaId_, &route_, nullptr))
        {
            // Wieder keine gefunden -> raus
            return Result::NoRouteFound;
        }

        // Wir fangen bei der neuen Route wieder von vorne an
        curRouteIdx = 0;
    } else if(goalRoutePos != world->getCoastalPoint(goalHarbor, seaId_))
    {
        // Our goal point of the current route has changed
        // If we are close to it, recalculate the route
        RTTR_Assert(route_.size() >= curRouteIdx);
        if(route_.size() - curRouteIdx < 10)
        {
            if(!world->findShipPathToHarbor(pos, goalHarbor, seaId_, &route_, nullptr))
                // Keiner gefunden -> raus
                return Result::NoRouteFound;

            curRouteIdx = 0;
        }
    }

    RTTR_Assert(curRouteIdx < route_.size());
    startDriving(route_[curRouteIdx++]);
    return Result::Driving;
}

HarborId noShip::getCurrentHarbor() const
{
    RTTR_Assert(state == State::ExpeditionWaiting);
    return goalHarbor;
}

HarborId noShip::getTargetHarbor() const
{
    return goalHarbor;
}

HarborId noShip::getHomeHarbor() const
{
    return homeHarbor;
}

/// Weist das Schiff an, in einer bestimmten Richtung die Expedition fortzusetzen
void noShip::continueExpedition(const ShipDirection dir)
{
    if(state != State::ExpeditionWaiting)
        return;

    // Nächsten Hafenpunkt in dieser Richtung suchen
    HarborId newGoal = world->getNextFreeHarborPoint(pos, goalHarbor, dir, ownerId_);

    // Auch ein Ziel gefunden?
    if(!newGoal)
        return;

    // Versuchen, Weg zu finden
    if(!world->findShipPathToHarbor(pos, newGoal, seaId_, &route_, nullptr))
        return;

    // Dann fahren wir da mal hin
    curRouteIdx = 0;
    goalHarbor = newGoal;
    state = State::ExpeditionDriving;

    handleStateExpeditionDriving();
}

/// Weist das Schiff an, eine Expedition abzubrechen (nur wenn es steht) und zum
/// Hafen zurückzukehren
void noShip::CancelExpedition()
{
    // Protect against double execution
    if(state != State::ExpeditionWaiting)
        return;

    // We are waiting. There should be no event!
    RTTR_Assert(!current_ev);

    // Zum Heimathafen zurückkehren
    // Oder sind wir schon dort?
    if(goalHarbor == homeHarbor)
    {
        route_.clear();
        curRouteIdx = 0;
        state = State::ExpeditionDriving; // just in case the home harbor was destroyed
        handleStateExpeditionDriving();
    } else
    {
        state = State::ExpeditionDriving;
        goalHarbor = homeHarbor;
        startDrivingToHarborPlace();
        handleStateExpeditionDriving();
    }
}

/// Weist das Schiff an, an der aktuellen Position einen Hafen zu gründen
void noShip::foundColony()
{
    if(state != State::ExpeditionWaiting)
        return;

    // Kolonie gründen
    if(world->foundColony(goalHarbor, ownerId_, seaId_))
    {
        // For checks
        state = State::ExpeditionUnloading;
        // Dann idlen wir wieder
        startIdling();
        // Neue Arbeit suchen
        world->getPlayer(ownerId_).getJobForShip(*this);
    } else // colony founding FAILED
        world->getNotifications().publish(ExpeditionNote(ExpeditionNote::Waiting, ownerId_, pos));
}

void noShip::handleStateGoToHarbor()
{
    // Hafen schon zerstört?
    if(!goalHarbor)
    {
        startIdling();
        return;
    }

    Result res = driveToHarbour();
    switch(res)
    {
        case Result::Driving: return; // Continue
        case Result::GoalReached:
        {
            const MapPoint goal = world->getHarborPoint(goalHarbor);
            // Go idle here (if harbor does not need it)
            startIdling();
            // Hafen Bescheid sagen, dass wir da sind (falls er überhaupt noch existiert)
            noBase* hb = goal.isValid() ? world->getNO(goal) : nullptr;
            if(hb && hb->getGOT() == GO_Type::NobHarborbuilding)
                static_cast<nobHarborBuilding*>(hb)->shipArrived(*this);
        }
        break;
        case Result::NoRouteFound:
        {
            MapPoint goal(world->getHarborPoint(goalHarbor));
            RTTR_Assert(goal.isValid());
            // Dem Hafen Bescheid sagen
            world->getSpecObj<nobHarborBuilding>(goal)->shipLost(this);
            startIdling();
        }
        break;
        case Result::HarborDoesntExist: startIdling(); break;
    }
}

void noShip::handleStateExpeditionDriving()
{
    Result res;
    // Zum Heimathafen fahren?
    if(homeHarbor == goalHarbor)
        res = driveToHarbour();
    else
        res = driveToHarbourPlace();

    switch(res)
    {
        case Result::Driving: return;
        case Result::GoalReached:
        {
            // Haben wir unsere Expedition beendet?
            if(homeHarbor == goalHarbor)
            {
                // Sachen wieder in den Hafen verladen
                state = State::ExpeditionUnloading;
                current_ev = getEvMgr().addEvent(this, UNLOADING_TIME, 1);
            } else
            {
                // Warten auf weitere Anweisungen
                state = State::ExpeditionWaiting;

                // Spieler benachrichtigen
                sendPostMessage(
                  ownerId_, std::make_unique<ShipPostMsg>(getEvMgr().getCurrentGF(),
                                                          _("A ship has reached the destination of its expedition."),
                                                          PostCategory::Economy, *this));
                world->getNotifications().publish(ExpeditionNote(ExpeditionNote::Waiting, ownerId_, pos));
            }
        }
        break;
        case Result::NoRouteFound:
        case Result::HarborDoesntExist: // should only happen when an expedition is cancelled and the home harbor no
                                        // longer exists
        {
            if(homeHarbor != goalHarbor && homeHarbor)
            {
                // Try to go back
                goalHarbor = homeHarbor;
                handleStateExpeditionDriving();
            } else
                findUnloadGoal(State::ExpeditionDriving); // Unload anywhere!
        }
        break;
    }
}

void noShip::handleStateExplorationExpeditionDriving()
{
    Result res;
    // Zum Heimathafen fahren?
    if(homeHarbor == goalHarbor)
        res = driveToHarbour();
    else
        res = driveToHarbourPlace();

    switch(res)
    {
        case Result::Driving: return;
        case Result::GoalReached:
        {
            // Haben wir unsere Expedition beendet?
            if(homeHarbor == goalHarbor)
            {
                // Dann sind wir fertig -> wieder entladen
                state = State::ExplorationexpeditionUnloading;
                current_ev = getEvMgr().addEvent(this, UNLOADING_TIME, 1);
            } else
            {
                // Strecke, die wir gefahren sind, draufaddieren
                covered_distance += route_.size();
                // Erstmal kurz ausruhen an diesem Punkt und das Rohr ausfahren, um ein bisschen
                // auf der Insel zu gucken
                state = State::ExplorationexpeditionWaiting;
                current_ev = getEvMgr().addEvent(this, EXPLORATION_EXPEDITION_WAITING_TIME, 1);
            }
        }
        break;
        case Result::NoRouteFound:
        case Result::HarborDoesntExist:
            if(homeHarbor != goalHarbor && homeHarbor)
            {
                // Try to go back
                goalHarbor = homeHarbor;
                handleStateExplorationExpeditionDriving();
            } else
                findUnloadGoal(State::ExplorationexpeditionDriving); // Unload anywhere!
            break;
    }
}

void noShip::handleStateTransportDriving()
{
    Result res = driveToHarbour();
    switch(res)
    {
        case Result::Driving: return;
        case Result::GoalReached:
        {
            // Waren abladen, dafür wieder kurze Zeit hier ankern
            state = State::TransportUnloading;
            current_ev = getEvMgr().addEvent(this, UNLOADING_TIME, 1);
        }
        break;
        case Result::NoRouteFound:
        case Result::HarborDoesntExist:
        {
            RTTR_Assert(!remaining_sea_attackers);
            // Kein Hafen mehr?
            // Dann müssen alle Leute ihren Heimatgebäuden Bescheid geben, dass sie
            // nun nicht mehr kommen
            // Das Schiff muss einen Notlandeplatz ansteuern
            // LOG.write(("transport goal harbor doesnt exist player %i state %i pos %u,%u \n",player,state,x,y);
            for(auto& figure : figures)
            {
                figure->abrogate();
                figure->setGoalTonullptr();
            }

            for(auto& ware : wares)
            {
                ware->notifyGoalAboutLostWare();
            }

            findUnloadGoal(State::TransportDriving);
        }
        break;
    }
}

void noShip::handleStateSeaAttackDriving()
{
    Result res = driveToHarbourPlace();
    switch(res)
    {
        case Result::Driving: return; // OK
        case Result::GoalReached:
            // Ziel erreicht, dann stellen wir das Schiff hier hin und die Soldaten laufen nacheinander raus zum Ziel
            state = State::SeaattackWaiting;
            current_ev = getEvMgr().addEvent(this, 15, 1);
            remaining_sea_attackers = figures.size();
            break;
        case Result::NoRouteFound:
        case Result::HarborDoesntExist:
            RTTR_Assert(goalHarbor != homeHarbor || !homeHarbor);
            abortSeaAttack();
            break;
    }
}

void noShip::handleStateSeaAttackReturn()
{
    Result res = driveToHarbour();
    switch(res)
    {
        case Result::Driving: return;
        case Result::GoalReached:
            // Entladen
            state = State::SeaattackUnloading;
            this->current_ev = getEvMgr().addEvent(this, UNLOADING_TIME, 1);
            break;
        case Result::HarborDoesntExist:
        case Result::NoRouteFound: abortSeaAttack(); break;
    }
}

/// Gibt zurück, ob das Schiff jetzt in der Lage wäre, eine Kolonie zu gründen
bool noShip::isAbleToFoundColony() const
{
    // Warten wir gerade?
    if(state == State::ExpeditionWaiting)
    {
        // We must always have a goal harbor
        RTTR_Assert(goalHarbor);
        // Ist der Punkt, an dem wir gerade ankern, noch frei?
        if(world->isHarborPointFree(goalHarbor, ownerId_))
            return true;
    }

    return false;
}

/// Gibt zurück, ob das Schiff einen bestimmten Hafen ansteuert
bool noShip::isGoingToHarbor(const nobHarborBuilding& hb) const
{
    if(goalHarbor != hb.getHarborPosID())
        return false;
    // Explicit switch to check all states
    switch(state)
    {
        case State::Idle:
        case State::ExpeditionLoading:
        case State::ExpeditionUnloading:
        case State::ExpeditionWaiting:
        case State::ExpeditionDriving:
        case State::ExplorationexpeditionLoading:
        case State::ExplorationexpeditionUnloading:
        case State::ExplorationexpeditionWaiting:
        case State::ExplorationexpeditionDriving:
        case State::SeaattackLoading:
        case State::SeaattackDrivingToDestination:
        case State::SeaattackWaiting: return false;
        case State::Gotoharbor:
        case State::TransportDriving:       // Driving to this harbor
        case State::TransportLoading:       // Loading at home harbor and going to goal
        case State::TransportUnloading:     // Unloading at this harbor
        case State::SeaattackUnloading:     // Unloading attackers at this harbor
        case State::SeaattackReturnDriving: // Returning attackers to this harbor
            return true;
    }
    RTTR_Assert(false);
    return false;
}

/// Belädt das Schiff mit Waren und Figuren, um eine Transportfahrt zu starten
void noShip::prepareTransport(HarborId homeHarborId, MapPoint goal, std::list<std::unique_ptr<noFigure>> figures,
                              std::list<std::unique_ptr<Ware>> wares)
{
    RTTR_Assert(homeHarborId);
    RTTR_Assert(pos == world->getCoastalPoint(homeHarborId, seaId_));
    this->homeHarbor = homeHarborId;
    // ID von Zielhafen herausfinden
    noBase* nb = world->getNO(goal);
    RTTR_Assert(nb->getGOT() == GO_Type::NobHarborbuilding);
    this->goalHarbor = static_cast<nobHarborBuilding*>(nb)->getHarborPosID();

    this->figures = std::move(figures);
    this->wares = std::move(wares);

    state = State::TransportLoading;
    current_ev = getEvMgr().addEvent(this, LOADING_TIME, 1);
}

/// Belädt das Schiff mit Schiffs-Angreifern
void noShip::prepareSeaAttack(HarborId homeHarborId, MapPoint goal, std::vector<std::unique_ptr<nofAttacker>> attackers)
{
    // Heimathafen merken
    RTTR_Assert(homeHarborId);
    RTTR_Assert(pos == world->getCoastalPoint(homeHarborId, seaId_));
    homeHarbor = homeHarborId;
    goalHarbor = world->getHarborPointID(goal);
    RTTR_Assert(goalHarbor);
    figures.clear();
    for(auto& attacker : attackers)
    {
        attacker->startShipJourney();
        attacker->seaAttackStarted();
        figures.push_back(std::move(attacker));
    }
    state = State::SeaattackLoading;
    current_ev = getEvMgr().addEvent(this, LOADING_TIME, 1);
}

/// Startet Schiffs-Angreiff
void noShip::startSeaAttack()
{
    state = State::SeaattackDrivingToDestination;
    startDrivingToHarborPlace();
    handleStateSeaAttackDriving();
}

void noShip::abortSeaAttack()
{
    RTTR_Assert(state != State::SeaattackWaiting); // figures are not aboard if this fails!
    RTTR_Assert(remaining_sea_attackers == 0);     // Some soldiers are still not aboard

    if((state == State::SeaattackLoading || state == State::SeaattackDrivingToDestination) && goalHarbor != homeHarbor
       && homeHarbor)
    {
        // We did not start the attack yet and we can (possibly) go back to our home harbor
        // -> tell the soldiers we go back (like after an attack)
        goalHarbor = homeHarbor;
        for(auto& figure : figures)
            checkedCast<nofAttacker*>(figure.get())->startReturnViaShip(*this);
        if(state == State::SeaattackLoading)
        {
            // We are still loading (loading event must be active)
            // -> Use it to unload
            RTTR_Assert(current_ev);
            state = State::SeaattackUnloading;
        } else
        {
            // Else start driving back
            state = State::SeaattackReturnDriving;
            handleStateSeaAttackReturn();
        }
    } else
    {
        // attack failed and we cannot go back to our home harbor
        // -> Tell figures that they won't go to their planned destination
        for(auto& figure : figures)
            checkedCast<nofAttacker*>(figure.get())->cancelSeaAttack();

        if(state == State::SeaattackLoading)
        {
            // Abort loading
            RTTR_Assert(current_ev);
            getEvMgr().removeEvent(current_ev);
        }

        // Das Schiff muss einen Notlandeplatz ansteuern
        findUnloadGoal(State::SeaattackReturnDriving);
    }
}

void noShip::startDrivingToHarborPlace()
{
    if(!goalHarbor)
    {
        route_.clear();
        curRouteIdx = 0;
        return;
    }

    MapPoint coastalPos = world->getCoastalPoint(goalHarbor, seaId_);
    if(pos == coastalPos)
        route_.clear();
    else
    {
        // if we still have and are at the home harbor get route directly
        if(homeHarbor && pos == world->getCoastalPoint(homeHarbor, seaId_))
        {
            route_ = world->getShipPathData().getHarborConnection(homeHarbor, goalHarbor, seaId_);
            RTTR_Assert(!route_.empty());
        } else
        {
            if(!world->findShipPathToHarbor(pos, goalHarbor, seaId_, &route_, nullptr))
            {
                // todo
                RTTR_Assert(false);
                LOG.write(
                  "WARNING: Bug detected (GF: %u). Please report this with the savegame and "
                  "replay.\nnoShip::StartDrivingToHarborPlace: Schiff hat keinen Weg gefunden!\nplayer %i state %i "
                  "pos %u,%u goal "
                  "coastal %u,%u goal-id %i goalpos %u,%u \n")
                  % getEvMgr().getCurrentGF() % unsigned(ownerId_) % unsigned(state) % pos.x % pos.y % coastalPos.x
                  % coastalPos.y % goalHarbor % world->getHarborPoint(goalHarbor).x
                  % world->getHarborPoint(goalHarbor).y;
                goalHarbor.reset();
                return;
            }
        }
    }
    curRouteIdx = 0;
}

/// Startet die eigentliche Transportaktion, nachdem das Schiff beladen wurde
void noShip::startTransport()
{
    state = State::TransportDriving;

    startDrivingToHarborPlace();
    // Einfach weiterfahren
    handleStateTransportDriving();
}

void noShip::findUnloadGoal(State newState)
{
    state = newState;
    // Das Schiff muss einen Notlandeplatz ansteuern
    // Neuen Hafen suchen
    if(world->getPlayer(ownerId_).findHarborForUnloading(this, pos, &goalHarbor, &route_, nullptr))
    {
        curRouteIdx = 0;
        homeHarbor = goalHarbor; // To allow unloading here
        if(state == State::ExpeditionDriving)
            handleStateExpeditionDriving();
        else if(state == State::ExplorationexpeditionDriving)
            handleStateExplorationExpeditionDriving();
        else if(state == State::TransportDriving)
            handleStateTransportDriving();
        else if(state == State::SeaattackReturnDriving)
            handleStateSeaAttackReturn();
        else
        {
            RTTR_Assert(false);
            LOG.write("Bug detected: Invalid state for FindUnloadGoal");
            findUnloadGoal(State::TransportDriving);
        }
    } else
    {
        // Ansonsten als verloren markieren, damit uns später Bescheid gesagt wird
        // wenn es einen neuen Hafen gibt
        homeHarbor.reset();
        goalHarbor.reset();
        lost = true;
    }
}

/// Sagt dem Schiff, das ein bestimmter Hafen zerstört wurde
void noShip::harborDestroyed(nobHarborBuilding* hb)
{
    const HarborId destroyedHarborId = hb->getHarborPosID();
    // Almost every case of a destroyed harbor is handled when the ships event fires (the handler detects the destroyed
    // harbor) So mostly we just reset the corresponding id

    if(destroyedHarborId == homeHarbor)
        homeHarbor.reset();

    // Ist unser Ziel betroffen?
    if(destroyedHarborId != goalHarbor)
        return;

    State oldState = state;

    switch(state)
    {
        default:
            // Just reset goal, but not for expeditions
            if(!isOnExpedition() && !isOnExplorationExpedition())
                goalHarbor.reset();
            return; // Skip the rest
        case State::TransportLoading:
        case State::TransportUnloading:
            // Tell wares and figures that they won't reach their goal
            for(auto& figure : figures)
            {
                figure->abrogate();
                figure->setGoalTonullptr();
            }
            for(auto& ware : wares)
            {
                // Notify goal only, if it is not the destroyed harbor. It already knows about that ;)
                if(ware->getGoal() != hb)
                    ware->notifyGoalAboutLostWare();
                else
                    ware->setGoal(nullptr);
            }
            break;
        case State::SeaattackLoading:
            // We could also just set the goal harbor id to 0 but this can reuse the event
            abortSeaAttack();
            break;
        case State::SeaattackUnloading: break;
    }

    // Are we currently getting the wares?
    if(oldState == State::TransportLoading)
    {
        RTTR_Assert(current_ev);
        if(homeHarbor)
        {
            // Then save us some time and unload immediately
            // goal is now the start harbor (if it still exists)
            goalHarbor = homeHarbor;
            state = State::TransportUnloading;
        } else
        {
            getEvMgr().removeEvent(current_ev);
            findUnloadGoal(State::TransportDriving);
        }
    } else if(oldState == State::TransportUnloading || oldState == State::SeaattackUnloading)
    {
        // Remove current unload event
        getEvMgr().removeEvent(current_ev);

        if(oldState == State::SeaattackUnloading)
            abortSeaAttack();
        else
            findUnloadGoal(State::TransportDriving);
    }
}

/// Fängt an mit idlen und setzt nötigen Sachen auf nullptr
void noShip::startIdling()
{
    // If those are not empty, then we are lost, not idling!
    RTTR_Assert(figures.empty());
    RTTR_Assert(wares.empty());
    RTTR_Assert(remaining_sea_attackers == 0);
    // Implicit contained wares/figures on expeditions
    RTTR_Assert(!isOnExplorationExpedition() || state == State::ExplorationexpeditionUnloading);
    RTTR_Assert(!isOnExpedition() || state == State::ExpeditionUnloading);

    homeHarbor.reset();
    goalHarbor.reset();
    state = State::Idle;
}

/// Sagt Bescheid, dass ein Schiffsangreifer nicht mehr mit nach Hause fahren will
void noShip::seaAttackerWishesNoReturn()
{
    RTTR_Assert(remaining_sea_attackers);
    RTTR_Assert(state == State::SeaattackWaiting);

    --remaining_sea_attackers;
    // Alle Soldaten an Bord
    if(remaining_sea_attackers == 0)
    {
        // Andere Events ggf. erstmal abmelden
        getEvMgr().removeEvent(current_ev);
        if(!figures.empty())
        {
            // Go back home. Note: home_harbor can be 0 if it was destroyed, allow this and let the state handlers
            // handle that case later
            goalHarbor = homeHarbor;
            state = State::SeaattackReturnDriving;
            startDrivingToHarborPlace();
            handleStateSeaAttackReturn();
        } else
        {
            // Wenn keine Soldaten mehr da sind können wir auch erstmal idlen
            startIdling();
            world->getPlayer(ownerId_).getJobForShip(*this);
        }
    }
}

/// Schiffs-Angreifer sind nach dem Angriff wieder zurückgekehrt
void noShip::addReturnedAttacker(std::unique_ptr<nofAttacker> attacker)
{
    RTTR_Assert(!helpers::containsPtr(figures, attacker.get()));

    figures.push_back(std::move(attacker));
    // Nun brauchen wir quasi einen Angreifer weniger
    seaAttackerWishesNoReturn();
}

/// Weist das Schiff an, seine Erkundungs-Expedition fortzusetzen
void noShip::continueExplorationExpedition()
{
    // Sind wir schon über unserem Limit, also zu weit gefahren
    if(covered_distance >= MAX_EXPLORATION_EXPEDITION_DISTANCE)
    {
        // Dann steuern wir unseren Heimathafen an!
        goalHarbor = homeHarbor;
    } else
    {
        // Find the next harbor spot to explore
        std::vector<HarborId> hps;
        if(goalHarbor)
            hps = world->getUnexploredHarborPoints(goalHarbor, seaId_, getPlayerId());

        // No possible spots? -> Go home
        if(hps.empty())
            goalHarbor = homeHarbor;
        else
        {
            // Choose one randomly
            goalHarbor = RANDOM_ELEMENT(hps);
        }
    }

    startDrivingToHarborPlace();
    state = State::ExplorationexpeditionDriving;
    handleStateExplorationExpeditionDriving();
}

/// Sagt dem Schiff, dass ein neuer Hafen erbaut wurde
void noShip::newHarborBuilt(nobHarborBuilding* hb)
{
    if(!lost)
        return;
    // Liegt der Hafen auch am Meer von diesem Schiff?
    if(!world->isHarborAtSea(hb->getHarborPosID(), seaId_))
        return;

    // LOG.write(("lost ship has new goal harbor player %i state %i pos %u,%u \n",player,state,x,y);
    homeHarbor = goalHarbor = hb->getHarborPosID();
    lost = false;

    startDrivingToHarborPlace();

    switch(state)
    {
        case State::ExplorationexpeditionDriving:
        case State::ExpeditionDriving:
        case State::TransportDriving:
        case State::SeaattackReturnDriving: driven(); break;
        default:
            RTTR_Assert(false); // Das darf eigentlich nicht passieren
            LOG.write("Bug detected: Invalid state in NewHarborBuilt");
            break;
    }
}
