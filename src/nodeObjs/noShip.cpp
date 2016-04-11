// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

///////////////////////////////////////////////////////////////////////////////
// Header

#include "defines.h" // IWYU pragma: keep
#include "noShip.h"
#include "Loader.h"
#include "GameClient.h"
#include "Random.h"
#include "EventManager.h"
#include "SerializedGameData.h"
#include "buildings/nobHarborBuilding.h"
#include "figures/noFigure.h"
#include "Ware.h"
#include "PostMsg.h"
#include "figures/nofAttacker.h"
#include "ai/AIEvents.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "gameData/GameConsts.h"
#include "Log.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

const unsigned int ship_count = 55;

/// Zeit zum Beladen des Schiffes
const unsigned LOADING_TIME = 200;
/// Zeit zum Entladen des Schiffes
const unsigned UNLOADING_TIME = 200;

/// Maximaler Weg, der zurückgelegt werden kann bei einem Erkundungsschiff
const unsigned MAX_EXPLORATION_EXPEDITION_DISTANCE = 100;
/// Zeit (in GF), die das Schiff bei der Erkundungs-Expedition jeweils an einem Punkt ankert
const unsigned EXPLORATION_EXPEDITION_WAITING_TIME = 300;

const std::string ship_names[NAT_COUNT][ship_count] =
{
    /* Nubier */    { "Aica", "Aida", "Ainra", "Alayna", "Alisha", "Alma", "Amila", "Anina", "Armina", "Banu", "Baya", "Bea", "Bia", "Bisa", "Cheche", "Dafina", "Daria", "Dina", "Do", "Dofi", "Efia", "Erin", "Esi", "Esra", "Fahari", "Faraya", "Fujo", "Ghiday", "Habiaba", "Hajunza", "Ina", "Layla", "Lenia", "Lillian", "Malika", "Mona", "Naja", "Neriman", "Nyela", "Olufunmilayo", "Panyin", "Rayyan", "Rhiannon", "Safiya", "Sahra", "Selda", "Senna", "Shaira", "Shakira", "Sharina", "Sinah", "Suada", "Sulamith", "Tiada", "Yelda" },
    /* Japaner */   { "Ai", "Aiko", "Aimi", "Akemi", "Amaya", "Aoi", "Ayaka", "Ayano", "Beniko", "Chiyo", "Chiyoko", "Emi", "Fumiko", "Haruka", "Hiroko", "Hotaru", "Kaori", "Kasumi", "Kazuko", "Kazumi", "Keiko", "Kiriko", "Kumiko", "Mai", "Mayumi", "Megumi", "Midori", "Misaki", "Miu", "Moe", "Nanami", "Naoko", "Naomi", "Natsuki", "Noriko", "Reika", "Sachiko", "Sadako", "Sakura", "Satsuki", "Sayuri", "Setsuko", "Shigeko", "Teiko", "Tomomi", "Umeko", "Yoko", "Yoshiko", "Youko", "Yukiko", "Yumi", "Yumiko", "Yuna", "Yuuka", "Yuzuki" },
    /* Römer */        { "Antia", "Ateia", "Aurelia", "Camilia", "Claudia", "Duccia", "Epidia", "Equitia", "Fabia", "Galeria", "Helvetia", "Iunia", "Iusta", "Iuventia", "Lafrenia", "Livia", "Longinia", "Maelia", "Maxima", "Nigilia", "Nipia", "Norbana", "Novia", "Orania", "Otacilia", "Petronia", "Pinaria", "Piscia", "Pisentia", "Placidia", "Quintia", "Quirinia", "Rusonia", "Rutilia", "Sabucia", "Sallustia", "Salonia", "Salvia", "Scribonia", "Secundia", "Secundinia", "Tadia", "Talmudia", "Tanicia", "Tertinia", "Tita", "Ulpia", "Umbrenia", "Valeria", "Varia", "Vassenia", "Vatinia", "Vedia", "Velia", "Verania" },
    /* Wikinger */      { "Adelberga", "Adelgund", "Adelheid", "Adelinde", "Alsuna", "Alwina", "Amelinde", "Astrid", "Baltrun", "Bernhild", "Bothilde", "Dagny", "Dankrun", "Eldrid", "Erlgard", "Fehild ", "Ferun", "Frauke ", "Freya", "Gerda ", "Gesa", "Gismara", "Hella", "Henrike ", "Hilke", "Ida", "Irma", "Irmlinde", "Isantrud ", "Kunheide", "Kunigunde", "Lioba", "Lykke", "Marada", "Margard ", "Merlinde", "Minnegard", "Nanna", "Norwiga", "Oda", "Odarike", "Osrun ", "Raginhild ", "Raskild ", "Rinelda", "Runa", "Runhild ", "Salgard", "Sarhild", "Tanka", "Tyra", "Ulla", "Uta", "Walda", "Wiebke" },
    /* Babylonier */        { "Anu", "Enlil", "Ea", "Sin", "Samas", "Istar", "Marduk", "Nabu", "Ninurta", "Nusku", "Nergal", "Adad", "Tammuz", "Asalluchi", "Tutu", "Nabu-mukin-zeri", "Tiglath-Pileser", "Shalmaneser", "Marduk-apla-iddina", "Sharrukin", "Sin-ahhe-eriba", "Bel-ibni", "Ashur-nadin-shumi", "Kandalanu", "Sin-shumu-lishir", "Sinsharishkun", "Ninurta-apla", "Agum", "Burnaburiash", "Kashtiliash", "Ulamburiash", "Karaindash", "Kurigalzu", "Shuzigash", "Gandas", "Abi-Rattas", "Hurbazum", "Gulkishar", "Peshgaldaramesh", "Ayadaragalama", "Akurduana", "Melamkurkurra", "Hammurabi", "Samsu-Ditana", "Bishapur", "Ekbatana", "Gundischapur", "Ktesiphon", "Bactra", "Pasargadae", "Persepolis", "Susa", "Rayy", "Pa-Rasit", "Spi-Keone" }
};

//{"FloSoftius", "Demophobius", "Olivianus", "Spikeonius", "Nastius"};

/// Positionen der Flaggen am Schiff für die 6 unterschiedlichen Richtungen jeweils#
const Point<int> SHIPS_FLAG_POS[12] =
{
    // Und wenn das Schiff steht und Segel nicht gehisst hat
    Point<int>(-3, -77),
    Point<int>(-6, -71),
    Point<int>(-3, -71),
    Point<int>(-1, -71),
    Point<int>(5, -63),
    Point<int>(-1, -70),

    // Und wenn es fährt
    Point<int>(3, -70),
    Point<int>(0, -64),
    Point<int>(3, -64),
    Point<int>(-1, -70),
    Point<int>(5, -63),
    Point<int>(5, -63)
};

noShip::noShip(const MapPoint pos, const unsigned char player)
    : noMovable(NOP_SHIP, pos),
      player(player), state(STATE_IDLE), seaId_(0), goal_harbor_id(0), goal_dir(0),
      name(ship_names[gwg->GetPlayer(player).nation][RANDOM.Rand(__FILE__, __LINE__, GetObjId(), ship_count)]),
      curRouteIdx(0), lost(false), remaining_sea_attackers(0), home_harbor(0), covered_distance(0)
{
    // Meer ermitteln, auf dem dieses Schiff fährt
    for(unsigned i = 0; i < 6; ++i)
    {
        unsigned short sea_id = gwg->GetNeighbourNode(pos, i).sea_id;
        if(sea_id)
            this->seaId_ = sea_id;
    }

    // Auf irgendeinem Meer müssen wir ja sein
    RTTR_Assert(seaId_ > 0);
}

void noShip::Serialize(SerializedGameData& sgd) const
{
    Serialize_noMovable(sgd);

    sgd.PushUnsignedChar(player);
    sgd.PushUnsignedChar(static_cast<unsigned char>(state));
    sgd.PushUnsignedShort(seaId_);
    sgd.PushUnsignedInt(goal_harbor_id);
    sgd.PushUnsignedChar(goal_dir);
    sgd.PushString(name);
    sgd.PushUnsignedInt(curRouteIdx);
    sgd.PushUnsignedInt(route_.size());
    sgd.PushBool(lost);
    sgd.PushUnsignedInt(remaining_sea_attackers);
    sgd.PushUnsignedInt(home_harbor);
    sgd.PushUnsignedInt(covered_distance);
    for(unsigned i = 0; i < route_.size(); ++i)
        sgd.PushUnsignedChar(route_[i]);
    sgd.PushObjectContainer(figures, false);
    sgd.PushObjectContainer(wares, true);
}

noShip::noShip(SerializedGameData& sgd, const unsigned obj_id) :
    noMovable(sgd, obj_id),
    player(sgd.PopUnsignedChar()),
    state(State(sgd.PopUnsignedChar())),
    seaId_(sgd.PopUnsignedShort()),
    goal_harbor_id(sgd.PopUnsignedInt()),
    goal_dir(sgd.PopUnsignedChar()),
    name(sgd.PopString()),
    curRouteIdx(sgd.PopUnsignedInt()),
    route_(sgd.PopUnsignedInt()),
    lost(sgd.PopBool()),
    remaining_sea_attackers(sgd.PopUnsignedInt()),
    home_harbor(sgd.PopUnsignedInt()),
    covered_distance(sgd.PopUnsignedInt())
{
    for(unsigned i = 0; i < route_.size(); ++i)
        route_[i] = sgd.PopUnsignedChar();
    sgd.PopObjectContainer(figures, GOT_UNKNOWN);
    sgd.PopObjectContainer(wares, GOT_WARE);
}

void noShip::Destroy()
{
    // Schiff wieder abmelden
    gwg->GetPlayer(player).RemoveShip(this);
    for(std::list<noFigure*>::iterator it = figures.begin(); it != figures.end(); ++it)
        RTTR_Assert(!*it);
    for(std::list<Ware*>::iterator it = wares.begin(); it != wares.end(); ++it)
        RTTR_Assert(!*it);
}

/// Zeichnet das Schiff stehend mit oder ohne Waren
void noShip::DrawFixed(const int x, const int y, const bool draw_wares)
{
    LOADER.GetImageN("boot_z",  ((GetCurMoveDir() + 3) % 6) * 2 + 1)->Draw(x, y, 0, 0, 0, 0, 0, 0, COLOR_SHADOW);
    LOADER.GetImageN("boot_z",  ((GetCurMoveDir() + 3) % 6) * 2)->Draw(x, y);

    if(draw_wares)
        /// Waren zeichnen
        LOADER.GetImageN("boot_z",  30 + ((GetCurMoveDir() + 3) % 6))->Draw(x, y);
}

void noShip::Draw(int x, int y)
{
    unsigned flag_drawing_type = 1;

    // Sind wir verloren? Dann immer stehend zeichnen
    if(lost)
    {
        DrawFixed(x, y, true);
        return;
    }

    switch(state)
    {
        default:
            break;
        case STATE_IDLE:
        case STATE_SEAATTACK_WAITING:
        {
            DrawFixed(x, y, false);
            flag_drawing_type = 0;
        } break;

        case STATE_GOTOHARBOR:
        {
            DrawDriving(x, y);
        } break;
        case STATE_EXPEDITION_LOADING:
        case STATE_EXPEDITION_UNLOADING:
        case STATE_TRANSPORT_LOADING:
        case STATE_TRANSPORT_UNLOADING:
        case STATE_SEAATTACK_LOADING:
        case STATE_SEAATTACK_UNLOADING:
        case STATE_EXPLORATIONEXPEDITION_LOADING:
        case STATE_EXPLORATIONEXPEDITION_UNLOADING:
        {
            DrawFixed(x, y, false);
        } break;
        case STATE_EXPLORATIONEXPEDITION_WAITING:
        case STATE_EXPEDITION_WAITING:

        {
            DrawFixed(x, y, true);
        } break;
        case STATE_EXPEDITION_DRIVING:
        case STATE_TRANSPORT_DRIVING:
        case STATE_SEAATTACK_DRIVINGTODESTINATION:
        case STATE_EXPLORATIONEXPEDITION_DRIVING:

        {
            DrawDrivingWithWares(x, y);
        } break;
        case STATE_SEAATTACK_RETURN_DRIVING:
        {
            if(!figures.empty() || !wares.empty())
                DrawDrivingWithWares(x, y);
            else
                DrawDriving(x, y);
        } break;
    }

    LOADER.GetPlayerImage("boot_z", 40 + GAMECLIENT.GetGlobalAnimation(6, 1, 1, GetObjId()))->
    Draw(x + SHIPS_FLAG_POS[GetCurMoveDir() + flag_drawing_type * 6].x, y + SHIPS_FLAG_POS[GetCurMoveDir() + flag_drawing_type * 6].y, 0, 0, 0, 0, 0, 0, COLOR_WHITE, gwg->GetPlayer(player).color);
    // Second, white flag, only when on expedition, always swinging in the opposite direction
    if(state >= STATE_EXPEDITION_LOADING && state <= STATE_EXPEDITION_DRIVING)
        LOADER.GetPlayerImage("boot_z", 40 + GAMECLIENT.GetGlobalAnimation(6, 1, 1, GetObjId() + 4))->
        Draw(x + SHIPS_FLAG_POS[GetCurMoveDir() + flag_drawing_type * 6].x, y + 4 + SHIPS_FLAG_POS[GetCurMoveDir() + flag_drawing_type * 6].y, 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLOR_WHITE);

}

/// Zeichnet normales Fahren auf dem Meer ohne irgendwelche Güter
void noShip::DrawDriving(int& x, int& y)
{
    // Interpolieren zwischen beiden Knotenpunkten
    Point<int> offset = CalcWalkingRelative();
    x += offset.x;
    y += offset.y;

    LOADER.GetImageN("boot_z", 13 + ((GetCurMoveDir() + 3) % 6) * 2)->Draw(x, y, 0, 0, 0, 0, 0, 0, COLOR_SHADOW);
    LOADER.GetImageN("boot_z", 12 + ((GetCurMoveDir() + 3) % 6) * 2)->Draw(x, y);
}

/// Zeichnet normales Fahren auf dem Meer mit Gütern
void noShip::DrawDrivingWithWares(int& x, int& y)
{
    // Interpolieren zwischen beiden Knotenpunkten
    Point<int> offset = CalcWalkingRelative();
    x += offset.x;
    y += offset.y;

    LOADER.GetImageN("boot_z", 13 + ((GetCurMoveDir() + 3) % 6) * 2)->Draw(x, y, 0, 0, 0, 0, 0, 0, COLOR_SHADOW);
    LOADER.GetImageN("boot_z", 12 + ((GetCurMoveDir() + 3) % 6) * 2)->Draw(x, y);
    /// Waren zeichnen
    LOADER.GetImageN("boot_z",  30 + ((GetCurMoveDir() + 3) % 6))->Draw(x, y);
}

void noShip::HandleEvent(const unsigned int id)
{
    RTTR_Assert(current_ev);
    RTTR_Assert(current_ev->id == id);
    current_ev = NULL;

    if(id == 0)
    {
        // Move event
        // neue Position einnehmen
        Walk();
        // entscheiden, was als nächstes zu tun ist
        Driven();
    }else
    {
        switch(state)
        {
        default: 
            RTTR_Assert(false);
            LOG.lprintf("Bug detected: Invalid state in ship event");
            break;
        case STATE_EXPEDITION_LOADING:
            // Schiff ist nun bereit und Expedition kann beginnen
            state = STATE_EXPEDITION_WAITING;

            // Spieler benachrichtigen
            if(GAMECLIENT.GetPlayerID() == this->player)
                GAMECLIENT.SendPostMessage(new ShipPostMsg(_("A ship is ready for an expedition."), PMC_GENERAL, GAMECLIENT.GetPlayer(player).nation, pos));

            // KI Event senden
            GAMECLIENT.SendAIEvent(new AIEvent::Location(AIEvent::ExpeditionWaiting, pos), player);
            break;
        case STATE_EXPLORATIONEXPEDITION_LOADING:
        case STATE_EXPLORATIONEXPEDITION_WAITING:
            // Schiff ist nun bereit und Expedition kann beginnen
            ContinueExplorationExpedition();
            break;
        case STATE_EXPEDITION_UNLOADING:
        {
            // Hafen herausfinden
            noBase* hb = goal_harbor_id ? gwg->GetNO(gwg->GetHarborPoint(goal_harbor_id)) : NULL;

            if(hb && hb->GetGOT() == GOT_NOB_HARBORBUILDING)
            {
                Inventory goods;
                unsigned char nation = gwg->GetPlayer(player).nation;
                goods.goods[GD_BOARDS] = BUILDING_COSTS[nation][BLD_HARBORBUILDING].boards;
                goods.goods[GD_STONES] = BUILDING_COSTS[nation][BLD_HARBORBUILDING].stones;
                goods.people[JOB_BUILDER] = 1;
                static_cast<nobBaseWarehouse*>(hb)->AddGoods(goods);
                // Wieder idlen und ggf. neuen Job suchen
                StartIdling();
                gwg->GetPlayer(player).GetJobForShip(this);
            }
            else
            {
                // target harbor for unloading doesnt exist anymore -> set state to driving and handle the new state
                state = STATE_EXPEDITION_DRIVING;
                HandleState_ExpeditionDriving();
            }
            break;
        }
        case STATE_EXPLORATIONEXPEDITION_UNLOADING:
        {
            // Hafen herausfinden
            noBase* hb = goal_harbor_id ? gwg->GetNO(gwg->GetHarborPoint(goal_harbor_id)): NULL;

            unsigned old_visual_range = GetVisualRange();

            if(hb && hb->GetGOT() == GOT_NOB_HARBORBUILDING)
            {
                // Späher wieder entladen
                Inventory goods;
                goods.people[JOB_SCOUT] = GAMECLIENT.GetGGS().GetNumScoutsExedition();
                static_cast<nobBaseWarehouse*>(hb)->AddGoods(goods);
                // Wieder idlen und ggf. neuen Job suchen
                StartIdling();
                gwg->GetPlayer(player).GetJobForShip(this);
            }
            else
            {
                // target harbor for unloading doesnt exist anymore -> set state to driving and handle the new state
                state = STATE_EXPLORATIONEXPEDITION_DRIVING;
                HandleState_ExplorationExpeditionDriving();
            }

            // Sichtbarkeiten neu berechnen
            gwg->RecalcVisibilitiesAroundPoint(pos, old_visual_range, player, NULL);

            break;
        }
        case STATE_TRANSPORT_LOADING:
            StartTransport();
            break;
        case STATE_TRANSPORT_UNLOADING:
        case STATE_SEAATTACK_UNLOADING:
        {
            // Hafen herausfinden
            RTTR_Assert(state == STATE_SEAATTACK_UNLOADING || remaining_sea_attackers == 0);
            noBase* hb = goal_harbor_id ? gwg->GetNO(gwg->GetHarborPoint(goal_harbor_id)): NULL;
            if(hb && hb->GetGOT() == GOT_NOB_HARBORBUILDING)
            {
                static_cast<nobHarborBuilding*>(hb)->ReceiveGoodsFromShip(figures, wares);
                figures.clear();
                wares.clear();

                state = STATE_TRANSPORT_UNLOADING;
                // Hafen bescheid sagen, dass er das Schiff nun nutzen kann
                static_cast<nobHarborBuilding*>(hb)->ShipArrived(this);

                // Hafen hat keinen Job für uns?
                if (state == STATE_TRANSPORT_UNLOADING)
                {
                    // Wieder idlen und ggf. neuen Job suchen
                    StartIdling();
                    gwg->GetPlayer(player).GetJobForShip(this);
                }
            }
            else
            {
                // target harbor for unloading doesnt exist anymore -> set state to driving and handle the new state
                if(state == STATE_TRANSPORT_UNLOADING)
                    FindUnloadGoal(STATE_TRANSPORT_DRIVING);
                else
                    FindUnloadGoal(STATE_SEAATTACK_RETURN_DRIVING);
            }
            break;
        }
        case STATE_SEAATTACK_LOADING:
            StartSeaAttack();
            break;
        case STATE_SEAATTACK_WAITING:
        {
            // Nächsten Soldaten nach draußen beordern
            if(figures.empty())
                break;

            nofAttacker* attacker = static_cast<nofAttacker*>(figures.front());
            // Evtl. ist ein Angreifer schon fertig und wieder an Board gegangen
            // der darf dann natürlich nicht noch einmal raus, sonst kann die schöne Reise
            // böse enden
            if(attacker->IsSeaAttackCompleted())
                break;

            figures.pop_front();
            gwg->AddFigure(attacker, pos);

            current_ev = em->AddEvent(this, 30, 1);
            attacker->StartAttackOnOtherIsland(pos, GetObjId());
            break;
        }
        }
    }
}

void noShip::StartDriving(const unsigned char dir)
{
    const unsigned SHIP_SPEEDS[] = {35, 25, 20, 10, 5};

    StartMoving(dir, SHIP_SPEEDS[GAMECLIENT.GetGGS().getSelection(AddonId::SHIP_SPEED)]);
}

void noShip::Driven()
{
    MapPoint enemy_territory_discovered(MapPoint::Invalid());
    gwg->RecalcMovingVisibilities(pos, player, GetVisualRange(), GetCurMoveDir(), &enemy_territory_discovered);

    // Feindliches Territorium entdeckt?
    if(enemy_territory_discovered.isValid())
    {
        // Send message if necessary
        if(gwg->GetPlayer(player).ShipDiscoveredHostileTerritory(enemy_territory_discovered) && player == GAMECLIENT.GetPlayerID())
            GAMECLIENT.SendPostMessage(new PostMsgWithLocation(_("A ship disovered an enemy territory"), PMC_MILITARY, enemy_territory_discovered));
    }

    switch(state)
    {
        case STATE_GOTOHARBOR: HandleState_GoToHarbor(); break;
        case STATE_EXPEDITION_DRIVING: HandleState_ExpeditionDriving(); break;
        case STATE_EXPLORATIONEXPEDITION_DRIVING: HandleState_ExplorationExpeditionDriving(); break;
        case STATE_TRANSPORT_DRIVING: HandleState_TransportDriving(); break;
        case STATE_SEAATTACK_DRIVINGTODESTINATION: HandleState_SeaAttackDriving(); break;
        case STATE_SEAATTACK_RETURN_DRIVING: HandleState_SeaAttackReturn(); break;
        default:
            RTTR_Assert(false);
            break;
    }
}

/// Gibt Sichtradius dieses Schiffes zurück
unsigned noShip::GetVisualRange() const
{
    // Erkundungsschiffe haben einen größeren Sichtbereich
    if(state >= STATE_EXPLORATIONEXPEDITION_LOADING && state <= STATE_EXPLORATIONEXPEDITION_DRIVING)
        return VISUALRANGE_EXPLORATION_SHIP;
    else
        return VISUALRANGE_SHIP;
}

/// Fährt zum Hafen, um dort eine Mission (Expedition) zu erledigen
void noShip::GoToHarbor(nobHarborBuilding* hb, const std::vector<unsigned char>& route)
{
    RTTR_Assert(state == STATE_IDLE); // otherwise we might carry wares etc
    RTTR_Assert(figures.empty());
    RTTR_Assert(wares.empty());
    RTTR_Assert(remaining_sea_attackers == 0);

    state = STATE_GOTOHARBOR;

    goal_harbor_id = gwg->GetNode(hb->GetPos()).harbor_id;
    RTTR_Assert(goal_harbor_id);

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
    state = STATE_EXPEDITION_LOADING;
    current_ev = em->AddEvent(this, LOADING_TIME, 1);
    RTTR_Assert(homeHarborId);
    RTTR_Assert(pos == gwg->GetCoastalPoint(homeHarborId, seaId_));
    home_harbor = homeHarborId;
    goal_harbor_id = homeHarborId; // This is current goal (commands are relative to current goal)
}

/// Startet eine Erkundungs-Expedition
void noShip::StartExplorationExpedition(unsigned homeHarborId)
{
    /// Schiff wird "beladen", also kurze Zeit am Hafen stehen, bevor wir bereit sind
    state = STATE_EXPLORATIONEXPEDITION_LOADING;
    current_ev = em->AddEvent(this, LOADING_TIME, 1);
    covered_distance = 0;
    RTTR_Assert(homeHarborId);
    RTTR_Assert(pos == gwg->GetCoastalPoint(homeHarborId, seaId_));
    home_harbor = homeHarborId;
    goal_harbor_id = homeHarborId; // This is current goal (commands are relative to current goal)
    // Sichtbarkeiten neu berechnen
    gwg->SetVisibilitiesAroundPoint(pos, GetVisualRange(), player);
}

/// Fährt weiter zu einem Hafen
noShip::Result noShip::DriveToHarbour()
{
    if(!goal_harbor_id)
        return HARBOR_DOESNT_EXIST;

    MapPoint goal(gwg->GetHarborPoint(goal_harbor_id));
    RTTR_Assert(goal.isValid());

    // Existiert der Hafen überhaupt noch?
    if(gwg->GetGOT(goal) != GOT_NOB_HARBORBUILDING)
        return HARBOR_DOESNT_EXIST;

    return DriveToHarbourPlace();
}

/// Fährt weiter zu Hafenbauplatz
noShip::Result noShip::DriveToHarbourPlace()
{
    if(goal_harbor_id == 0)
        return HARBOR_DOESNT_EXIST;

    // Sind wir schon da?
    if(curRouteIdx == route_.size())
        return GOAL_REACHED;

    // Punkt, zu dem uns der Hafen hinführen will (Küstenpunkt)
    MapPoint coastalPos = gwg->GetCoastalPoint(goal_harbor_id, seaId_);

    MapPoint goalRoutePos;

    // Route überprüfen
    if(!gwg->CheckShipRoute(pos, route_, curRouteIdx, &goalRoutePos))
    {
        // Route kann nicht mehr passiert werden --> neue Route suchen
        if(!gwg->FindShipPath(pos, coastalPos, &route_, NULL))
        {
            // Wieder keine gefunden -> raus
            return NO_ROUTE_FOUND;
        }

        // Wir fangen bei der neuen Route wieder von vorne an
        curRouteIdx = 0;
    }

    // Kommen wir auch mit unser bisherigen Route an dem ursprünglich anvisierten Hafenplatz an?
    if(coastalPos != goalRoutePos)
    {
        // Nein, d.h. wenn wir schon nah dran sind, müssen wir die Route neu berechnen
        RTTR_Assert(route_.size() >= curRouteIdx);
        if(route_.size() - curRouteIdx < 10)
        {
            if(!gwg->FindShipPath(pos, coastalPos, &route_, NULL))
                // Keiner gefunden -> raus
                return NO_ROUTE_FOUND;

            curRouteIdx = 0;
        }
    }

    StartDriving(route_[curRouteIdx++]);
    return DRIVING;
}


unsigned noShip::GetCurrentHarbor() const
{
    RTTR_Assert(state == STATE_EXPEDITION_WAITING);
    return goal_harbor_id;
}


/// Weist das Schiff an, in einer bestimmten Richtung die Expedition fortzusetzen
void noShip::ContinueExpedition(const unsigned char dir)
{
    if(state != STATE_EXPEDITION_WAITING)
        return;

    // Nächsten Hafenpunkt in dieser Richtung suchen
    unsigned new_goal = gwg->GetNextFreeHarborPoint(pos, goal_harbor_id, dir, player);

    // Auch ein Ziel gefunden?
    if(!new_goal)
        return;

    MapPoint coastalPos = gwg->GetCoastalPoint(new_goal, seaId_);

    // Versuchen, Weg zu finden
    if(!gwg->FindShipPath(pos, coastalPos, &route_, NULL))
        return;

    // Dann fahren wir da mal hin
    curRouteIdx = 0;
    goal_harbor_id = new_goal;
    state = STATE_EXPEDITION_DRIVING;

    StartDriving(route_[curRouteIdx++]);
}

/// Weist das Schiff an, eine Expedition abzubrechen (nur wenn es steht) und zum
/// Hafen zurückzukehren
void noShip::CancelExpedition()
{
    // Protect against double execution
    if(state != STATE_EXPEDITION_WAITING)
        return;

    // We are waiting. There should be no event!
    RTTR_Assert(!current_ev);

    // Zum Heimathafen zurückkehren
    // Oder sind wir schon dort?
    if(goal_harbor_id == home_harbor)
    {
        route_.clear();
        curRouteIdx = 0;
        state = STATE_EXPEDITION_DRIVING; //just in case the home harbor was destroyed
        HandleState_ExpeditionDriving();
    }
    else
    {
        state = STATE_EXPEDITION_DRIVING;
        goal_harbor_id = home_harbor;
        StartDrivingToHarborPlace();
        HandleState_ExpeditionDriving();
    }
}

/// Weist das Schiff an, an der aktuellen Position einen Hafen zu gründen
void noShip::FoundColony()
{
    if(state != STATE_EXPEDITION_WAITING)
        return;

    // Kolonie gründen
    if(gwg->FoundColony(goal_harbor_id, player, seaId_))
    {
        // Dann idlen wir wieder
        StartIdling();
        // Neue Arbeit suchen
        gwg->GetPlayer(player).GetJobForShip(this);
    }
    else //colony founding FAILED - notify ai
        GAMECLIENT.SendAIEvent(new AIEvent::Location(AIEvent::ExpeditionWaiting, pos), player);
}

void noShip::HandleState_GoToHarbor()
{
    // Hafen schon zerstört?
    if(goal_harbor_id == 0)
    {
        StartIdling();
        return;
    }

    Result res = DriveToHarbour();
    switch(res)
    {
    case DRIVING:
        return; // Continue
    case GOAL_REACHED:
        {
            MapPoint goal(gwg->GetHarborPoint(goal_harbor_id));
            RTTR_Assert(goal.isValid());
            // Go idle here (if harbor does not need it)
            StartIdling();
            // Hafen Bescheid sagen, dass wir da sind (falls er überhaupt noch existiert)
            noBase* hb = goal.isValid() ? gwg->GetNO(goal) : NULL;
            if(hb && hb->GetGOT() == GOT_NOB_HARBORBUILDING)
                static_cast<nobHarborBuilding*>(hb)->ShipArrived(this);
        } break;
    case NO_ROUTE_FOUND:
        {
            MapPoint goal(gwg->GetHarborPoint(goal_harbor_id));
            RTTR_Assert(goal.isValid());
            // Dem Hafen Bescheid sagen
            gwg->GetSpecObj<nobHarborBuilding>(goal)->ShipLost(this);
            StartIdling();
        } break;
    case HARBOR_DOESNT_EXIST:
        StartIdling();
        break;
    }
}

void noShip::HandleState_ExpeditionDriving()
{
    Result res;
    // Zum Heimathafen fahren?
    if(home_harbor == goal_harbor_id)
        res = DriveToHarbour();
    else
        res = DriveToHarbourPlace();

    switch(res)
    {
        case DRIVING: return;
        case GOAL_REACHED:
        {
            // Haben wir unsere Expedition beendet?
            if(home_harbor == goal_harbor_id)
            {
                // Sachen wieder in den Hafen verladen
                state = STATE_EXPEDITION_UNLOADING;
                current_ev = em->AddEvent(this, UNLOADING_TIME, 1);
            }
            else
            {
                // Warten auf weitere Anweisungen
                state = STATE_EXPEDITION_WAITING;

                // Spieler benachrichtigen
                if(GAMECLIENT.GetPlayerID() == this->player)
                    GAMECLIENT.SendPostMessage(new ShipPostMsg(_("A ship has reached the destination of its expedition."), PMC_GENERAL, GAMECLIENT.GetPlayer(player).nation, pos));

                // KI Event senden
                GAMECLIENT.SendAIEvent(new AIEvent::Location(AIEvent::ExpeditionWaiting, pos), player);
            }
        } break;
        case NO_ROUTE_FOUND:
        case HARBOR_DOESNT_EXIST: //should only happen when an expedition is cancelled and the home harbor no longer exists
        {
            if(home_harbor != goal_harbor_id && home_harbor != 0)
            {
                // Try to go back
                goal_harbor_id = home_harbor;
                HandleState_ExpeditionDriving();
            }else
                FindUnloadGoal(STATE_EXPEDITION_DRIVING); // Unload anywhere!
        } break;
    }
}

void noShip::HandleState_ExplorationExpeditionDriving()
{
    Result res;
    // Zum Heimathafen fahren?
    if(home_harbor == goal_harbor_id && covered_distance >= MAX_EXPLORATION_EXPEDITION_DISTANCE)
        res = DriveToHarbour();
    else
        res = DriveToHarbourPlace();

    switch(res)
    {
        case DRIVING: return;
        case GOAL_REACHED:
        {
            // Haben wir unsere Expedition beendet?
            if(home_harbor == goal_harbor_id && covered_distance >= MAX_EXPLORATION_EXPEDITION_DISTANCE)
            {
                // Dann sind wir fertig -> wieder entladen
                state = STATE_EXPLORATIONEXPEDITION_UNLOADING;
                current_ev = em->AddEvent(this, UNLOADING_TIME, 1);
            }
            else
            {
                // Strecke, die wir gefahren sind, draufaddieren
                covered_distance += route_.size();
                // Erstmal kurz ausruhen an diesem Punkt und das Rohr ausfahren, um ein bisschen
                // auf der Insel zu gucken
                state = STATE_EXPLORATIONEXPEDITION_WAITING;
                current_ev = em->AddEvent(this, EXPLORATION_EXPEDITION_WAITING_TIME, 1);
            }

        } break;
        case NO_ROUTE_FOUND:
        case HARBOR_DOESNT_EXIST:
            gwg->RecalcVisibilitiesAroundPoint(pos, GetVisualRange(), player, NULL);
            StartIdling();
            break;
    }
}

void noShip::HandleState_TransportDriving()
{
    Result res = DriveToHarbour();
    switch(res)
    {
        case DRIVING: return;
        case GOAL_REACHED:
        {
            // Waren abladen, dafür wieder kurze Zeit hier ankern
            state = STATE_TRANSPORT_UNLOADING;
            current_ev = em->AddEvent(this, UNLOADING_TIME, 1);
        } break;
        case NO_ROUTE_FOUND:
        case HARBOR_DOESNT_EXIST:
        {
            RTTR_Assert(!remaining_sea_attackers);
            // Kein Hafen mehr?
            // Dann müssen alle Leute ihren Heimatgebäuden Bescheid geben, dass sie
            // nun nicht mehr kommen
            // Das Schiff muss einen Notlandeplatz ansteuern
            //LOG.lprintf("transport goal harbor doesnt exist player %i state %i pos %u,%u \n",player,state,x,y);
            for(std::list<noFigure*>::iterator it = figures.begin(); it != figures.end(); ++it)
            {
                (*it)->Abrogate();
                (*it)->SetGoalToNULL();
            }

            for(std::list<Ware*>::iterator it = wares.begin(); it != wares.end(); ++it)
            {
                (*it)->NotifyGoalAboutLostWare();
            }

            FindUnloadGoal(STATE_TRANSPORT_DRIVING);
        } break;
    }
}

void noShip::HandleState_SeaAttackDriving()
{
    Result res = DriveToHarbourPlace();
    switch(res)
    {
    case DRIVING:
        return; // OK
    case GOAL_REACHED:
        // Ziel erreicht, dann stellen wir das Schiff hier hin und die Soldaten laufen nacheinander raus zum Ziel
        state = STATE_SEAATTACK_WAITING;
        current_ev = em->AddEvent(this, 15, 1);
        remaining_sea_attackers = figures.size();
        break;
    case NO_ROUTE_FOUND:
    case HARBOR_DOESNT_EXIST:
        RTTR_Assert(goal_harbor_id != home_harbor || home_harbor == 0);
        AbortSeaAttack();
        break;
    }
}

void noShip::HandleState_SeaAttackReturn()
{
    Result res = DriveToHarbour();
    switch(res)
    {
    case DRIVING: return;
    case GOAL_REACHED:
        // Entladen
        state = STATE_SEAATTACK_UNLOADING;
        this->current_ev = em->AddEvent(this, UNLOADING_TIME, 1);
        break;
    case HARBOR_DOESNT_EXIST:
    case NO_ROUTE_FOUND:
        AbortSeaAttack();
        break;
    }
}

/// Gibt zurück, ob das Schiff jetzt in der Lage wäre, eine Kolonie zu gründen
bool noShip::IsAbleToFoundColony() const
{
    // Warten wir gerade?
    if(state == STATE_EXPEDITION_WAITING)
    {
        // We must always have a goal harbor
        RTTR_Assert(goal_harbor_id);
        // Ist der Punkt, an dem wir gerade ankern, noch frei?
        if(gwg->IsHarborPointFree(goal_harbor_id, player, seaId_))
            return true;
    }

    return false;
}

/// Gibt zurück, ob das Schiff einen bestimmten Hafen ansteuert
bool noShip::IsGoingToHarbor(nobHarborBuilding* hb) const
{
    if(goal_harbor_id != hb->GetHarborPosID())
        return false;
    // Explicit switch to check all states
    switch (state)
    {
    case noShip::STATE_IDLE:
    case noShip::STATE_EXPEDITION_LOADING:
    case noShip::STATE_EXPEDITION_UNLOADING:
    case noShip::STATE_EXPEDITION_WAITING:
    case noShip::STATE_EXPEDITION_DRIVING:
    case noShip::STATE_EXPLORATIONEXPEDITION_LOADING:
    case noShip::STATE_EXPLORATIONEXPEDITION_UNLOADING:
    case noShip::STATE_EXPLORATIONEXPEDITION_WAITING:
    case noShip::STATE_EXPLORATIONEXPEDITION_DRIVING:
    case noShip::STATE_SEAATTACK_LOADING:
    case noShip::STATE_SEAATTACK_DRIVINGTODESTINATION:
    case noShip::STATE_SEAATTACK_WAITING:
        return false;
    case noShip::STATE_GOTOHARBOR:
    case noShip::STATE_TRANSPORT_DRIVING:   // Driving to this harbor
    case noShip::STATE_TRANSPORT_LOADING:   // Loading at home harbor and going to goal
    case noShip::STATE_TRANSPORT_UNLOADING: // Unloading at this harbor
    case noShip::STATE_SEAATTACK_UNLOADING: // Unloading attackers at this harbor
    case noShip::STATE_SEAATTACK_RETURN_DRIVING:    // Returning attackers to this harbor
        return true;
}
    RTTR_Assert(false);
    return false;
}

/// Belädt das Schiff mit Waren und Figuren, um eine Transportfahrt zu starten
void noShip::PrepareTransport(unsigned homeHarborId, MapPoint goal, const std::list<noFigure*>& figures, const std::list<Ware*>& wares)
{
    RTTR_Assert(homeHarborId);
    RTTR_Assert(pos == gwg->GetCoastalPoint(homeHarborId, seaId_));
    this->home_harbor = homeHarborId;
    // ID von Zielhafen herausfinden
    noBase* nb = gwg->GetNO(goal);
    RTTR_Assert(nb->GetGOT() == GOT_NOB_HARBORBUILDING);
    this->goal_harbor_id = static_cast<nobHarborBuilding*>(nb)->GetHarborPosID();

    this->figures = figures;
    this->wares = wares;

    state = STATE_TRANSPORT_LOADING;
    current_ev = em->AddEvent(this, LOADING_TIME, 1);
}

/// Belädt das Schiff mit Schiffs-Angreifern
void noShip::PrepareSeaAttack(unsigned homeHarborId, MapPoint goal, const std::list<noFigure*>& figures)
{
    // Heimathafen merken
    RTTR_Assert(homeHarborId);
    RTTR_Assert(pos == gwg->GetCoastalPoint(homeHarborId, seaId_));
    this->home_harbor = homeHarborId;
    this->goal_harbor_id = gwg->GetHarborPointID(goal);
    RTTR_Assert(goal_harbor_id);
    this->figures = figures;
    for(std::list<noFigure*>::iterator it = this->figures.begin(); it != this->figures.end(); ++it)
    {
        static_cast<nofAttacker*>(*it)->StartShipJourney();
        static_cast<nofAttacker*>(*it)->SeaAttackStarted();
    }
    state = STATE_SEAATTACK_LOADING;
    current_ev = em->AddEvent(this, LOADING_TIME, 1);
}

/// Startet Schiffs-Angreiff
void noShip::StartSeaAttack()
{
    state = STATE_SEAATTACK_DRIVINGTODESTINATION;
    StartDrivingToHarborPlace();
    HandleState_SeaAttackDriving();
}

void noShip::AbortSeaAttack()
{
    RTTR_Assert(state != STATE_SEAATTACK_WAITING); // figures are not aboard if this fails!
    RTTR_Assert(remaining_sea_attackers == 0); // Some soldiers are still not aboard

    if ((state == STATE_SEAATTACK_LOADING || state == STATE_SEAATTACK_DRIVINGTODESTINATION) &&
        goal_harbor_id != home_harbor && home_harbor != 0)
    {
        // We did not start the attack yet and we can (possibly) go back to our home harbor
        // -> tell the soldiers we go back (like after an attack)
        goal_harbor_id = home_harbor;
        for (std::list<noFigure*>::iterator it = figures.begin(); it != figures.end(); ++it)
        {
            RTTR_Assert(dynamic_cast<nofAttacker*>(*it));
            static_cast<nofAttacker*>(*it)->StartReturnViaShip(*this);
        }
        if(state == STATE_SEAATTACK_LOADING)
        {
            // We are still loading (loading event must be active)
            // -> Use it to unload
            RTTR_Assert(current_ev);
            state = STATE_SEAATTACK_UNLOADING;
        }else
        {
            // Else start driving back
            state = STATE_SEAATTACK_RETURN_DRIVING;
            HandleState_SeaAttackReturn();
        }
    }else
    {
        // attack failed and we cannot go back to our home harbor 
        // -> Tell figures that they won't go to their planned destination
        for (std::list<noFigure*>::iterator it = figures.begin(); it != figures.end(); ++it)
        {
            RTTR_Assert(dynamic_cast<nofAttacker*>(*it));
            static_cast<nofAttacker*>(*it)->CancelSeaAttack();
        }

        if (state == STATE_SEAATTACK_LOADING)
        {
            // Abort loading
            RTTR_Assert(current_ev);
            em->RemoveEvent(current_ev);
        }

        // Das Schiff muss einen Notlandeplatz ansteuern
        FindUnloadGoal(STATE_SEAATTACK_RETURN_DRIVING);
    }
}

/// Fängt an zu einem Hafen zu fahren (berechnet Route usw.)
void noShip::StartDrivingToHarborPlace()
{
    if(!goal_harbor_id)
    {
        route_.clear();
        curRouteIdx = 0;
        return;
    }

    MapPoint coastalPos = gwg->GetCoastalPoint(goal_harbor_id, seaId_);
    if(pos == coastalPos)
        route_.clear();
    else if(!gwg->FindShipPath(pos, coastalPos, &route_, NULL))
    {
        // todo
        RTTR_Assert(false);
        LOG.lprintf("WARNING: Bug detected (GF: %u). Please report this with the savegame and replay. noShip::StartDrivingToHarborPlace: Schiff hat keinen Weg gefunden! player %i state %i pos %u,%u goal coastal %u,%u goal-id %i goalpos %u,%u \n",
            GAMECLIENT.GetGFNumber(), player, state, pos.x, pos.y, coastalPos.x, coastalPos.y, goal_harbor_id, gwg->GetHarborPoint(goal_harbor_id).x, gwg->GetHarborPoint(goal_harbor_id).y);
        goal_harbor_id = 0;
        return;
    }
    curRouteIdx = 0;
}

/// Startet die eigentliche Transportaktion, nachdem das Schiff beladen wurde
void noShip::StartTransport()
{
    state = STATE_TRANSPORT_DRIVING;

    StartDrivingToHarborPlace();
    // Einfach weiterfahren
    HandleState_TransportDriving();
}

void noShip::FindUnloadGoal(State newState)
{
    state = newState;
    // Das Schiff muss einen Notlandeplatz ansteuern
    // Neuen Hafen suchen
    if(gwg->GetPlayer(player).FindHarborForUnloading(this, pos, &goal_harbor_id, &route_, NULL))
    {
        curRouteIdx = 0;
        home_harbor = goal_harbor_id; // To allow unloading here
        if(state == STATE_EXPEDITION_DRIVING)
            HandleState_ExpeditionDriving();
        else if(state == STATE_TRANSPORT_DRIVING)
            HandleState_TransportDriving();
        else if(state == STATE_SEAATTACK_RETURN_DRIVING)
            HandleState_SeaAttackReturn();
        else
        {
            RTTR_Assert(false);
            LOG.lprintf("Bug detected: Invalid state for FindUnloadGoal");
            FindUnloadGoal(STATE_TRANSPORT_DRIVING);
        }
    }
    else
    {
        // Ansonsten als verloren markieren, damit uns später Bescheid gesagt wird
        // wenn es einen neuen Hafen gibt
        home_harbor = goal_harbor_id = 0;
        lost = true;
    }
}

/// Sagt dem Schiff, das ein bestimmter Hafen zerstört wurde
void noShip::HarborDestroyed(nobHarborBuilding* hb)
{
    const unsigned destroyedHarborId = hb->GetHarborPosID();
    // Almost every case of a destroyed harbor is handled when the ships event fires (the handler detects the destroyed harbor)
    // So mostly we just reset the corresponding id

    if (destroyedHarborId == home_harbor)
        home_harbor = 0;

    // Ist unser Ziel betroffen?
    if(destroyedHarborId != goal_harbor_id)
    {
        return;
    }

    State oldState = state;

    switch (state)
    {
    default:
        // Just reset goal, but not for expeditions
        if(!IsOnExpedition() && !IsOnExplorationExpedition())
        {
            goal_harbor_id = 0;
        }
        return; // Skip the rest
    case noShip::STATE_TRANSPORT_LOADING:
    case noShip::STATE_TRANSPORT_UNLOADING:
    // Tell wares and figures that they won't reach their goal
        for(std::list<noFigure*>::iterator it = figures.begin(); it != figures.end(); ++it)
        {
            (*it)->Abrogate();
            (*it)->SetGoalToNULL();
        }
        for(std::list<Ware*>::iterator it = wares.begin(); it != wares.end(); ++it)
        {
            // Notify goal only, if it is not the destroyed harbor. It already knows about that ;)
            if((*it)->GetGoal() != hb)
                (*it)->NotifyGoalAboutLostWare();
            else
                (*it)->SetGoal(NULL);
        }
        break;
    case noShip::STATE_SEAATTACK_LOADING:
        // We could also just set the goal harbor id to 0 but this can reuse the event
        AbortSeaAttack();
        break;
    case noShip::STATE_SEAATTACK_UNLOADING:
        break;
    }

    // Are we currently getting the wares?
    if(oldState == STATE_TRANSPORT_LOADING)
    {
        RTTR_Assert(current_ev);
        if (home_harbor)
        {
            // Then save us some time and unload immediately
            // goal is now the start harbor (if it still exists)
            goal_harbor_id = home_harbor;
            state = STATE_TRANSPORT_UNLOADING;
        }
        else
        {
            em->RemoveEvent(current_ev);
            FindUnloadGoal(STATE_TRANSPORT_DRIVING);
        }
    }
    else if(oldState == STATE_TRANSPORT_UNLOADING || oldState == STATE_SEAATTACK_UNLOADING)
    {
        // Remove current unload event
        em->RemoveEvent(current_ev);

        if(oldState == STATE_SEAATTACK_UNLOADING)
            AbortSeaAttack();
        else
            FindUnloadGoal(STATE_TRANSPORT_DRIVING);
    }
}

/// Fängt an mit idlen und setzt nötigen Sachen auf NULL
void noShip::StartIdling()
{
    // If those are not empty, then we ware lost, not idling!
    RTTR_Assert(figures.empty());
    RTTR_Assert(wares.empty());
    RTTR_Assert(remaining_sea_attackers == 0);

    home_harbor = 0;
    goal_harbor_id = 0;
    state = STATE_IDLE;
}

/// Sagt Bescheid, dass ein Schiffsangreifer nicht mehr mit nach Hause fahren will
void noShip::SeaAttackerWishesNoReturn()
{
    RTTR_Assert(remaining_sea_attackers);
    RTTR_Assert(state == STATE_SEAATTACK_WAITING);

    --remaining_sea_attackers;
    // Alle Soldaten an Bord
    if(remaining_sea_attackers == 0)
    {
        // Andere Events ggf. erstmal abmelden
        em->RemoveEvent(current_ev);
        if(!figures.empty())
        {
            // Go back home. Note: home_harbor can be 0 if it was destroyed, allow this and let the state handlers handle that case later
            goal_harbor_id = home_harbor;
            state = STATE_SEAATTACK_RETURN_DRIVING;
            StartDrivingToHarborPlace();
            HandleState_SeaAttackReturn();
        }
        else
        {
            // Wenn keine Soldaten mehr da sind können wir auch erstmal idlen
            StartIdling();
            gwg->GetPlayer(player).GetJobForShip(this);
        }
    }

}

/// Schiffs-Angreifer sind nach dem Angriff wieder zurückgekehrt
void noShip::AddReturnedAttacker(nofAttacker* attacker)
{
    RTTR_Assert(!helpers::contains(figures, attacker));

    figures.push_back(attacker);
    // Nun brauchen wir quasi einen Angreifer weniger
    SeaAttackerWishesNoReturn();
}

/// Weist das Schiff an, seine Erkundungs-Expedition fortzusetzen
void noShip::ContinueExplorationExpedition()
{
    // Sind wir schon über unserem Limit, also zu weit gefahren
    if(covered_distance >= MAX_EXPLORATION_EXPEDITION_DISTANCE)
    {
        // Ggf. sind wir schon da?
        if(goal_harbor_id == home_harbor)
        {
            // Dann sind wir fertig -> wieder entladen
            state = STATE_EXPLORATIONEXPEDITION_UNLOADING;
            current_ev = em->AddEvent(this, UNLOADING_TIME, 1);
            return;
        }

        // Dann steuern wir unseren Heimathafen an!
        goal_harbor_id = home_harbor;
    }
    else
    {
        // Nächsten Zielpunkt bestimmen
        std::vector<unsigned> hps;
        if(goal_harbor_id)
            hps = gwg->GetHarborPointsWithinReach(goal_harbor_id, seaId_);

        // Keine möglichen Häfen gefunden?
        if(hps.empty())
            // Dann wieder Heimathafen ansteuern
            goal_harbor_id = home_harbor;
        else
            // Zufällig den nächsten Hafen auswählen
            goal_harbor_id = hps[RANDOM.Rand(__FILE__, __LINE__, GetObjId(), hps.size())];
    }

    StartDrivingToHarborPlace();
    state = STATE_EXPLORATIONEXPEDITION_DRIVING;
    HandleState_ExplorationExpeditionDriving();
}

/// Sagt dem Schiff, dass ein neuer Hafen erbaut wurde
void noShip::NewHarborBuilt(nobHarborBuilding* hb)
{
    if(!lost)
        return;
    // Liegt der Hafen auch am Meer von diesem Schiff?
    if(!gwg->IsAtThisSea(hb->GetHarborPosID(), seaId_))
        return;

    //LOG.lprintf("lost ship has new goal harbor player %i state %i pos %u,%u \n",player,state,x,y);
    home_harbor = goal_harbor_id = hb->GetHarborPosID();
    lost = false;

    StartDrivingToHarborPlace();

    switch(state)
    {
    case STATE_EXPLORATIONEXPEDITION_DRIVING:
    case STATE_EXPEDITION_DRIVING:
    case STATE_TRANSPORT_DRIVING:
    case STATE_SEAATTACK_RETURN_DRIVING:
        Driven();
        break;
    default:
        RTTR_Assert(false); // Das darf eigentlich nicht passieren
        LOG.lprintf("Bug detected: Invalid state in NewHarborBuilt");
        break;
    }
}
