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
    : noMovable(NodalObjectType::Ship, pos), ownerId_(player), state(State::Idle), seaId_(0), goal_harborId(0),
      goal_dir(0), name(RANDOM_ELEMENT(ship_names[world->GetPlayer(player).nation])), curRouteIdx(0), lost(false),
      remaining_sea_attackers(0), home_harbor(0), covered_distance(0)
{
    // Meer ermitteln, auf dem dieses Schiff fährt
    for(const auto dir : helpers::EnumRange<Direction>{})
    {
        unsigned short seaId = world->GetNeighbourNode(pos, dir).seaId;
        if(seaId)
            this->seaId_ = seaId;
    }

    // Auf irgendeinem Meer müssen wir ja sein
    RTTR_Assert(seaId_ > 0);
}

noShip::~noShip() = default;

void noShip::Serialize(SerializedGameData& sgd) const
{
    noMovable::Serialize(sgd);

    sgd.PushUnsignedChar(ownerId_);
    sgd.PushEnum<uint8_t>(state);
    sgd.PushUnsignedShort(seaId_);
    sgd.PushUnsignedInt(goal_harborId);
    sgd.PushUnsignedChar(goal_dir);
    sgd.PushString(name);
    sgd.PushUnsignedInt(curRouteIdx);
    sgd.PushBool(lost);
    sgd.PushUnsignedInt(remaining_sea_attackers);
    sgd.PushUnsignedInt(home_harbor);
    sgd.PushUnsignedInt(covered_distance);
    helpers::pushContainer(sgd, route_);
    sgd.PushObjectContainer(figures);
    sgd.PushObjectContainer(wares, true);
}

noShip::noShip(SerializedGameData& sgd, const unsigned obj_id)
    : noMovable(sgd, obj_id), ownerId_(sgd.PopUnsignedChar()), state(sgd.Pop<State>()), seaId_(sgd.PopUnsignedShort()),
      goal_harborId(sgd.PopUnsignedInt()), goal_dir(sgd.PopUnsignedChar()),
      name(sgd.GetGameDataVersion() < 2 ? sgd.PopLongString() : sgd.PopString()), curRouteIdx(sgd.PopUnsignedInt()),
      route_(sgd.GetGameDataVersion() < 7 ? sgd.PopUnsignedInt() : 0), lost(sgd.PopBool()),
      remaining_sea_attackers(sgd.PopUnsignedInt()), home_harbor(sgd.PopUnsignedInt()),
      covered_distance(sgd.PopUnsignedInt())
{
    helpers::popContainer(sgd, route_, sgd.GetGameDataVersion() < 7);
    sgd.PopObjectContainer(figures);
    sgd.PopObjectContainer(wares, GO_Type::Ware);
}

void noShip::Destroy()
{
    RTTR_Assert(figures.empty());
    RTTR_Assert(wares.empty());
    world->GetNotifications().publish(ShipNote(ShipNote::Destroyed, ownerId_, pos));
    // Schiff wieder abmelden
    world->GetPlayer(ownerId_).RemoveShip(*this);
}

void noShip::Draw(DrawPoint drawPt)
{
    unsigned flag_drawing_type = 1;

    // Sind wir verloren? Dann immer stehend zeichnen
    if(lost)
    {
        DrawFixed(drawPt, true);
        return;
    }

    switch(state)
    {
        default: break;
        case State::Idle:
        case State::SeaattackWaiting:
        {
            DrawFixed(drawPt, false);
            flag_drawing_type = 0;
        }
        break;

        case State::Gotoharbor:
        {
            DrawDriving(drawPt);
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
            DrawFixed(drawPt, false);
        }
        break;
        case State::ExplorationexpeditionWaiting:
        case State::ExpeditionWaiting:
        {
            DrawFixed(drawPt, true);
        }
        break;
        case State::ExpeditionDriving:
        case State::TransportDriving:
        case State::SeaattackDrivingToDestination:
        case State::ExplorationexpeditionDriving:
        {
            DrawDrivingWithWares(drawPt);
        }
        break;
        case State::SeaattackReturnDriving:
        {
            if(!figures.empty() || !wares.empty())
                DrawDrivingWithWares(drawPt);
            else
                DrawDriving(drawPt);
        }
        break;
    }

    LOADER.GetPlayerImage("boot_z", 40 + GAMECLIENT.GetGlobalAnimation(6, 1, 1, GetObjId()))
      ->DrawFull(drawPt + SHIPS_FLAG_POS[flag_drawing_type][GetCurMoveDir()], COLOR_WHITE,
                 world->GetPlayer(ownerId_).color);
    // Second, white flag, only when on expedition, always swinging in the opposite direction
    if(state >= State::ExpeditionLoading && state <= State::ExpeditionDriving)
        LOADER.GetPlayerImage("boot_z", 40 + GAMECLIENT.GetGlobalAnimation(6, 1, 1, GetObjId() + 4))
          ->DrawFull(drawPt + SHIPS_FLAG_POS[flag_drawing_type][GetCurMoveDir()]);
}

/// Zeichnet das Schiff stehend mit oder ohne Waren
void noShip::DrawFixed(DrawPoint drawPt, const bool draw_wares)
{
    LOADER.GetImageN("boot_z", rttr::enum_cast(GetCurMoveDir() + 3u) * 2 + 1)->DrawFull(drawPt, COLOR_SHADOW);
    LOADER.GetImageN("boot_z", rttr::enum_cast(GetCurMoveDir() + 3u) * 2)->DrawFull(drawPt);

    if(draw_wares)
        /// Waren zeichnen
        LOADER.GetImageN("boot_z", 30 + rttr::enum_cast(GetCurMoveDir() + 3u))->DrawFull(drawPt);
}

/// Zeichnet normales Fahren auf dem Meer ohne irgendwelche Güter
void noShip::DrawDriving(DrawPoint& drawPt)
{
    // Interpolieren zwischen beiden Knotenpunkten
    drawPt += CalcWalkingRelative();

    LOADER.GetImageN("boot_z", 13 + rttr::enum_cast(GetCurMoveDir() + 3u) * 2)->DrawFull(drawPt, COLOR_SHADOW);
    LOADER.GetImageN("boot_z", 12 + rttr::enum_cast(GetCurMoveDir() + 3u) * 2)->DrawFull(drawPt);
}

/// Zeichnet normales Fahren auf dem Meer mit Gütern
void noShip::DrawDrivingWithWares(DrawPoint& drawPt)
{
    DrawDriving(drawPt);
    /// Waren zeichnen
    LOADER.GetImageN("boot_z", 30 + rttr::enum_cast(GetCurMoveDir() + 3u))->DrawFull(drawPt);
}

void noShip::HandleEvent(const unsigned id)
{
    RTTR_Assert(current_ev);
    RTTR_Assert(current_ev->id == id);
    current_ev = nullptr;

    if(id == 0)
    {
        // Move event
        // neue Position einnehmen
        Walk();
        // entscheiden, was als nächstes zu tun ist
        Driven();
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
                SendPostMessage(ownerId_, std::make_unique<ShipPostMsg>(GetEvMgr().GetCurrentGF(),
                                                                        _("A ship is ready for an expedition."),
                                                                        PostCategory::Economy, *this));
                world->GetNotifications().publish(ExpeditionNote(ExpeditionNote::Waiting, ownerId_, pos));
                break;
            case State::ExplorationexpeditionLoading:
            case State::ExplorationexpeditionWaiting:
                // Schiff ist nun bereit und Expedition kann beginnen
                ContinueExplorationExpedition();
                break;
            case State::ExpeditionUnloading:
            {
                // Hafen herausfinden
                noBase* hb = goal_harborId ? world->GetNO(world->GetHarborPoint(goal_harborId)) : nullptr;

                if(hb && hb->GetGOT() == GO_Type::NobHarborbuilding)
                {
                    Inventory goods;
                    goods.goods[GoodType::Boards] = BUILDING_COSTS[BuildingType::HarborBuilding].boards;
                    goods.goods[GoodType::Stones] = BUILDING_COSTS[BuildingType::HarborBuilding].stones;
                    goods.people[Job::Builder] = 1;
                    static_cast<nobBaseWarehouse*>(hb)->AddGoods(goods, false);
                    // Wieder idlen und ggf. neuen Job suchen
                    StartIdling();
                    world->GetPlayer(ownerId_).GetJobForShip(*this);
                } else
                {
                    // target harbor for unloading doesnt exist anymore -> set state to driving and handle the new state
                    state = State::ExpeditionDriving;
                    HandleState_ExpeditionDriving();
                }
                break;
            }
            case State::ExplorationexpeditionUnloading:
            {
                // Hafen herausfinden
                noBase* hb = goal_harborId ? world->GetNO(world->GetHarborPoint(goal_harborId)) : nullptr;

                unsigned old_visual_range = GetVisualRange();

                if(hb && hb->GetGOT() == GO_Type::NobHarborbuilding)
                {
                    // Späher wieder entladen
                    Inventory goods;
                    goods.people[Job::Scout] = world->GetGGS().GetNumScoutsExpedition();
                    static_cast<nobBaseWarehouse*>(hb)->AddGoods(goods, false);
                    // Wieder idlen und ggf. neuen Job suchen
                    StartIdling();
                    world->GetPlayer(ownerId_).GetJobForShip(*this);
                } else
                {
                    // target harbor for unloading doesnt exist anymore -> set state to driving and handle the new state
                    state = State::ExplorationexpeditionDriving;
                    HandleState_ExplorationExpeditionDriving();
                }

                // Sichtbarkeiten neu berechnen
                world->RecalcVisibilitiesAroundPoint(pos, old_visual_range, ownerId_, nullptr);

                break;
            }
            case State::TransportLoading: StartTransport(); break;
            case State::TransportUnloading:
            case State::SeaattackUnloading:
            {
                // Hafen herausfinden
                RTTR_Assert(state == State::SeaattackUnloading || remaining_sea_attackers == 0);
                noBase* hb = goal_harborId ? world->GetNO(world->GetHarborPoint(goal_harborId)) : nullptr;
                if(hb && hb->GetGOT() == GO_Type::NobHarborbuilding)
                {
                    static_cast<nobHarborBuilding*>(hb)->ReceiveGoodsFromShip(figures, wares);
                    figures.clear();
                    wares.clear();

                    state = State::TransportUnloading;
                    // Hafen bescheid sagen, dass er das Schiff nun nutzen kann
                    static_cast<nobHarborBuilding*>(hb)->ShipArrived(*this);

                    // Hafen hat keinen Job für uns?
                    if(state == State::TransportUnloading)
                    {
                        // Wieder idlen und ggf. neuen Job suchen
                        StartIdling();
                        world->GetPlayer(ownerId_).GetJobForShip(*this);
                    }
                } else
                {
                    // target harbor for unloading doesnt exist anymore -> set state to driving and handle the new state
                    if(state == State::TransportUnloading)
                        FindUnloadGoal(State::TransportDriving);
                    else
                        FindUnloadGoal(State::SeaattackReturnDriving);
                }
                break;
            }
            case State::SeaattackLoading: StartSeaAttack(); break;
            case State::SeaattackWaiting:
            {
                // Nächsten Soldaten nach draußen beordern
                if(figures.empty())
                    break;

                // Evtl. ist ein Angreifer schon fertig und wieder an Board gegangen
                // der darf dann natürlich nicht noch einmal raus, sonst kann die schöne Reise
                // böse enden
                if(static_cast<nofAttacker&>(*figures.front()).IsSeaAttackCompleted())
                    break;

                auto& attacker = world->AddFigure(pos, std::move(figures.front()));
                figures.pop_front();

                current_ev = GetEvMgr().AddEvent(this, 30, 1);
                static_cast<nofAttacker&>(attacker).StartAttackOnOtherIsland(pos, GetObjId());
                break;
            }
        }
    }
}

void noShip::StartDriving(const Direction dir)
{
    const std::array<unsigned, 5> SHIP_SPEEDS = {35, 25, 20, 10, 5};

    StartMoving(dir, SHIP_SPEEDS[world->GetGGS().getSelection(AddonId::SHIP_SPEED)]);
}

void noShip::Driven()
{
    MapPoint enemy_territory_discovered(MapPoint::Invalid());
    world->RecalcMovingVisibilities(pos, ownerId_, GetVisualRange(), GetCurMoveDir(), &enemy_territory_discovered);

    // Feindliches Territorium entdeckt?
    if(enemy_territory_discovered.isValid())
    {
        // Send message if necessary
        if(world->GetPlayer(ownerId_).ShipDiscoveredHostileTerritory(enemy_territory_discovered))
            SendPostMessage(ownerId_, std::make_unique<PostMsg>(GetEvMgr().GetCurrentGF(),
                                                                _("A ship disovered an enemy territory"),
                                                                PostCategory::Military, enemy_territory_discovered));
    }

    switch(state)
    {
        case State::Gotoharbor: HandleState_GoToHarbor(); break;
        case State::ExpeditionDriving: HandleState_ExpeditionDriving(); break;
        case State::ExplorationexpeditionDriving: HandleState_ExplorationExpeditionDriving(); break;
        case State::TransportDriving: HandleState_TransportDriving(); break;
        case State::SeaattackDrivingToDestination: HandleState_SeaAttackDriving(); break;
        case State::SeaattackReturnDriving: HandleState_SeaAttackReturn(); break;
        default: RTTR_Assert(false); break;
    }
}

bool noShip::IsLoading() const
{
    return state == State::ExpeditionLoading || state == State::ExplorationexpeditionLoading
           || state == State::TransportLoading || state == State::SeaattackLoading;
}

bool noShip::IsUnloading() const
{
    return state == State::ExpeditionUnloading || state == State::ExplorationexpeditionUnloading
           || state == State::TransportUnloading || state == State::SeaattackUnloading;
}

bool noShip::IsOnBoard(const noFigure& figure) const
{
    return helpers::containsPtr(figures, &figure);
}

/// Gibt Sichtradius dieses Schiffes zurück
unsigned noShip::GetVisualRange() const
{
    // Erkundungsschiffe haben einen größeren Sichtbereich
    if(state >= State::ExplorationexpeditionLoading && state <= State::ExplorationexpeditionDriving)
        return VISUALRANGE_EXPLORATION_SHIP;
    else
        return VISUALRANGE_SHIP;
}

/// Fährt zum Hafen, um dort eine Mission (Expedition) zu erledigen
void noShip::GoToHarbor(const nobHarborBuilding& hb, const std::vector<Direction>& route)
{
    RTTR_Assert(state == State::Idle); // otherwise we might carry wares etc
    RTTR_Assert(figures.empty());
    RTTR_Assert(wares.empty());
    RTTR_Assert(remaining_sea_attackers == 0);

    state = State::Gotoharbor;

    goal_harborId = world->GetNode(hb.GetPos()).harborId;
    RTTR_Assert(goal_harborId);

    // Route merken
    this->route_ = route;
    curRouteIdx = 1;

    // losfahren
    StartDriving(route[0]);
}

/// Startet eine Expedition
void noShip::StartExpedition(unsigned homeHarborId)
{
    /// Schiff wird "beladen", also kurze Zeit am Hafen stehen, bevor wir bereit sind
    state = State::ExpeditionLoading;
    current_ev = GetEvMgr().AddEvent(this, LOADING_TIME, 1);
    RTTR_Assert(homeHarborId);
    RTTR_Assert(pos == world->GetCoastalPoint(homeHarborId, seaId_));
    home_harbor = homeHarborId;
    goal_harborId = homeHarborId; // This is current goal (commands are relative to current goal)
}

/// Startet eine Erkundungs-Expedition
void noShip::StartExplorationExpedition(unsigned homeHarborId)
{
    /// Schiff wird "beladen", also kurze Zeit am Hafen stehen, bevor wir bereit sind
    state = State::ExplorationexpeditionLoading;
    current_ev = GetEvMgr().AddEvent(this, LOADING_TIME, 1);
    covered_distance = 0;
    RTTR_Assert(homeHarborId);
    RTTR_Assert(pos == world->GetCoastalPoint(homeHarborId, seaId_));
    home_harbor = homeHarborId;
    goal_harborId = homeHarborId; // This is current goal (commands are relative to current goal)
    // Sichtbarkeiten neu berechnen
    world->MakeVisibleAroundPoint(pos, GetVisualRange(), ownerId_);
}

/// Fährt weiter zu einem Hafen
noShip::Result noShip::DriveToHarbour()
{
    if(!goal_harborId)
        return Result::HarborDoesntExist;

    MapPoint goal(world->GetHarborPoint(goal_harborId));
    RTTR_Assert(goal.isValid());

    // Existiert der Hafen überhaupt noch?
    if(world->GetGOT(goal) != GO_Type::NobHarborbuilding)
        return Result::HarborDoesntExist;

    return DriveToHarbourPlace();
}

/// Fährt weiter zu Hafenbauplatz
noShip::Result noShip::DriveToHarbourPlace()
{
    if(goal_harborId == 0)
        return Result::HarborDoesntExist;

    // Sind wir schon da?
    if(curRouteIdx == route_.size())
        return Result::GoalReached;

    MapPoint goalRoutePos;

    // Route überprüfen
    if(!world->CheckShipRoute(pos, route_, curRouteIdx, &goalRoutePos))
    {
        // Route kann nicht mehr passiert werden --> neue Route suchen
        if(!world->FindShipPathToHarbor(pos, goal_harborId, seaId_, &route_, nullptr))
        {
            // Wieder keine gefunden -> raus
            return Result::NoRouteFound;
        }

        // Wir fangen bei der neuen Route wieder von vorne an
        curRouteIdx = 0;
    } else if(goalRoutePos != world->GetCoastalPoint(goal_harborId, seaId_))
    {
        // Our goal point of the current route has changed
        // If we are close to it, recalculate the route
        RTTR_Assert(route_.size() >= curRouteIdx);
        if(route_.size() - curRouteIdx < 10)
        {
            if(!world->FindShipPathToHarbor(pos, goal_harborId, seaId_, &route_, nullptr))
                // Keiner gefunden -> raus
                return Result::NoRouteFound;

            curRouteIdx = 0;
        }
    }

    StartDriving(route_[curRouteIdx++]);
    return Result::Driving;
}

unsigned noShip::GetCurrentHarbor() const
{
    RTTR_Assert(state == State::ExpeditionWaiting);
    return goal_harborId;
}

unsigned noShip::GetTargetHarbor() const
{
    return goal_harborId;
}

unsigned noShip::GetHomeHarbor() const
{
    return home_harbor;
}

/// Weist das Schiff an, in einer bestimmten Richtung die Expedition fortzusetzen
void noShip::ContinueExpedition(const ShipDirection dir)
{
    if(state != State::ExpeditionWaiting)
        return;

    // Nächsten Hafenpunkt in dieser Richtung suchen
    unsigned new_goal = world->GetNextFreeHarborPoint(pos, goal_harborId, dir, ownerId_);

    // Auch ein Ziel gefunden?
    if(!new_goal)
        return;

    // Versuchen, Weg zu finden
    if(!world->FindShipPathToHarbor(pos, new_goal, seaId_, &route_, nullptr))
        return;

    // Dann fahren wir da mal hin
    curRouteIdx = 0;
    goal_harborId = new_goal;
    state = State::ExpeditionDriving;

    StartDriving(route_[curRouteIdx++]);
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
    if(goal_harborId == home_harbor)
    {
        route_.clear();
        curRouteIdx = 0;
        state = State::ExpeditionDriving; // just in case the home harbor was destroyed
        HandleState_ExpeditionDriving();
    } else
    {
        state = State::ExpeditionDriving;
        goal_harborId = home_harbor;
        StartDrivingToHarborPlace();
        HandleState_ExpeditionDriving();
    }
}

/// Weist das Schiff an, an der aktuellen Position einen Hafen zu gründen
void noShip::FoundColony()
{
    if(state != State::ExpeditionWaiting)
        return;

    // Kolonie gründen
    if(world->FoundColony(goal_harborId, ownerId_, seaId_))
    {
        // For checks
        state = State::ExpeditionUnloading;
        // Dann idlen wir wieder
        StartIdling();
        // Neue Arbeit suchen
        world->GetPlayer(ownerId_).GetJobForShip(*this);
    } else // colony founding FAILED
        world->GetNotifications().publish(ExpeditionNote(ExpeditionNote::Waiting, ownerId_, pos));
}

void noShip::HandleState_GoToHarbor()
{
    // Hafen schon zerstört?
    if(goal_harborId == 0)
    {
        StartIdling();
        return;
    }

    Result res = DriveToHarbour();
    switch(res)
    {
        case Result::Driving: return; // Continue
        case Result::GoalReached:
        {
            MapPoint goal(world->GetHarborPoint(goal_harborId));
            RTTR_Assert(goal.isValid());
            // Go idle here (if harbor does not need it)
            StartIdling();
            // Hafen Bescheid sagen, dass wir da sind (falls er überhaupt noch existiert)
            noBase* hb = goal.isValid() ? world->GetNO(goal) : nullptr;
            if(hb && hb->GetGOT() == GO_Type::NobHarborbuilding)
                static_cast<nobHarborBuilding*>(hb)->ShipArrived(*this);
        }
        break;
        case Result::NoRouteFound:
        {
            MapPoint goal(world->GetHarborPoint(goal_harborId));
            RTTR_Assert(goal.isValid());
            // Dem Hafen Bescheid sagen
            world->GetSpecObj<nobHarborBuilding>(goal)->ShipLost(this);
            StartIdling();
        }
        break;
        case Result::HarborDoesntExist: StartIdling(); break;
    }
}

void noShip::HandleState_ExpeditionDriving()
{
    Result res;
    // Zum Heimathafen fahren?
    if(home_harbor == goal_harborId)
        res = DriveToHarbour();
    else
        res = DriveToHarbourPlace();

    switch(res)
    {
        case Result::Driving: return;
        case Result::GoalReached:
        {
            // Haben wir unsere Expedition beendet?
            if(home_harbor == goal_harborId)
            {
                // Sachen wieder in den Hafen verladen
                state = State::ExpeditionUnloading;
                current_ev = GetEvMgr().AddEvent(this, UNLOADING_TIME, 1);
            } else
            {
                // Warten auf weitere Anweisungen
                state = State::ExpeditionWaiting;

                // Spieler benachrichtigen
                SendPostMessage(
                  ownerId_, std::make_unique<ShipPostMsg>(GetEvMgr().GetCurrentGF(),
                                                          _("A ship has reached the destination of its expedition."),
                                                          PostCategory::Economy, *this));
                world->GetNotifications().publish(ExpeditionNote(ExpeditionNote::Waiting, ownerId_, pos));
            }
        }
        break;
        case Result::NoRouteFound:
        case Result::HarborDoesntExist: // should only happen when an expedition is cancelled and the home harbor no
                                        // longer exists
        {
            if(home_harbor != goal_harborId && home_harbor != 0)
            {
                // Try to go back
                goal_harborId = home_harbor;
                HandleState_ExpeditionDriving();
            } else
                FindUnloadGoal(State::ExpeditionDriving); // Unload anywhere!
        }
        break;
    }
}

void noShip::HandleState_ExplorationExpeditionDriving()
{
    Result res;
    // Zum Heimathafen fahren?
    if(home_harbor == goal_harborId)
        res = DriveToHarbour();
    else
        res = DriveToHarbourPlace();

    switch(res)
    {
        case Result::Driving: return;
        case Result::GoalReached:
        {
            // Haben wir unsere Expedition beendet?
            if(home_harbor == goal_harborId)
            {
                // Dann sind wir fertig -> wieder entladen
                state = State::ExplorationexpeditionUnloading;
                current_ev = GetEvMgr().AddEvent(this, UNLOADING_TIME, 1);
            } else
            {
                // Strecke, die wir gefahren sind, draufaddieren
                covered_distance += route_.size();
                // Erstmal kurz ausruhen an diesem Punkt und das Rohr ausfahren, um ein bisschen
                // auf der Insel zu gucken
                state = State::ExplorationexpeditionWaiting;
                current_ev = GetEvMgr().AddEvent(this, EXPLORATION_EXPEDITION_WAITING_TIME, 1);
            }
        }
        break;
        case Result::NoRouteFound:
        case Result::HarborDoesntExist:
            if(home_harbor != goal_harborId && home_harbor != 0)
            {
                // Try to go back
                goal_harborId = home_harbor;
                HandleState_ExplorationExpeditionDriving();
            } else
                FindUnloadGoal(State::ExplorationexpeditionDriving); // Unload anywhere!
            break;
    }
}

void noShip::HandleState_TransportDriving()
{
    Result res = DriveToHarbour();
    switch(res)
    {
        case Result::Driving: return;
        case Result::GoalReached:
        {
            // Waren abladen, dafür wieder kurze Zeit hier ankern
            state = State::TransportUnloading;
            current_ev = GetEvMgr().AddEvent(this, UNLOADING_TIME, 1);
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
                figure->Abrogate();
                figure->SetGoalTonullptr();
            }

            for(auto& ware : wares)
            {
                ware->NotifyGoalAboutLostWare();
            }

            FindUnloadGoal(State::TransportDriving);
        }
        break;
    }
}

void noShip::HandleState_SeaAttackDriving()
{
    Result res = DriveToHarbourPlace();
    switch(res)
    {
        case Result::Driving: return; // OK
        case Result::GoalReached:
            // Ziel erreicht, dann stellen wir das Schiff hier hin und die Soldaten laufen nacheinander raus zum Ziel
            state = State::SeaattackWaiting;
            current_ev = GetEvMgr().AddEvent(this, 15, 1);
            remaining_sea_attackers = figures.size();
            break;
        case Result::NoRouteFound:
        case Result::HarborDoesntExist:
            RTTR_Assert(goal_harborId != home_harbor || home_harbor == 0);
            AbortSeaAttack();
            break;
    }
}

void noShip::HandleState_SeaAttackReturn()
{
    Result res = DriveToHarbour();
    switch(res)
    {
        case Result::Driving: return;
        case Result::GoalReached:
            // Entladen
            state = State::SeaattackUnloading;
            this->current_ev = GetEvMgr().AddEvent(this, UNLOADING_TIME, 1);
            break;
        case Result::HarborDoesntExist:
        case Result::NoRouteFound: AbortSeaAttack(); break;
    }
}

/// Gibt zurück, ob das Schiff jetzt in der Lage wäre, eine Kolonie zu gründen
bool noShip::IsAbleToFoundColony() const
{
    // Warten wir gerade?
    if(state == State::ExpeditionWaiting)
    {
        // We must always have a goal harbor
        RTTR_Assert(goal_harborId);
        // Ist der Punkt, an dem wir gerade ankern, noch frei?
        if(world->IsHarborPointFree(goal_harborId, ownerId_))
            return true;
    }

    return false;
}

/// Gibt zurück, ob das Schiff einen bestimmten Hafen ansteuert
bool noShip::IsGoingToHarbor(const nobHarborBuilding& hb) const
{
    if(goal_harborId != hb.GetHarborPosID())
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
void noShip::PrepareTransport(unsigned homeHarborId, MapPoint goal, std::list<std::unique_ptr<noFigure>> figures,
                              std::list<std::unique_ptr<Ware>> wares)
{
    RTTR_Assert(homeHarborId);
    RTTR_Assert(pos == world->GetCoastalPoint(homeHarborId, seaId_));
    this->home_harbor = homeHarborId;
    // ID von Zielhafen herausfinden
    noBase* nb = world->GetNO(goal);
    RTTR_Assert(nb->GetGOT() == GO_Type::NobHarborbuilding);
    this->goal_harborId = static_cast<nobHarborBuilding*>(nb)->GetHarborPosID();

    this->figures = std::move(figures);
    this->wares = std::move(wares);

    state = State::TransportLoading;
    current_ev = GetEvMgr().AddEvent(this, LOADING_TIME, 1);
}

/// Belädt das Schiff mit Schiffs-Angreifern
void noShip::PrepareSeaAttack(unsigned homeHarborId, MapPoint goal, std::vector<std::unique_ptr<nofAttacker>> attackers)
{
    // Heimathafen merken
    RTTR_Assert(homeHarborId);
    RTTR_Assert(pos == world->GetCoastalPoint(homeHarborId, seaId_));
    home_harbor = homeHarborId;
    goal_harborId = world->GetHarborPointID(goal);
    RTTR_Assert(goal_harborId);
    figures.clear();
    for(auto& attacker : attackers)
    {
        attacker->StartShipJourney();
        attacker->SeaAttackStarted();
        figures.push_back(std::move(attacker));
    }
    state = State::SeaattackLoading;
    current_ev = GetEvMgr().AddEvent(this, LOADING_TIME, 1);
}

/// Startet Schiffs-Angreiff
void noShip::StartSeaAttack()
{
    state = State::SeaattackDrivingToDestination;
    StartDrivingToHarborPlace();
    HandleState_SeaAttackDriving();
}

void noShip::AbortSeaAttack()
{
    RTTR_Assert(state != State::SeaattackWaiting); // figures are not aboard if this fails!
    RTTR_Assert(remaining_sea_attackers == 0);     // Some soldiers are still not aboard

    if((state == State::SeaattackLoading || state == State::SeaattackDrivingToDestination)
       && goal_harborId != home_harbor && home_harbor != 0)
    {
        // We did not start the attack yet and we can (possibly) go back to our home harbor
        // -> tell the soldiers we go back (like after an attack)
        goal_harborId = home_harbor;
        for(auto& figure : figures)
            checkedCast<nofAttacker*>(figure.get())->StartReturnViaShip(*this);
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
            HandleState_SeaAttackReturn();
        }
    } else
    {
        // attack failed and we cannot go back to our home harbor
        // -> Tell figures that they won't go to their planned destination
        for(auto& figure : figures)
            checkedCast<nofAttacker*>(figure.get())->CancelSeaAttack();

        if(state == State::SeaattackLoading)
        {
            // Abort loading
            RTTR_Assert(current_ev);
            GetEvMgr().RemoveEvent(current_ev);
        }

        // Das Schiff muss einen Notlandeplatz ansteuern
        FindUnloadGoal(State::SeaattackReturnDriving);
    }
}

/// Fängt an zu einem Hafen zu fahren (berechnet Route usw.)
void noShip::StartDrivingToHarborPlace()
{
    if(!goal_harborId)
    {
        route_.clear();
        curRouteIdx = 0;
        return;
    }

    MapPoint coastalPos = world->GetCoastalPoint(goal_harborId, seaId_);
    if(pos == coastalPos)
        route_.clear();
    else
    {
        bool routeFound;
        // Use upper bound to distance by checking the distance between the harbors if we still have and are at the home
        // harbor
        if(home_harbor && pos == world->GetCoastalPoint(home_harbor, seaId_))
        {
            // Use the maximum distance between the harbors plus 6 fields
            unsigned maxDistance = world->CalcHarborDistance(home_harbor, goal_harborId) + 6;
            routeFound = world->FindShipPath(pos, coastalPos, maxDistance, &route_, nullptr);
        } else
            routeFound = world->FindShipPathToHarbor(pos, goal_harborId, seaId_, &route_, nullptr);
        if(!routeFound)
        {
            // todo
            RTTR_Assert(false);
            LOG.write("WARNING: Bug detected (GF: %u). Please report this with the savegame and "
                      "replay.\nnoShip::StartDrivingToHarborPlace: Schiff hat keinen Weg gefunden!\nplayer %i state %i "
                      "pos %u,%u goal "
                      "coastal %u,%u goal-id %i goalpos %u,%u \n")
              % GetEvMgr().GetCurrentGF() % unsigned(ownerId_) % unsigned(state) % pos.x % pos.y % coastalPos.x
              % coastalPos.y % goal_harborId % world->GetHarborPoint(goal_harborId).x
              % world->GetHarborPoint(goal_harborId).y;
            goal_harborId = 0;
            return;
        }
    }
    curRouteIdx = 0;
}

/// Startet die eigentliche Transportaktion, nachdem das Schiff beladen wurde
void noShip::StartTransport()
{
    state = State::TransportDriving;

    StartDrivingToHarborPlace();
    // Einfach weiterfahren
    HandleState_TransportDriving();
}

void noShip::FindUnloadGoal(State newState)
{
    state = newState;
    // Das Schiff muss einen Notlandeplatz ansteuern
    // Neuen Hafen suchen
    if(world->GetPlayer(ownerId_).FindHarborForUnloading(this, pos, &goal_harborId, &route_, nullptr))
    {
        curRouteIdx = 0;
        home_harbor = goal_harborId; // To allow unloading here
        if(state == State::ExpeditionDriving)
            HandleState_ExpeditionDriving();
        else if(state == State::ExplorationexpeditionDriving)
            HandleState_ExplorationExpeditionDriving();
        else if(state == State::TransportDriving)
            HandleState_TransportDriving();
        else if(state == State::SeaattackReturnDriving)
            HandleState_SeaAttackReturn();
        else
        {
            RTTR_Assert(false);
            LOG.write("Bug detected: Invalid state for FindUnloadGoal");
            FindUnloadGoal(State::TransportDriving);
        }
    } else
    {
        // Ansonsten als verloren markieren, damit uns später Bescheid gesagt wird
        // wenn es einen neuen Hafen gibt
        home_harbor = goal_harborId = 0;
        lost = true;
    }
}

/// Sagt dem Schiff, das ein bestimmter Hafen zerstört wurde
void noShip::HarborDestroyed(nobHarborBuilding* hb)
{
    const unsigned destroyedHarborId = hb->GetHarborPosID();
    // Almost every case of a destroyed harbor is handled when the ships event fires (the handler detects the destroyed
    // harbor) So mostly we just reset the corresponding id

    if(destroyedHarborId == home_harbor)
        home_harbor = 0;

    // Ist unser Ziel betroffen?
    if(destroyedHarborId != goal_harborId)
    {
        return;
    }

    State oldState = state;

    switch(state)
    {
        default:
            // Just reset goal, but not for expeditions
            if(!IsOnExpedition() && !IsOnExplorationExpedition())
            {
                goal_harborId = 0;
            }
            return; // Skip the rest
        case State::TransportLoading:
        case State::TransportUnloading:
            // Tell wares and figures that they won't reach their goal
            for(auto& figure : figures)
            {
                figure->Abrogate();
                figure->SetGoalTonullptr();
            }
            for(auto& ware : wares)
            {
                // Notify goal only, if it is not the destroyed harbor. It already knows about that ;)
                if(ware->GetGoal() != hb)
                    ware->NotifyGoalAboutLostWare();
                else
                    ware->SetGoal(nullptr);
            }
            break;
        case State::SeaattackLoading:
            // We could also just set the goal harbor id to 0 but this can reuse the event
            AbortSeaAttack();
            break;
        case State::SeaattackUnloading: break;
    }

    // Are we currently getting the wares?
    if(oldState == State::TransportLoading)
    {
        RTTR_Assert(current_ev);
        if(home_harbor)
        {
            // Then save us some time and unload immediately
            // goal is now the start harbor (if it still exists)
            goal_harborId = home_harbor;
            state = State::TransportUnloading;
        } else
        {
            GetEvMgr().RemoveEvent(current_ev);
            FindUnloadGoal(State::TransportDriving);
        }
    } else if(oldState == State::TransportUnloading || oldState == State::SeaattackUnloading)
    {
        // Remove current unload event
        GetEvMgr().RemoveEvent(current_ev);

        if(oldState == State::SeaattackUnloading)
            AbortSeaAttack();
        else
            FindUnloadGoal(State::TransportDriving);
    }
}

/// Fängt an mit idlen und setzt nötigen Sachen auf nullptr
void noShip::StartIdling()
{
    // If those are not empty, then we are lost, not idling!
    RTTR_Assert(figures.empty());
    RTTR_Assert(wares.empty());
    RTTR_Assert(remaining_sea_attackers == 0);
    // Implicit contained wares/figures on expeditions
    RTTR_Assert(!IsOnExplorationExpedition() || state == State::ExplorationexpeditionUnloading);
    RTTR_Assert(!IsOnExpedition() || state == State::ExpeditionUnloading);

    home_harbor = 0;
    goal_harborId = 0;
    state = State::Idle;
}

/// Sagt Bescheid, dass ein Schiffsangreifer nicht mehr mit nach Hause fahren will
void noShip::SeaAttackerWishesNoReturn()
{
    RTTR_Assert(remaining_sea_attackers);
    RTTR_Assert(state == State::SeaattackWaiting);

    --remaining_sea_attackers;
    // Alle Soldaten an Bord
    if(remaining_sea_attackers == 0)
    {
        // Andere Events ggf. erstmal abmelden
        GetEvMgr().RemoveEvent(current_ev);
        if(!figures.empty())
        {
            // Go back home. Note: home_harbor can be 0 if it was destroyed, allow this and let the state handlers
            // handle that case later
            goal_harborId = home_harbor;
            state = State::SeaattackReturnDriving;
            StartDrivingToHarborPlace();
            HandleState_SeaAttackReturn();
        } else
        {
            // Wenn keine Soldaten mehr da sind können wir auch erstmal idlen
            StartIdling();
            world->GetPlayer(ownerId_).GetJobForShip(*this);
        }
    }
}

/// Schiffs-Angreifer sind nach dem Angriff wieder zurückgekehrt
void noShip::AddReturnedAttacker(std::unique_ptr<nofAttacker> attacker)
{
    RTTR_Assert(!helpers::containsPtr(figures, attacker.get()));

    figures.push_back(std::move(attacker));
    // Nun brauchen wir quasi einen Angreifer weniger
    SeaAttackerWishesNoReturn();
}

/// Weist das Schiff an, seine Erkundungs-Expedition fortzusetzen
void noShip::ContinueExplorationExpedition()
{
    // Sind wir schon über unserem Limit, also zu weit gefahren
    if(covered_distance >= MAX_EXPLORATION_EXPEDITION_DISTANCE)
    {
        // Dann steuern wir unseren Heimathafen an!
        goal_harborId = home_harbor;
    } else
    {
        // Find the next harbor spot to explore
        std::vector<unsigned> hps;
        if(goal_harborId)
            hps = world->GetUnexploredHarborPoints(goal_harborId, seaId_, GetPlayerId());

        // No possible spots? -> Go home
        if(hps.empty())
            goal_harborId = home_harbor;
        else
        {
            // Choose one randomly
            goal_harborId = RANDOM_ELEMENT(hps);
        }
    }

    StartDrivingToHarborPlace();
    state = State::ExplorationexpeditionDriving;
    HandleState_ExplorationExpeditionDriving();
}

/// Sagt dem Schiff, dass ein neuer Hafen erbaut wurde
void noShip::NewHarborBuilt(nobHarborBuilding* hb)
{
    if(!lost)
        return;
    // Liegt der Hafen auch am Meer von diesem Schiff?
    if(!world->IsHarborAtSea(hb->GetHarborPosID(), seaId_))
        return;

    // LOG.write(("lost ship has new goal harbor player %i state %i pos %u,%u \n",player,state,x,y);
    home_harbor = goal_harborId = hb->GetHarborPosID();
    lost = false;

    StartDrivingToHarborPlace();

    switch(state)
    {
        case State::ExplorationexpeditionDriving:
        case State::ExpeditionDriving:
        case State::TransportDriving:
        case State::SeaattackReturnDriving: Driven(); break;
        default:
            RTTR_Assert(false); // Das darf eigentlich nicht passieren
            LOG.write("Bug detected: Invalid state in NewHarborBuilt");
            break;
    }
}
