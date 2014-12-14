// $Id: nobHarborBuilding.cpp 9546 2014-12-14 12:06:35Z marcus $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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


#include "main.h"
#include "nobHarborBuilding.h"
#include "GameWorld.h"
#include "Loader.h"
#include "noExtension.h"
#include "MilitaryConsts.h"
#include "GameClient.h"
#include "GameClientPlayer.h"
#include "Ware.h"
#include "EventManager.h"
#include "noShip.h"
#include "noFigure.h"
#include "Random.h"
#include "nobMilitary.h"
#include "nofAttacker.h"
#include "nofDefender.h"

#include "glSmartBitmap.h"

#include <set>

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


nobHarborBuilding::ExpeditionInfo::ExpeditionInfo(SerializedGameData* sgd) :
    active(sgd->PopBool()),
    boards(sgd->PopUnsignedInt()),
    stones(sgd->PopUnsignedInt()),
    builder(sgd->PopBool())
{
}

void nobHarborBuilding::ExpeditionInfo::Serialize(SerializedGameData* sgd) const
{
    sgd->PushBool(active);
    sgd->PushUnsignedInt(boards);
    sgd->PushUnsignedInt(stones);
    sgd->PushBool(builder);
}

nobHarborBuilding::ExplorationExpeditionInfo::ExplorationExpeditionInfo(SerializedGameData* sgd) :
    active(sgd->PopBool()),
    scouts(sgd->PopUnsignedInt())
{
}

void nobHarborBuilding::ExplorationExpeditionInfo::Serialize(SerializedGameData* sgd) const
{
    sgd->PushBool(active);
    sgd->PushUnsignedInt(scouts);
}



nobHarborBuilding::nobHarborBuilding(const unsigned short x, const unsigned short y, const unsigned char player, const Nation nation)
    : nobBaseWarehouse(BLD_HARBORBUILDING, x, y, player, nation), orderware_ev(0)
{
    // ins Militärquadrat einfügen
    gwg->GetMilitarySquare(x, y).push_back(this);
    gwg->RecalcTerritory(this, GetMilitaryRadius(), false, true);

    // Alle Waren 0, außer 100 Träger
    goods.clear();
    real_goods.clear();

    // Aktuellen Warenbestand zur aktuellen Inventur dazu addieren
    AddToInventory();

    // Der Wirtschaftsverwaltung Bescheid sagen
    gwg->GetPlayer(player)->AddWarehouse(this);

    /// Die Meere herausfinden, an die dieser Hafen grenzt
    for(unsigned i = 0; i < 6; ++i)
        sea_ids[i] = gwg->IsCoastalPoint(gwg->GetXA(x, y, i), gwg->GetYA(x, y, i));

    // Post versenden
    if(GameClient::inst().GetPlayerID() == this->player)
        GameClient::inst().SendPostMessage(new ImagePostMsgWithLocation(
                                               _("New harbor building finished"), PMC_GENERAL, x, y, BLD_HARBORBUILDING, nation));
}



void nobHarborBuilding::Destroy()
{
    em->RemoveEvent(orderware_ev);
    players->getElement(player)->HarborDestroyed(this);

    // Der Wirtschaftsverwaltung Bescheid sagen
    gwg->GetPlayer(player)->RemoveWarehouse(this);
    gwg->GetPlayer(player)->RemoveHarbor(this);

    // Baumaterialien in der Inventur verbuchen
    if(expedition.active)
    {
        gwg->GetPlayer(player)->DecreaseInventoryWare(GD_BOARDS, expedition.boards);
        gwg->GetPlayer(player)->DecreaseInventoryWare(GD_STONES, expedition.stones);

        // Und Bauarbeiter (später) rausschicken
        if(expedition.builder)
            ++real_goods.people[JOB_BUILDER];
        else
            gwg->GetPlayer(player)->OneJobNotWanted(JOB_BUILDER, this);
    }
    //cancel order for scouts
    if (exploration_expedition.active)
    {
		real_goods.people[JOB_SCOUT] += exploration_expedition.scouts;
        for (unsigned i = exploration_expedition.scouts; i < SCOUTS_EXPLORATION_EXPEDITION; i++)
        {
            gwg->GetPlayer(player)->OneJobNotWanted(JOB_SCOUT, this);
        }
    }
	//cancel all jobs wanted for this building
	gwg->GetPlayer(player)->JobNotWanted(this,true);
    // Waiting Wares löschen
    for(std::list<Ware*>::iterator it = wares_for_ships.begin(); it != wares_for_ships.end(); ++it)
    {
        (*it)->WareLost(player);
        delete (*it);
    }
    wares_for_ships.clear();

    // Leute, die noch aufs Schiff warten, rausschicken
    for(std::list<FigureForShip>::iterator it = figures_for_ships.begin(); it != figures_for_ships.end(); ++it)
    {
        gwg->AddFigure(it->fig, x, y);

        it->fig->Abrogate();
        it->fig->StartWandering();
        it->fig->StartWalking(RANDOM.Rand(__FILE__, __LINE__, obj_id, 6));
    }
    figures_for_ships.clear();

    for(std::list<SoldierForShip>::iterator it = soldiers_for_ships.begin(); it != soldiers_for_ships.end(); ++it)
    {
        gwg->AddFigure(it->attacker, x, y);

        it->attacker->CancelAtHomeMilitaryBuilding();
        it->attacker->StartWandering();
        it->attacker->StartWalking(RANDOM.Rand(__FILE__, __LINE__, obj_id, 6));
    }
    soldiers_for_ships.clear();



    Destroy_nobBaseWarehouse();

    // Land drumherum neu berechnen (nur wenn es schon besetzt wurde!)
    // Nach dem BaseDestroy erst, da in diesem erst das Feuer gesetzt, die Straße gelöscht wird usw.
    gwg->RecalcTerritory(this, HARBOR_ALONE_RADIUS, true, false);

    // Wieder aus dem Militärquadrat rauswerfen
    gwg->GetMilitarySquare(x, y).erase(this);


}

void nobHarborBuilding::Serialize(SerializedGameData* sgd) const
{
    Serialize_nobBaseWarehouse(sgd);
    expedition.Serialize(sgd);
    exploration_expedition.Serialize(sgd);
    sgd->PushObject(orderware_ev, true);
    for(unsigned i = 0; i < 6; ++i)
        sgd->PushUnsignedShort(sea_ids[i]);
    sgd->PushObjectList(wares_for_ships, true);
    sgd->PushUnsignedInt(figures_for_ships.size());
    for(std::list<FigureForShip>::const_iterator it = figures_for_ships.begin(); it != figures_for_ships.end(); ++it)
    {
        sgd->PushUnsignedShort(it->dest.x);
        sgd->PushUnsignedShort(it->dest.y);
        sgd->PushObject(it->fig, false);
    }
    sgd->PushUnsignedInt(soldiers_for_ships.size());
    for(std::list<SoldierForShip>::const_iterator it = soldiers_for_ships.begin(); it != soldiers_for_ships.end(); ++it)
    {
        sgd->PushUnsignedShort(it->dest.x);
        sgd->PushUnsignedShort(it->dest.y);
        sgd->PushObject(it->attacker, true);
    }



}

nobHarborBuilding::nobHarborBuilding(SerializedGameData* sgd, const unsigned obj_id)
    : nobBaseWarehouse(sgd, obj_id),
      expedition(sgd),
      exploration_expedition(sgd),
      orderware_ev(sgd->PopObject<EventManager::Event>(GOT_EVENT))
{
    // ins Militärquadrat einfügen
    gwg->GetMilitarySquare(x, y).push_back(this);

    for(unsigned i = 0; i < 6; ++i)
        sea_ids[i] = sgd->PopUnsignedShort();

    sgd->PopObjectList<Ware>(wares_for_ships, GOT_WARE);

    unsigned count = sgd->PopUnsignedInt();
    for(unsigned i = 0; i < count; ++i)
    {
        FigureForShip ffs;
        ffs.dest.x = sgd->PopUnsignedShort();
        ffs.dest.y = sgd->PopUnsignedShort();
        ffs.fig = sgd->PopObject<noFigure>(GOT_UNKNOWN);
        figures_for_ships.push_back(ffs);
    }

    count = sgd->PopUnsignedInt();
    for(unsigned i = 0; i < count; ++i)
    {
        SoldierForShip ffs;
        ffs.dest.x = sgd->PopUnsignedShort();
        ffs.dest.y = sgd->PopUnsignedShort();
        ffs.attacker = sgd->PopObject<nofAttacker>(GOT_NOF_ATTACKER);
        soldiers_for_ships.push_back(ffs);
    }

}

// Relative Position des Bauarbeiters
const Point<int> BUILDER_POS[NATION_COUNT] = { Point<int>(0, 18), Point<int>(-8, 17), Point<int>(0, 15), Point<int>(-18, 17), Point<int>(-18, 17) };
/// Relative Position der Brettertürme
const Point<int> BOARDS_POS[NATION_COUNT] = { Point<int>(-70, -5), Point<int>(-55, -5), Point<int>(-50, -5), Point<int>(-60, -5), Point<int>(-60, -5) };
/// Relative Position der Steintürme
const Point<int> STONES_POS[NATION_COUNT] = { Point<int>(-73, 10), Point<int>(-60, 10), Point<int>(-50, 10), Point<int>(-60, 10), Point<int>(-60, 10) };
/// Relative Postion der inneren Hafenfeuer
const Point<int> FIRE_POS[NATION_COUNT] = { Point<int>(36, -51), Point<int>(0, 0), Point<int>(0, 0), Point<int>(5, -80), Point<int>(0, 0) };
/// Relative Postion der äußeren Hafenfeuer
const Point<int> EXTRAFIRE_POS[NATION_COUNT] = { Point<int>(0, 0), Point<int>(0, 0), Point<int>(8, -115), Point<int>(0, 0), Point<int>(0, 0) };

void nobHarborBuilding::Draw(int x, int y)
{
    // Gebäude an sich zeichnen
    DrawBaseBuilding(x, y);

    // Hafenfeuer zeichnen // TODO auch für nicht-römer machen
    if (nation == NAT_ROMANS || nation == NAT_JAPANESES || nation == NAT_BABYLONIANS)
    {
        LOADER.GetNationImageN(nation, 500 + 5 * GameClient::inst().GetGlobalAnimation(8, 2, 1, obj_id + GetX() + GetY()))->Draw(x + FIRE_POS[nation].x, y + FIRE_POS[nation].y, 0, 0, 0, 0, 0, 0);
    }
    else if (nation == NAT_AFRICANS || nation == NAT_VIKINGS)
    {
        LOADER.GetMapImageN(740 + GameClient::inst().GetGlobalAnimation(8, 5, 2, obj_id + GetX() + GetY()))->Draw(x + FIRE_POS[nation].x, y + FIRE_POS[nation].y);
    }

    if (nation == NAT_ROMANS)
    {
        // Zusätzliches Feuer
        LOADER.GetMapImageN(740 + GameClient::inst().GetGlobalAnimation(8, 5, 2, obj_id + GetX() + GetY()))->Draw(x + EXTRAFIRE_POS[nation].x, y + EXTRAFIRE_POS[nation].y);
    }

    // Läuft gerade eine Expedition?
    if(expedition.active)
    {
        // Waren für die Expedition zeichnen

        // Bretter
        for(unsigned char i = 0; i < expedition.boards; ++i)
            LOADER.GetMapImageN(2200 + GD_BOARDS)->Draw(x + BOARDS_POS[nation].x - 5, y + BOARDS_POS[nation].y - i * 4, 0, 0, 0, 0, 0, 0);
        // Steine
        for(unsigned char i = 0; i < expedition.stones; ++i)
            LOADER.GetMapImageN(2200 + GD_STONES)->Draw(x + STONES_POS[nation].x + 8, y + STONES_POS[nation].y - i * 4, 0, 0, 0, 0, 0, 0);

        // Und den Bauarbeiter, falls er schon da ist
        if(expedition.builder)
        {
            unsigned id = GameClient::inst().GetGlobalAnimation(1000, 7, 1, GetX() + GetY());

            const int WALKING_DISTANCE = 30;

            // Wegstrecke, die er von einem Punkt vom anderen schon gelaufen ist
            int walking_distance = (id % 500) * WALKING_DISTANCE / 500;
            // Id vom laufen
            unsigned walking_id = (id / 32) % 8;

            int right_point = x - 20 + BUILDER_POS[nation].x;

            if(id < 500)
            {
                Loader::bob_jobs_cache[nation][JOB_BUILDER][0][walking_id].draw(right_point - walking_distance, y + BUILDER_POS[nation].y, COLOR_WHITE, COLORS[gwg->GetPlayer(player)->color]);
//              LOADER.GetBobN("jobs")->Draw(23,0,false,walking_id,right_point-walking_distance,
//                  y+BUILDER_POS[nation].y,COLORS[gwg->GetPlayer(player)->color]);
                //DrawShadow(right_point-walking_distance,y,walking_id,0);
            }
            else
            {
                Loader::bob_jobs_cache[nation][JOB_BUILDER][3][walking_id].draw(right_point - WALKING_DISTANCE + walking_distance, y + BUILDER_POS[nation].y, COLOR_WHITE, COLORS[gwg->GetPlayer(player)->color]);
//              LOADER.GetBobN("jobs")->Draw(23,3,false,walking_id,
//                  right_point-WALKING_DISTANCE+walking_distance,y+BUILDER_POS[nation].y,
//                  COLORS[gwg->GetPlayer(player)->color]);
                //DrawShadow(right_point-WALKING_DISTANCE+walking_distance,y,walking_id,0);
            }
        }

    }
}


void nobHarborBuilding::HandleEvent(const unsigned int id)
{
    switch(id)
    {
            // Waren-Bestell-Event
        case 10:
        {
            this->orderware_ev = NULL;
            // Mal wieder schauen, ob es Waren für unsere Expedition gibt
            OrderExpeditionWares();
        } break;
        default: HandleBaseEvent(id);
    }
}

/// Startet eine Expedition oder stoppt sie, wenn bereits eine stattfindet
void nobHarborBuilding::StartExpedition()
{
    // Schon eine Expedition gestartet?
    if(expedition.active)
    {
        // Dann diese stoppen
        expedition.active = false;

        // Waren zurücktransferieren
        real_goods.goods[GD_BOARDS] += expedition.boards;
        goods.goods[GD_BOARDS] += expedition.boards;
        real_goods.goods[GD_STONES] += expedition.stones;
        goods.goods[GD_STONES] += expedition.stones;

        if(expedition.builder)
        {
            ++real_goods.people[JOB_BUILDER];
            ++goods.people[JOB_BUILDER];
            // Evtl. Abnehmer für die Figur wieder finden
            gwg->GetPlayer(player)->FindWarehouseForAllJobs(JOB_BUILDER);
        }
        else //todo falls noch nicht da - unterscheiden ob unterwegs oder nur bestellt - falls bestellt stornieren sonst informieren damit kein ersatz geschickt wird falls was nicht klappt aufm weg
        {
            gwg->GetPlayer(player)->OneJobNotWanted(JOB_BUILDER, this);
        }

        return;
    }

    // Initialisierung
    expedition.active = true;

    // In unseren Warenbestand gucken und die erforderlichen Bretter und Steine sowie den
    // Bauarbeiter holen, falls vorhanden
    expedition.boards = min(unsigned(BUILDING_COSTS[nation][BLD_HARBORBUILDING].boards), real_goods.goods[GD_BOARDS]);
    expedition.stones = min(unsigned(BUILDING_COSTS[nation][BLD_HARBORBUILDING].stones), real_goods.goods[GD_STONES]);
    real_goods.goods[GD_BOARDS] -= expedition.boards;
    goods.goods[GD_BOARDS] -= expedition.boards;
    real_goods.goods[GD_STONES] -= expedition.stones;
    goods.goods[GD_STONES] -= expedition.stones;

    if(real_goods.people[JOB_BUILDER])
    {
        expedition.builder = true;
        --real_goods.people[JOB_BUILDER];
        --goods.people[JOB_BUILDER];
    }
    else
    {
        bool convert = true;
        expedition.builder = false;
        //got a builder in ANY storehouse?
        for(std::list<nobBaseWarehouse*>::const_iterator it = gwg->GetPlayer(player)->GetStorehouses().begin(); it != gwg->GetPlayer(player)->GetStorehouses().end(); it++)
        {
            if((*it)->GetRealFiguresCount(JOB_BUILDER))
            {
                convert = false;
                break;
            }
        }
        if(convert && real_goods.goods[GD_HAMMER] && real_goods.people[JOB_HELPER] > 1) //maybe have a hammer & helper to create our own builder?
        {
            --real_goods.goods[GD_HAMMER];
            --goods.goods[GD_HAMMER];
            gwg->GetPlayer(player)->DecreaseInventoryWare(GD_HAMMER, 1);
            --real_goods.people[JOB_HELPER];
            --goods.people[JOB_HELPER];
            gwg->GetPlayer(player)->DecreaseInventoryJob(JOB_HELPER, 1);

            gwg->GetPlayer(player)->IncreaseInventoryJob(JOB_BUILDER, 1);
            expedition.builder = true;
        }
        // not in harbor, and didnt have to or couldnt convert so order a builder
        if(!expedition.builder)
            gwg->GetPlayer(player)->AddJobWanted(JOB_BUILDER, this);
    }

    // Ggf. Waren bestellen, die noch fehlen
    OrderExpeditionWares();

    // Ggf. ist jetzt alles benötigte schon da
    // Dann Schiff rufen
    CheckExpeditionReady();

}

/// Startet eine Erkundungs-Expedition oder stoppt sie, wenn bereits eine stattfindet
void nobHarborBuilding::StartExplorationExpedition()
{
    // Schon eine Expedition gestartet?
    if(exploration_expedition.active)
    {
        // Dann diese stoppen
        exploration_expedition.active = false;
        // cancel order for scouts
        for (unsigned i = exploration_expedition.scouts; i < SCOUTS_EXPLORATION_EXPEDITION; i++)
        {
            gwg->GetPlayer(player)->OneJobNotWanted(JOB_SCOUT, this);
        }
        // Erkunder zurücktransferieren
        if(exploration_expedition.scouts)
        {
            real_goods.people[JOB_SCOUT] += exploration_expedition.scouts;
            // Evtl. Abnehmer für die Figur wieder finden
            gwg->GetPlayer(player)->FindWarehouseForAllJobs(JOB_SCOUT);
        }
        return;
    }

    // Initialisierung
    exploration_expedition.active = true;

    exploration_expedition.scouts = 0;

    // In unseren Warenbestand gucken und die erforderlichen Erkunder rausziehen
    if(real_goods.people[JOB_SCOUT])
    {
        exploration_expedition.scouts = min(real_goods.people[JOB_SCOUT], SCOUTS_EXPLORATION_EXPEDITION);

        real_goods.people[JOB_SCOUT] -= exploration_expedition.scouts;
    }
    if(exploration_expedition.scouts < SCOUTS_EXPLORATION_EXPEDITION)
    {
        unsigned missing = SCOUTS_EXPLORATION_EXPEDITION - exploration_expedition.scouts;
        //got scouts in ANY storehouse?
        for(std::list<nobBaseWarehouse*>::const_iterator it = gwg->GetPlayer(player)->GetStorehouses().begin(); it != gwg->GetPlayer(player)->GetStorehouses().end(); it++)
        {
            if((*it)->GetRealFiguresCount(JOB_SCOUT))
            {
                (*it)->GetRealFiguresCount(JOB_SCOUT) >= missing ? missing = 0 : missing -= (*it)->GetRealFiguresCount(JOB_SCOUT);
                if (!missing)
                    break;
            }
        }
        while(missing && real_goods.goods[GD_BOW] && real_goods.people[JOB_HELPER] > 1) //maybe have bows & helpers to create our own scouts?
        {
            --real_goods.goods[GD_BOW];
            --goods.goods[GD_BOW];
            gwg->GetPlayer(player)->DecreaseInventoryWare(GD_BOW, 1);
            --real_goods.people[JOB_HELPER];
            --goods.people[JOB_HELPER];
            gwg->GetPlayer(player)->DecreaseInventoryJob(JOB_HELPER, 1);

            ++goods.people[JOB_SCOUT];
            gwg->GetPlayer(player)->IncreaseInventoryJob(JOB_SCOUT, 1);
            missing--;
            exploration_expedition.scouts++;
        }
        // not in harbor, and didnt have to or couldnt convert so order scouts
        // Den Rest bestellen
        for(unsigned i = exploration_expedition.scouts; i < SCOUTS_EXPLORATION_EXPEDITION; ++i)
            gwg->GetPlayer(player)->AddJobWanted(JOB_SCOUT, this);
    }


    CheckExplorationExpeditionReady();

}


/// Bestellt die zusätzlichen erforderlichen Waren für eine Expedition
void nobHarborBuilding::OrderExpeditionWares()
{
    if(!expedition.active) //expedition no longer active?
        return;
    // Waren in der Bestellungsliste mit beachten
    unsigned boards = 0, stones = 0;
    for(std::list<Ware*>::iterator it = dependent_wares.begin(); it!=dependent_wares.end(); ++it)
    {
        if (*it == 0) // qx: check for bug #1132707
        {
            std::cout << "Error: Iterator to 0-Ware" << std::endl;
        }
        else
        {
            if((*it)->type == GD_BOARDS)
                ++boards;
            if((*it)->type == GD_STONES)
                ++stones;
        }
    }

    // Prüfen, ob jeweils noch weitere Waren bestellt werden müssen
    unsigned todo_boards = 0;
    if(boards + expedition.boards < BUILDING_COSTS[nation][BLD_HARBORBUILDING].boards)
    {
        todo_boards = BUILDING_COSTS[nation][BLD_HARBORBUILDING].boards - (boards + expedition.boards);
        Ware* ware;
        do
        {
            ware = gwg->GetPlayer(player)->OrderWare(GD_BOARDS, this);
            if(ware)
            {
                dependent_wares.push_back(ware);
                --todo_boards;
            }
        }
        while(ware && todo_boards);
    }

    unsigned todo_stones = 0;
    if(stones + expedition.stones < BUILDING_COSTS[nation][BLD_HARBORBUILDING].stones)
    {
        todo_stones = BUILDING_COSTS[nation][BLD_HARBORBUILDING].stones - (stones + expedition.stones);
        Ware* ware;
        do
        {
            ware = gwg->GetPlayer(player)->OrderWare(GD_STONES, this);
            if(ware)
            {
                dependent_wares.push_back(ware);
                --todo_stones;
            }
        }
        while(ware && todo_stones);
    }

    // Wenn immer noch nicht alles da ist, später noch einmal bestellen
    if(orderware_ev == NULL)
        orderware_ev = em->AddEvent(this, 210, 10);

}


/// Eine bestellte Ware konnte doch nicht kommen
void nobHarborBuilding::WareLost(Ware* ware)
{
    // ggf. neue Waren für Expedition bestellen
    if(expedition.active && (ware->type == GD_BOARDS || ware->type == GD_STONES))
        OrderExpeditionWares();
    RemoveDependentWare(ware);
}



/// Schiff ist angekommen
void nobHarborBuilding::ShipArrived(noShip* ship)
{
    // get a new job - priority is given according to this list: attack,expedition,exploration,transport
    // any attackers ready?
    if(soldiers_for_ships.size())
    {
        // load all soldiers that share the same target as the first soldier in the list
        std::list<noFigure*> attackers;
        Point<MapCoord> ship_dest = soldiers_for_ships.begin()->dest;

        for(std::list<SoldierForShip>::iterator it = soldiers_for_ships.begin(); it != soldiers_for_ships.end();)
        {
            if(it->dest == ship_dest)
            {
                --goods.people[it->attacker->GetJobType()];
                attackers.push_back(it->attacker);
                it = soldiers_for_ships.erase(it);
            }
            else
                ++it;
        }

        ship->PrepareSeaAttack(ship_dest, attackers);
        return;
    }
    //Expedition ready?
    if(expedition.active && expedition.builder
            && expedition.boards == BUILDING_COSTS[nation][BLD_HARBORBUILDING].boards
            && expedition.stones == BUILDING_COSTS[nation][BLD_HARBORBUILDING].stones)
    {
        // Aufräumen am Hafen
        expedition.active = false;
        // Expedition starten
        ship->StartExpedition();
        return;
    }
    // Exploration-Expedition ready?
    if(IsExplorationExpeditionReady())
    {
        // Aufräumen am Hafen
        exploration_expedition.active = false;
        // Expedition starten
        ship->StartExplorationExpedition();
        assert(goods.people[JOB_SCOUT] >= exploration_expedition.scouts);
        goods.people[JOB_SCOUT] -= exploration_expedition.scouts;
        return;

    }
    // Gibt es Waren oder Figuren, die ein Schiff von hier aus nutzen wollen?
    if(wares_for_ships.size() || figures_for_ships.size())
    {
        // Das Ziel wird nach der ersten Figur bzw. ersten Ware gewählt
        // actually since the wares might not yet have informed the harbor that their target harbor was destroyed we pick the first figure/ware with a valid target instead
        Point<MapCoord> dest;
        bool gotdest = false;
        for(std::list<FigureForShip>::iterator it = figures_for_ships.begin(); it != figures_for_ships.end(); it++)
        {
            noBase* nb = gwg->GetNO(it->dest.x, it->dest.y);
            if(nb->GetGOT() == GOT_NOB_HARBORBUILDING && gwg->GetNode(it->dest.x, it->dest.y).owner == player + 1) //target is a harbor and owned by the same player
            {
                dest = it->dest;
                gotdest = true;
                break;
            }
        }
        for(std::list<Ware*>::iterator it = wares_for_ships.begin(); !gotdest && it != wares_for_ships.end(); it++)
        {
            noBase* nb = gwg->GetNO((*it)->GetNextHarbor().x, (*it)->GetNextHarbor().y);
            if(nb->GetGOT() == GOT_NOB_HARBORBUILDING && gwg->GetNode((*it)->GetNextHarbor().x, (*it)->GetNextHarbor().y).owner == player + 1)
            {
                dest = (*it)->GetNextHarbor();
                gotdest = true;
                break;
            }
        }
        if(gotdest)
        {
            std::list<noFigure*> figures;

            // Figuren auswählen, die zu diesem Ziel wollen
            for(std::list<FigureForShip>::iterator it = figures_for_ships.begin();
                    it != figures_for_ships.end() && figures.size() < SHIP_CAPACITY;)
            {
                if(it->dest == dest)
                {
                    figures.push_back(it->fig);
                    it->fig->StartShipJourney(dest);
                    --goods.people[it->fig->GetJobType()];
                    it = figures_for_ships.erase(it);

                }
                else
                    ++it;
            }

            // Und noch die Waren auswählen
            std::list<Ware*> wares;
            for(std::list<Ware*>::iterator it = wares_for_ships.begin();
                    it != wares_for_ships.end() && figures.size() + wares.size() < SHIP_CAPACITY;)
            {
                if((*it)->GetNextHarbor() == dest)
                {
                    wares.push_back(*it);
                    (*it)->StartShipJourney();
                    --goods.goods[ConvertShields((*it)->type)];
                    it = wares_for_ships.erase(it);

                }
                else
                    ++it;
            }

            // Und das Schiff starten lassen
            ship->PrepareTransport(dest, figures, wares);
        }
    }
}

/// Legt eine Ware im Lagerhaus ab
void nobHarborBuilding::AddWare(Ware* ware)
{
    if(ware->goal != this)
        ware->RecalcRoute();

    // Will diese Ware mit dem Schiff irgendwo hin fahren?
    if(ware->GetNextDir() == SHIP_DIR)
    {
        // Dann fügen wir die mal bei uns hinzu
        AddWareForShip(ware);
        return;
    }

    // Brauchen wir die Ware?
    if(expedition.active)
    {
        if((ware->type == GD_BOARDS && expedition.boards < BUILDING_COSTS[nation][BLD_HARBORBUILDING].boards)
                || (ware->type == GD_STONES && expedition.stones < BUILDING_COSTS[nation][BLD_HARBORBUILDING].stones))
        {
            if(ware->type == GD_BOARDS) ++expedition.boards;
            else ++expedition.stones;

            // Ware nicht mehr abhängig
            RemoveDependentWare(ware);
            // Dann zweigen wir die einfach mal für die Expedition ab
            gwg->GetPlayer(player)->RemoveWare(ware);
            delete ware;

            // Ggf. ist jetzt alles benötigte da
            CheckExpeditionReady();
            return;
        }
    }

    nobBaseWarehouse::AddWare(ware);

}

/// Eine Figur geht ins Lagerhaus
void nobHarborBuilding::AddFigure(noFigure* figure, const bool increase_visual_counts)
{
    // Brauchen wir einen Bauarbeiter für die Expedition?
    if(figure->GetJobType() == JOB_BUILDER && expedition.active && !expedition.builder)
    {

        nobBaseWarehouse::RemoveDependentFigure(figure);
        em->AddToKillList(figure);

        expedition.builder = true;
        // Ggf. ist jetzt alles benötigte da
        CheckExpeditionReady();
    }
    // Brauchen wir einen Spähter für die Expedition?
    else if(figure->GetJobType() == JOB_SCOUT && exploration_expedition.active && !IsExplorationExpeditionReady())
    {
        nobBaseWarehouse::RemoveDependentFigure(figure);
        em->AddToKillList(figure);

        ++exploration_expedition.scouts;
        ++goods.people[JOB_SCOUT];
        // Ggf. ist jetzt alles benötigte da
        CheckExplorationExpeditionReady();
    }
    else
        // ansonsten weiterdelegieren
        nobBaseWarehouse::AddFigure(figure, increase_visual_counts);
}

/// Gibt zurück, ob Expedition vollständig ist
bool nobHarborBuilding::IsExpeditionReady() const
{
    if(!expedition.active)
        return false;
    // Alles da?
    if(expedition.boards < BUILDING_COSTS[nation][BLD_HARBORBUILDING].boards)
        return false;
    if(expedition.stones < BUILDING_COSTS[nation][BLD_HARBORBUILDING].stones)
        return false;
    if(!expedition.builder)
        return false;

    return true;
}

/// Gibt zurück, ob Expedition vollständig ist
bool nobHarborBuilding::IsExplorationExpeditionReady() const
{
    if(!exploration_expedition.active)
        return false;
    // Alles da?
    if(exploration_expedition.scouts < SCOUTS_EXPLORATION_EXPEDITION)
        return false;

    return true;
}

/// Prüft, ob eine Expedition von den Waren her vollständig ist und ruft ggf. das Schiff
void nobHarborBuilding::CheckExpeditionReady()
{
    // Alles da?
    // Dann bestellen wir mal das Schiff
    if(IsExpeditionReady())
        OrderShip();
}

/// Prüft, ob eine Expedition von den Spähern her vollständig ist und ruft ggf. das Schiff
void nobHarborBuilding::CheckExplorationExpeditionReady()
{
    if ((exploration_expedition.scouts < SCOUTS_EXPLORATION_EXPEDITION) && (exploration_expedition.scouts + real_goods.people[JOB_SCOUT] >= SCOUTS_EXPLORATION_EXPEDITION))
    {
        real_goods.people[JOB_SCOUT] -= SCOUTS_EXPLORATION_EXPEDITION - exploration_expedition.scouts;
        exploration_expedition.scouts = SCOUTS_EXPLORATION_EXPEDITION;
    }

    // Alles da?
    // Dann bestellen wir mal das Schiff
    if(IsExplorationExpeditionReady())
        OrderShip();
}

/// Schiff konnte nicht mehr kommen
void nobHarborBuilding::ShipLost(noShip* ship)
{
    // Neues Schiff bestellen
    OrderShip();
}

/// Gibt die Hafenplatz-ID zurück, auf der der Hafen steht
unsigned nobHarborBuilding::GetHarborPosID() const
{
    return gwg->GetHarborPointID(x, y);
}

/// Abfangen, wenn ein Mann nicht mehr kommen kann --> könnte ein Bauarbeiter sein und
/// wenn wir einen benötigen, müssen wir einen neuen bestellen
void nobHarborBuilding::RemoveDependentFigure(noFigure* figure)
{
    nobBaseWarehouse::RemoveDependentFigure(figure);
    // Ist das ein Bauarbeiter und brauchen wir noch einen
    if(figure->GetJobType() == JOB_BUILDER && expedition.active && !expedition.builder)
    {
        // Alle Figuren durchkommen, die noch hierher kommen wollen und gucken, ob ein
        // Bauarbeiter dabei ist
        for(std::list<noFigure*>::iterator it = dependent_figures.begin(); it != dependent_figures.end(); ++it)
        {
            if((*it)->GetJobType() == JOB_BUILDER)
                // Brauchen keinen bestellen, also raus
                return;
        }

        // Keinen gefunden, also müssen wir noch einen bestellen
        players->getElement(player)->AddJobWanted(JOB_BUILDER, this);
    }

    // Ist das ein Erkunder und brauchen wir noch welche?
    else if(figure->GetJobType() == JOB_SCOUT && exploration_expedition.active)
    {
        unsigned scouts_coming = 0;
        // Alle Figuren durchkommen, die noch hierher kommen wollen und gucken, ob ein
        // Bauarbeiter dabei ist
        for(std::list<noFigure*>::iterator it = dependent_figures.begin(); it != dependent_figures.end(); ++it)
        {
            if((*it)->GetJobType() == JOB_SCOUT)
                // Brauchen keinen bestellen, also raus
                ++scouts_coming;
        }

        // Wenn nicht genug Erkunder mehr kommen, müssen wir einen neuen bestellen
        if(exploration_expedition.scouts + scouts_coming < SCOUTS_EXPLORATION_EXPEDITION)
            players->getElement(player)->AddJobWanted(JOB_SCOUT, this);
    }


}

/// Gibt eine Liste mit möglichen Verbindungen zurück
void nobHarborBuilding::GetShipConnections(std::vector<ShipConnection>& connections) const
{
    // Is there any harbor building at all? (could be destroyed)?
    if(gwg->GetGOT(this->x, this->y) != GOT_NOB_HARBORBUILDING)
        // Then good-bye
        return;

    // Is the harbor being destroyed right now?
    if (IsBeingDestroyedNow())
        return;

    std::vector<nobHarborBuilding*> harbor_buildings;
    for(unsigned short sea_id = 0; sea_id < 6; ++sea_id)
    {
        if(sea_ids[sea_id] != 0)
            players->getElement(player)->GetHarborBuildings(harbor_buildings, sea_ids[sea_id]);
    }

    for(unsigned i = 0; i < harbor_buildings.size(); ++i)
    {
        ShipConnection sc;
        sc.dest = harbor_buildings[i];
        // Als Kantengewicht nehmen wir die doppelte Entfernung (evtl muss ja das Schiff erst kommen)
        // plus einer Kopfpauschale (Ein/Ausladen usw. dauert ja alles)
        sc.way_costs = 2 * gwg->CalcHarborDistance(GetHarborPosID(), harbor_buildings[i]->GetHarborPosID()) + 10;
        connections.push_back(sc);
    }
}


/// Fügt einen Mensch hinzu, der mit dem Schiff irgendwo hin fahren will
void nobHarborBuilding::AddFigureForShip(noFigure* fig, Point<MapCoord> dest)
{
    FigureForShip ffs = { fig, dest };
    figures_for_ships.push_back(ffs);
    OrderShip();
    // Anzahl visuell erhöhen
    ++goods.people[fig->GetJobType()];
}

/// Fügt eine Ware hinzu, die mit dem Schiff verschickt werden soll
void nobHarborBuilding::AddWareForShip(Ware* ware)
{
    wares_for_ships.push_back(ware);
    // Anzahl visuell erhöhen
    ++goods.goods[ConvertShields(ware->type)];
    ware->WaitForShip(this);
    OrderShip();

}

/// Gibt Anzahl der Schiffe zurück, die noch für ausstehende Aufgaben benötigt werden
unsigned nobHarborBuilding::GetNeededShipsCount() const
{
    unsigned count = 0;

    // Expedition -> 1 Schiff
    if(IsExpeditionReady())
        ++count;
    // Erkundungs-Expedition -> noch ein Schiff
    if(IsExplorationExpeditionReady())
        ++count;
    // Evtl. Waren und Figuren -> noch ein Schiff pro Ziel
    if ((figures_for_ships.size() > 0) || (wares_for_ships.size() > 0))
    {
        // Die verschiedenen Zielhäfen -> Für jeden Hafen ein Schiff ordern
        std::vector< Point<MapCoord> > destinations;

        for (std::list<FigureForShip>::const_iterator it = figures_for_ships.begin(); it != figures_for_ships.end(); ++it)
        {
            if (std::find(destinations.begin(), destinations.end(), it->dest) == destinations.end())
            {
                destinations.push_back(it->dest);
                ++count;
            }
        }

        for (std::list<Ware*>::const_iterator it = wares_for_ships.begin(); it != wares_for_ships.end(); ++it)
        {
            if (std::find(destinations.begin(), destinations.end(), (*it)->GetNextHarbor())
                    == destinations.end())
            {
                destinations.push_back((*it)->GetNextHarbor());
                ++count;
            }
        }
    }
    // Evtl. Angreifer, die noch verschifft werden müssen
    if(soldiers_for_ships.size())
    {
        // Die verschiedenen Zielhäfen -> Für jeden Hafen ein Schiff ordern
        std::vector< Point<MapCoord> > different_dests;
        for(std::list<SoldierForShip>::const_iterator it = soldiers_for_ships.begin();
                it != soldiers_for_ships.end(); ++it)
        {
            if(std::find(different_dests.begin(), different_dests.end(), it->dest)
                    == different_dests.end())
            {
                different_dests.push_back(it->dest);
                ++count;
            }
        }
    }

    return count;
}

/// Gibt die Wichtigkeit an, dass ein Schiff kommen muss (0 -> keine Bedürftigkeit)
int nobHarborBuilding::GetNeedForShip(unsigned ships_coming) const
{
    int points = 0;

    // Expedition -> 1 Schiff
    if(IsExpeditionReady())
    {
        if(ships_coming == 0)
            points += 100;
        else
            --ships_coming;
    }
    if(IsExplorationExpeditionReady())
    {
        if(ships_coming == 0)
            points += 100;
        else
            --ships_coming;
    }
    if ((figures_for_ships.size() > 0) || (wares_for_ships.size() > 0))
    {
        if (ships_coming)
        {
            --ships_coming;
        }
        else
        {
            points += (figures_for_ships.size() + wares_for_ships.size()) * 5;
        }
    }

    if(soldiers_for_ships.size() > 0 && ships_coming == 0)
        points += (soldiers_for_ships.size() * 10);

    return points;
}

// try to order any ship that might be needed and is not ordered yet
void nobHarborBuilding::OrderShip()
{
    unsigned needed = GetNeededShipsCount();
    unsigned ordered = players->getElement(player)->GetShipsToHarbor(this);

    if (ordered < needed)
    {
        // need to order more ships
        needed -= ordered;

        while (needed)
        {
            if (players->getElement(player)->OrderShip(this) == false)
            {
                break;
            }

            needed--;
        }
    }
}

/// Abgeleitete kann eine gerade erzeugte Ware ggf. sofort verwenden
/// (muss in dem Fall true zurückgeben)
bool nobHarborBuilding::UseWareAtOnce(Ware* ware, noBaseBuilding* const goal)
{
    // Evtl. muss die Ware gleich das Schiff nehmen ->
    // dann zum Schiffsreservoir hinzufügen
    Point<MapCoord> next_harbor;
    ware->RecalcRoute();
    if(ware->GetNextDir() == SHIP_DIR)
    {
        // Reduce ware count because wares don't go through the house leaving process
        // And therefore the visual count reducement
        goods.goods[ware->type]--;
        // Dann fügen wir die mal bei uns hinzu
        AddWareForShip(ware);

        return true;
    }

    return false;
}


/// Dasselbe für Menschen
bool nobHarborBuilding::UseFigureAtOnce(noFigure* fig, noRoadNode* const goal)
{
    // Evtl. muss die Ware gleich das Schiff nehmen ->
    // dann zum Schiffsreservoir hinzufügen
    Point<MapCoord> next_harbor;
    if(gwg->FindHumanPathOnRoads(this, goal, NULL, &next_harbor) == SHIP_DIR)
    {
        // Reduce figure count because figues don't go through the house leaving process
        // And therefore the visual count reducement
        goods.people[fig->GetJobType()]--;
        // Dann fügen wir die mal bei uns hinzu
        AddFigureForShip(fig, next_harbor);
        return true;
    }

    return false;
}

/// Erhält die Waren von einem Schiff und nimmt diese in den Warenbestand auf
void nobHarborBuilding::ReceiveGoodsFromShip(const std::list<noFigure*> figures, const std::list<Ware*> wares)
{
    // Menschen zur Ausgehliste hinzufügen
    for(std::list<noFigure*>::const_iterator it = figures.begin(); it != figures.end(); ++it)
    {
        if((*it)->GetJobType() == JOB_BOATCARRIER)
        {
            ++goods.people[JOB_HELPER];
            ++goods.goods[GD_BOAT];
        }
        else
            ++goods.people[(*it)->GetJobType()];

        // Wenn es kein Ziel mehr hat, sprich keinen weiteren Weg, kann es direkt hier gelagert
        // werden
        if ((*it)->HasNoGoal() || ((*it)->GetGoal() == this))
        {
            //required for expedition / exploration?
            // Brauchen wir einen Bauarbeiter für die Expedition?
            if((*it)->GetJobType() == JOB_BUILDER && expedition.active && !expedition.builder)
            {
                --goods.people[(*it)->GetJobType()]; //reduce visual count again
                nobBaseWarehouse::RemoveDependentFigure((*it));
                em->AddToKillList((*it));
                expedition.builder = true;
                CheckExpeditionReady();
            }
            // Brauchen wir einen Spähter für die Expedition?
            else if((*it)->GetJobType() == JOB_SCOUT && exploration_expedition.active && !IsExplorationExpeditionReady())
            {
                nobBaseWarehouse::RemoveDependentFigure((*it));
                em->AddToKillList((*it));
                ++exploration_expedition.scouts;
                CheckExplorationExpeditionReady();
            }
            else    //not required for expedition / exploration:
            {
                AddFigure(*it, false);
            }
        }
        else //figure has a different goal
        {
            Point<MapCoord> next_harbor = (*it)->ExamineRouteBeforeShipping();
            unsigned char next_dir = (*it)->GetDir();

            if (next_dir == 4)
            {
                AddLeavingFigure(*it);
                (*it)->ShipJourneyEnded();
            }
            else if (next_dir == SHIP_DIR)
            {
                AddFigureForShip(*it, next_harbor);
            }
            else
            {
                AddFigure(*it, false);
            }
        }
    }

    // Waren zur Warteliste hinzufügen
    for(std::list<Ware*>::const_iterator it = wares.begin(); it != wares.end(); ++it)
    {
        if((*it)->ShipJorneyEnded(this))
        {
            // Optische Warenwerte entsprechend erhöhen
            ++goods.goods[ConvertShields((*it)->type)];

            // Ware will die weitere Reise antreten, also muss sie zur Liste der rausgetragenen Waren
            // hinzugefügt werden
            waiting_wares.push_back(*it);
        }
        else
        {
            // add to harbor inventory unless ...
            // required for an expedition?
            if(expedition.active)
            {
                //board or stones and still required for the expedition?
                if(((*it)->type == GD_BOARDS && expedition.boards < BUILDING_COSTS[nation][BLD_HARBORBUILDING].boards)  || ((*it)->type == GD_STONES && expedition.stones < BUILDING_COSTS[nation][BLD_HARBORBUILDING].stones))
                {
                    // use it for expedition and reduce count
                    if((*it)->type == GD_BOARDS) ++expedition.boards;
                    else ++expedition.stones;
                    RemoveDependentWare(*it);
                    gwg->GetPlayer(player)->RemoveWare((*it));
                    //remove item
                    delete *it;
                    CheckExpeditionReady();
                }
                else
                {
                    nobBaseWarehouse::AddWare(*it);
                }
            }
            else
            {
                nobBaseWarehouse::AddWare(*it);
            }
        }
    }

    // Ggf. neues Rausgeh-Event anmelden, was notwendig ist, wenn z.B. nur Waren zur Liste hinzugefügt wurden
    AddLeavingEvent();
}

/// Storniert die Bestellung für eine bestimmte Ware, die mit einem Schiff transportiert werden soll
void nobHarborBuilding::CancelWareForShip(Ware* ware)
{
    // Ware aus der Liste entfernen
    wares_for_ships.remove(ware);
    // Ware zur Inventur hinzufügen
    // Anzahl davon wieder hochsetzen
    ++real_goods.goods[ConvertShields(ware->type)];
}

/// Bestellte Figur, die sich noch inder Warteschlange befindet, kommt nicht mehr und will rausgehauen werden
void nobHarborBuilding::CancelFigure(noFigure* figure)
{
    // Merken, ob sie entfernt wurde
    bool removed = false;
    // Figur ggf. aus der List entfernen
    for(std::list<FigureForShip>::iterator it = figures_for_ships.begin(); it != figures_for_ships.end(); ++it)
    {
        if(it->fig == figure)
        {
            figures_for_ships.erase(it);
            removed = true;
            break;
        }
    }

    // Wurde sie entfernt?
    if(removed)
    {
        // Dann zu unserem Inventar hinzufügen und anschließend vernichten
        AddFigure(figure, false);
    }
    // An Basisklasse weiterdelegieren
    else
        nobBaseWarehouse::CancelFigure(figure);

}

///Gibt verfügbare Angreifer zurück
void nobHarborBuilding::GetAttackerBuildingsForSeaIdAttack(std::vector<SeaAttackerBuilding>*buildings)
{
    std::list<nobBaseMilitary*> all_buildings;
    gwg->LookForMilitaryBuildings(all_buildings, x, y, 3);

    // Und zählen
    for(std::list<nobBaseMilitary*>::iterator it = all_buildings.begin(); it != all_buildings.end(); ++it)
    {
        if((*it)->GetGOT() != GOT_NOB_MILITARY)
            continue;

        // Liegt er auch im groben Raster und handelt es sich um den gleichen Besitzer?
        if((*it)->GetPlayer() != player || gwg->CalcDistance((*it)->GetX(), (*it)->GetY(), x, y) > BASE_ATTACKING_DISTANCE)
            continue;
        // Gebäude suchen, vielleicht schon vorhanden? Dann können wir uns den pathfinding Aufwand sparen!
        std::vector<SeaAttackerBuilding>::iterator it2 = std::find(buildings->begin(), buildings->end(), static_cast<nobMilitary*>(*it));
        // in liste gefunden?
        if(it2 != buildings->end())
        {
            // Dann zum nächsten test
            continue;
        }
        // Weg vom Hafen zum Militärgebäude berechnen
        if(!gwg->FindFreePath((*it)->GetX(), (*it)->GetY(), x, y, false, MAX_ATTACKING_RUN_DISTANCE, NULL, NULL, NULL, NULL, NULL, NULL, false))
            continue;
        //neues Gebäude mit weg und allem -> in die Liste!
        SeaAttackerBuilding sab = { static_cast<nobMilitary*>(*it), this , 0};
        buildings->push_back(sab);
    }
}
/// Gibt die Angreifergebäude zurück, die dieser Hafen für einen Seeangriff zur Verfügung stellen kann
void nobHarborBuilding::GetAttackerBuildingsForSeaAttack(std::vector<SeaAttackerBuilding> * buildings,
        const std::vector<unsigned>& defender_harbors)
{
    std::list<nobBaseMilitary*> all_buildings;
    gwg->LookForMilitaryBuildings(all_buildings, x, y, 3);

    // Und zählen
    for(std::list<nobBaseMilitary*>::iterator it = all_buildings.begin(); it != all_buildings.end(); ++it)
    {
        if((*it)->GetGOT() != GOT_NOB_MILITARY)
            continue;

        // Liegt er auch im groben Raster und handelt es sich um den gleichen Besitzer?
        if((*it)->GetPlayer() != player || gwg->CalcDistance((*it)->GetX(), (*it)->GetY(), x, y) > BASE_ATTACKING_DISTANCE)
            continue;

        // Weg vom Hafen zum Militärgebäude berechnen
        if(!gwg->FindFreePath((*it)->GetX(), (*it)->GetY(), x, y, false, MAX_ATTACKING_RUN_DISTANCE, NULL, NULL, NULL, NULL, NULL, NULL, false))
            continue;

        // Entfernung zwischen Hafen und möglichen Zielhafenpunkt ausrechnen
        unsigned min_distance = 0xffffffff;
        for(unsigned i = 0; i < defender_harbors.size(); ++i)
        {
            min_distance = min(min_distance, gwg->CalcHarborDistance(GetHarborPosID(), defender_harbors.at(i)));
        }

        // Gebäude suchen, vielleicht schon vorhanden?
        std::vector<SeaAttackerBuilding>::iterator it2 = std::find(buildings->begin(), buildings->end(),
                static_cast<nobMilitary*>(*it));
        // Noch nicht vorhanden?
        if(it2 == buildings->end())
        {
            // Dann neu hinzufügen
            SeaAttackerBuilding sab = { static_cast<nobMilitary*>(*it), this, min_distance };
            buildings->push_back(sab);
        }
        // Oder vorhanden und jetzige Distanz ist kleiner?
        else if(min_distance < it2->distance)
        {
            // Dann Distanz und betreffenden Hafen aktualisieren
            it2->distance = min_distance;
            it2->harbor = this;
        }
    }
}

/// Fügt einen Schiffs-Angreifer zum Hafen hinzu
void nobHarborBuilding::AddSeaAttacker(nofAttacker* attacker)
{
    unsigned best_distance = 0xffffffff;
    unsigned best_harbor_point = 0xffffffff;
    std::vector<unsigned> harbor_points;
    gwg->GetHarborPointsAroundMilitaryBuilding(attacker->GetAttackedGoal()->GetX(), attacker->GetAttackedGoal()->GetY(),
            &harbor_points);
    for(unsigned i = 0; i < harbor_points.size(); ++i)
    {
        unsigned tmp_distance = gwg->CalcHarborDistance(this->GetHarborPosID(), harbor_points[i]);
        if(tmp_distance < best_distance)
        {
            best_distance = tmp_distance;
            best_harbor_point = harbor_points[i];
        }
    }

    // no harbor to target (should not happen) or no target (might happen very very rarely not sure)
    if (best_harbor_point == 0xffffffff)
    {
        // notify target about noShow, notify home that soldier wont return, add to inventory
        attacker->InformTargetsAboutCancelling();
        attacker->CancelAtHomeMilitaryBuilding();
        attacker->SeaAttackFailedBeforeLaunch(); //set state, remove target & home
        AddFigure(attacker, true);
        return;
    }

    SoldierForShip sfs = { attacker, gwg->GetHarborPoint(best_harbor_point) };
    soldiers_for_ships.push_back(sfs);

    OrderShip();
    ++goods.people[attacker->GetJobType()];
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
unsigned nobHarborBuilding::CalcDistributionPoints(const GoodType type)
{

    // Ist überhaupt eine Expedition im Gang und ein entsprechender Warentyp
    if(!expedition.active || !(type == GD_BOARDS || type == GD_STONES))
        return 0;


    unsigned ordered_boards = 0, ordered_stones = 0;
    // Ermitteln, wiviele Bretter und Steine auf dem Weg zum Lagerhaus sind
	for(std::list<Ware*>::iterator it = dependent_wares.begin(); it!=dependent_wares.end(); ++it)
    {
        if((*it)->type == GD_BOARDS) ++ordered_boards;
        else if((*it)->type == GD_STONES) ++ordered_stones;
    }

    // 10000 als Basis wählen, damit man auch noch was abziehen kann
    unsigned short points = 10000;

    // Ermitteln, ob wir noch Bretter oder Steine brauchen
    if(expedition.boards + ordered_boards
            >= BUILDING_COSTS[nation][BLD_HARBORBUILDING].boards && type == GD_BOARDS)
        return 0;
    if(expedition.stones + ordered_stones
            >= BUILDING_COSTS[nation][BLD_HARBORBUILDING].stones && type == GD_STONES)
        return 0;

    // Schon bestellte Sachen wirken sich positiv aus, da wir ja so eher eine Expedition bereit haben
    if(type == GD_BOARDS)
        points += (expedition.boards + ordered_boards) * 30;
    else if(type == GD_STONES)
        points += (expedition.stones + ordered_stones) * 30;

    return points;
}

/// A ware changed its route and doesn't want to use the ship anymore
void nobHarborBuilding::WareDontWantToTravelByShip(Ware* ware)
{
    // Maybe this building is already destroyed
    if(gwg->GetGOT(x, y) != GOT_NOB_HARBORBUILDING)
        return;

    // Ware aus unserer Liste streichen
    wares_for_ships.remove(ware);
    // Will die Ware jetzt vielleicht zu uns?
    if(ware->goal == this)
    {
        // Dann hier gleich einliefern
        AddWare(ware);
        // and dont forget to reduce our visual count - addware will increase both and the ware waiting for a ship added to the visual count!
        --goods.goods[ConvertShields(ware->type)];

    }
    // Oder will sie wieder raus?
    else
    {
        waiting_wares.push_back(ware);
        AddLeavingEvent();
    }

}


/// Stellt Verteidiger zur Verfügung
nofDefender* nobHarborBuilding::ProvideDefender(nofAttacker* const attacker)
{
    // Versuchen, zunächst auf konventionelle Weise Angreifer zu bekoommen
    nofDefender* defender = nobBaseWarehouse::ProvideDefender(attacker);
    // Wenn das nicht geklappt hat und noch Soldaten in der Warteschlange für den Seeangriff sind
    // zweigen wir einfach diese ab
    if(!defender && soldiers_for_ships.size())
    {
        nofAttacker* defender_attacker = soldiers_for_ships.begin()->attacker;
        defender = new nofDefender(x, y, player, this, defender_attacker->GetRank(), attacker);
        defender_attacker->CancelSeaAttack();
        defender_attacker->Abrogate();
        defender_attacker->Destroy();
        delete defender_attacker;
        soldiers_for_ships.pop_front();
    }

    return defender;
}

/// People waiting for a ship have to examine their route if a road was destroyed
void nobHarborBuilding::ExamineShipRouteOfPeople()
{
    for(std::list<FigureForShip>::iterator it = figures_for_ships.begin();
            it != figures_for_ships.end();)
    {
        it->dest = it->fig->ExamineRouteBeforeShipping();
        unsigned char next_dir = it->fig->GetDir();

        if(next_dir == 0xff)
        {
            // No route found!
            // I.E. insert the worker in this harbor
            noFigure* fig = it->fig;
            it = figures_for_ships.erase(it);
            AddFigure(fig, false);
        }
        else if(next_dir != SHIP_DIR)
        {
            // Figure want to continue walking to its goal but not on ship anymore
            noFigure* fig = it->fig;
            it = figures_for_ships.erase(it);
            this->AddLeavingFigure(fig);
        }
        else
            // Otherwise figure want to travel by ship, do nothing!
            ++it;
    }
}

bool nobHarborBuilding::IsBeingDestroyedNow() const
{
    // check if this harbor is in the known harbors. if not, it is probably being destroyed right now.
    const std::list<nobHarborBuilding*> allHarbors = gwg->GetPlayer(player)->GetHarbors();
    if (std::find(allHarbors.begin(), allHarbors.end(), this) == allHarbors.end())
    {
        return true;
    }
    else
    {
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
// EOF
///////////////////////////////////////////////////////////////////////////////

