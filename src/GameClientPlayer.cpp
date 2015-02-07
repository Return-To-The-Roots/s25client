// $Id: GameClientPlayer.cpp 9599 2015-02-07 11:08:22Z marcus $
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
#include "GameClientPlayer.h"
#include "GameWorld.h"
#include "GameClient.h"
#include "Random.h"

#include "GameConsts.h"
#include "MilitaryConsts.h"

#include "RoadSegment.h"
#include "Ware.h"

#include "noFlag.h"
#include "noBuildingSite.h"
#include "nobUsual.h"
#include "nobMilitary.h"
#include "nofFlagWorker.h"
#include "nofCarrier.h"
#include "noShip.h"
#include "nobHarborBuilding.h"
#include "nofTradeLeader.h"

#include "GameInterface.h"

#include "SerializedGameData.h"
#include "GameMessages.h"

#include <stdint.h>
#include <limits>

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Standardbelegung der Transportreihenfolge festlegen
const unsigned char STD_TRANSPORT[35] =
{
    2, 12, 12, 12, 12, 12, 12, 12, 12, 12, 10, 10, 12, 12, 12, 13, 1, 3, 11, 11, 11, 1, 9, 7, 8, 1, 1, 11, 0, 4, 5, 6, 11, 11, 1
};

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p GameClientPlayer.
 *
 *  @author OLiver
 */
GameClientPlayer::GameClientPlayer(const unsigned playerid) : GamePlayerInfo(playerid), build_order(31), military_settings(MILITARY_SETTINGS_COUNT), tools_settings(12, 0)
{
    // Erstmal kein HQ (leerer Spieler) wie das bei manchen Karten der Fall ist
    hqy = hqx = 0xFFFF;

    for (unsigned i = 0; i < BUILDING_TYPES_COUNT; ++i)
    {
        building_enabled[i] = true;
    }

    // Verteilung mit Standardwerten füllen bei Waren mit nur einem Ziel (wie z.B. Mehl, Holz...)
    distribution[GD_FLOUR].client_buildings.push_back(BLD_BAKERY);
    distribution[GD_GOLD].client_buildings.push_back(BLD_MINT);
    distribution[GD_IRONORE].client_buildings.push_back(BLD_IRONSMELTER);
    distribution[GD_HAM].client_buildings.push_back(BLD_SLAUGHTERHOUSE);
    distribution[GD_STONES].client_buildings.push_back(BLD_HEADQUARTERS); // BLD_HEADQUARTERS = Baustellen!
    distribution[GD_STONES].client_buildings.push_back(BLD_CATAPULT);


    // Waren mit mehreren möglichen Zielen erstmal nullen, kann dann im Fenster eingestellt werden
    for(unsigned char i = 0; i < WARE_TYPES_COUNT; ++i)
    {
        memset(distribution[i].percent_buildings, 0, 40 * sizeof(unsigned char));
        distribution[i].selected_goal = 0;
    }

    // Standardverteilung der Waren
    GAMECLIENT.visual_settings.distribution[0] = distribution[GD_FISH].percent_buildings[BLD_GRANITEMINE] = 3;
    GAMECLIENT.visual_settings.distribution[1] = distribution[GD_FISH].percent_buildings[BLD_COALMINE] = 5;
    GAMECLIENT.visual_settings.distribution[2] = distribution[GD_FISH].percent_buildings[BLD_IRONMINE] = 7;
    GAMECLIENT.visual_settings.distribution[3] = distribution[GD_FISH].percent_buildings[BLD_GOLDMINE] = 10;

    GAMECLIENT.visual_settings.distribution[4] = distribution[GD_GRAIN].percent_buildings[BLD_MILL] = 5;
    GAMECLIENT.visual_settings.distribution[5] = distribution[GD_GRAIN].percent_buildings[BLD_PIGFARM] = 3;
    GAMECLIENT.visual_settings.distribution[6] = distribution[GD_GRAIN].percent_buildings[BLD_DONKEYBREEDER] = 2;
    GAMECLIENT.visual_settings.distribution[7] = distribution[GD_GRAIN].percent_buildings[BLD_BREWERY] = 3;
    GAMECLIENT.visual_settings.distribution[8] = distribution[GD_GRAIN].percent_buildings[BLD_CHARBURNER] = 3;

    GAMECLIENT.visual_settings.distribution[9] = distribution[GD_IRON].percent_buildings[BLD_ARMORY] = 8;
    GAMECLIENT.visual_settings.distribution[10] = distribution[GD_IRON].percent_buildings[BLD_METALWORKS] = 4;

    GAMECLIENT.visual_settings.distribution[11] = distribution[GD_COAL].percent_buildings[BLD_ARMORY] = 8;
    GAMECLIENT.visual_settings.distribution[12] = distribution[GD_COAL].percent_buildings[BLD_IRONSMELTER] = 7;
    GAMECLIENT.visual_settings.distribution[13] = distribution[GD_COAL].percent_buildings[BLD_MINT] = 10;

    GAMECLIENT.visual_settings.distribution[14] = distribution[GD_WOOD].percent_buildings[BLD_SAWMILL] = 8;
    GAMECLIENT.visual_settings.distribution[15] = distribution[GD_WOOD].percent_buildings[BLD_CHARBURNER] = 3;

    GAMECLIENT.visual_settings.distribution[16] = distribution[GD_BOARDS].percent_buildings[BLD_HEADQUARTERS] = 10;
    GAMECLIENT.visual_settings.distribution[17] = distribution[GD_BOARDS].percent_buildings[BLD_METALWORKS] = 4;
    GAMECLIENT.visual_settings.distribution[18] = distribution[GD_BOARDS].percent_buildings[BLD_SHIPYARD] = 2;

    GAMECLIENT.visual_settings.distribution[19] = distribution[GD_WATER].percent_buildings[BLD_BAKERY] = 6;
    GAMECLIENT.visual_settings.distribution[20] = distribution[GD_WATER].percent_buildings[BLD_BREWERY] = 3;
    GAMECLIENT.visual_settings.distribution[21] = distribution[GD_WATER].percent_buildings[BLD_PIGFARM] = 2;
    GAMECLIENT.visual_settings.distribution[22] = distribution[GD_WATER].percent_buildings[BLD_DONKEYBREEDER] = 3;

    RecalcDistribution();

    GAMECLIENT.visual_settings.order_type = order_type = 0;

    // Baureihenfolge füllen (0 ist das HQ!)
    for(unsigned char i = 1, j = 0; i < 40; ++i)
    {
        // Diese Ids sind noch nicht besetzt
        if(i == 3 || (i >= 5 && i <= 8) || i == 15 || i == 27 || i == 30)
            continue;

        GAMECLIENT.visual_settings.build_order[j] = build_order[j] = i;
        ++j;
    }

    // Transportreihenfolge festlegen
    memcpy(transport, STD_TRANSPORT, 35 * sizeof(unsigned char));

    GAMECLIENT.visual_settings.transport_order[0] = STD_TRANSPORT[GD_COINS];
    GAMECLIENT.visual_settings.transport_order[1] = STD_TRANSPORT[GD_SWORD];
    GAMECLIENT.visual_settings.transport_order[2] = STD_TRANSPORT[GD_BEER];
    GAMECLIENT.visual_settings.transport_order[3] = STD_TRANSPORT[GD_IRON];
    GAMECLIENT.visual_settings.transport_order[4] = STD_TRANSPORT[GD_GOLD];
    GAMECLIENT.visual_settings.transport_order[5] = STD_TRANSPORT[GD_IRONORE];
    GAMECLIENT.visual_settings.transport_order[6] = STD_TRANSPORT[GD_COAL];
    GAMECLIENT.visual_settings.transport_order[7] = STD_TRANSPORT[GD_BOARDS];
    GAMECLIENT.visual_settings.transport_order[8] = STD_TRANSPORT[GD_STONES];
    GAMECLIENT.visual_settings.transport_order[9] = STD_TRANSPORT[GD_WOOD];
    GAMECLIENT.visual_settings.transport_order[10] = STD_TRANSPORT[GD_WATER];
    GAMECLIENT.visual_settings.transport_order[11] = STD_TRANSPORT[GD_FISH];
    GAMECLIENT.visual_settings.transport_order[12] = STD_TRANSPORT[GD_HAMMER];
    GAMECLIENT.visual_settings.transport_order[13] = STD_TRANSPORT[GD_BOAT];

    // Militär- und Werkzeugeinstellungen
    military_settings[0] = 10;
    military_settings[1] = 3;
    military_settings[2] = 5;
    military_settings[3] = 3;
    military_settings[4] = 2;
    military_settings[5] = 4;
    military_settings[6] = MILITARY_SETTINGS_SCALE[6];
    military_settings[7] = MILITARY_SETTINGS_SCALE[7];
    GAMECLIENT.visual_settings.military_settings = military_settings;
    GAMECLIENT.visual_settings.tools_settings = tools_settings;

    // qx:tools
    for (unsigned i = 0; i < TOOL_COUNT; ++i)
        tools_ordered[i] = 0;
    for (unsigned i = 0; i < TOOL_COUNT; ++i)
        tools_ordered_delta[i] = 0;

    // Standardeinstellungen kopieren
    GAMECLIENT.default_settings = GAMECLIENT.visual_settings;

    defenders_pos = 0;
    for(unsigned i = 0; i < 5; ++i)
        defenders[i] = true;

    is_lagging = true;

    // Inventur nullen
    global_inventory.clear();

    // Statistiken mit 0en füllen
    memset(&statistic[STAT_15M], 0, sizeof(statistic[STAT_15M]));
    memset(&statistic[STAT_1H], 0, sizeof(statistic[STAT_1H]));
    memset(&statistic[STAT_4H], 0, sizeof(statistic[STAT_4H]));
    memset(&statistic[STAT_16H], 0, sizeof(statistic[STAT_16H]));
    memset(&statisticCurrentData, 0, sizeof(statisticCurrentData));
    memset(&statisticCurrentMerchandiseData, 0, sizeof(statisticCurrentMerchandiseData));

    // Initial kein Notfallprogramm
    emergency = false;
}

void GameClientPlayer::Serialize(SerializedGameData* sgd)
{
    // PlayerStatus speichern, ehemalig
    sgd->PushUnsignedChar(static_cast<unsigned char>(ps));

    // Nur richtige Spieler serialisieren
    if(!(ps == PS_OCCUPIED || ps == PS_KI))
        return;

    sgd->PushObjectList(warehouses, false);
    sgd->PushObjectList(harbors, true);

    //sgd->PushObjectList(unoccupied_roads,true);
    sgd->PushObjectList(roads, true);

    sgd->PushUnsignedInt(jobs_wanted.size());
    for(std::list<JobNeeded>::iterator it = jobs_wanted.begin(); it != jobs_wanted.end(); ++it)
    {
        sgd->PushUnsignedChar(it->job);
        sgd->PushObject(it->workplace, false);
    }

    for(unsigned i = 0; i < 30; ++i)
        sgd->PushObjectList(buildings[i], true);

    sgd->PushObjectList(building_sites, true);

    sgd->PushObjectList(military_buildings, true);

    sgd->PushObjectList(ware_list, true);

    sgd->PushObjectList(flagworkers, false);

    sgd->PushObjectVector(ships, true);

    for(unsigned i = 0; i < 5; ++i)
        sgd->PushBool(defenders[i]);
    sgd->PushUnsignedShort(defenders_pos);

    sgd->PushUnsignedShort(hqx);
    sgd->PushUnsignedShort(hqy);

    for(unsigned i = 0; i < WARE_TYPES_COUNT; ++i)
    {
        for (unsigned bldType = 0; bldType < BUILDING_TYPES_COUNT; ++bldType)
        {
            sgd->PushUnsignedChar(distribution[i].percent_buildings[bldType]);
        }
        sgd->PushUnsignedInt(distribution[i].client_buildings.size());
        for(std::list<BuildingType>::iterator it = distribution[i].client_buildings.begin(); it != distribution[i].client_buildings.end(); ++it)
            sgd->PushUnsignedChar(*it);
        sgd->PushUnsignedInt(unsigned(distribution[i].goals.size()));
        for(unsigned z = 0; z < distribution[i].goals.size(); ++z)
            sgd->PushUnsignedChar(distribution[i].goals[z]);
        sgd->PushUnsignedInt(distribution[i].selected_goal);
    }

    sgd->PushUnsignedChar(order_type);

    for(unsigned i = 0; i < 31; ++i)
        sgd->PushUnsignedChar(build_order[i]);

    sgd->PushRawData(transport, WARE_TYPES_COUNT);

    for(unsigned i = 0; i < MILITARY_SETTINGS_COUNT; ++i)
        sgd->PushUnsignedChar(military_settings[i]);

    for(unsigned i = 0; i < 12; ++i)
        sgd->PushUnsignedChar(tools_settings[i]);

    //qx:tools
    for (unsigned i = 0; i < TOOL_COUNT; ++i)
        sgd->PushUnsignedChar(tools_ordered[i]);

    for(unsigned i = 0; i < WARE_TYPES_COUNT; ++i)
        sgd->PushUnsignedInt(global_inventory.goods[i]);
    for(unsigned i = 0; i < JOB_TYPES_COUNT; ++i)
        sgd->PushUnsignedInt(global_inventory.people[i]);

    // für Statistik
    for (unsigned i = 0; i < STAT_TIME_COUNT; ++i)
    {
        // normale Statistik
        for (unsigned j = 0; j < STAT_TYPE_COUNT; ++j)
            for (unsigned k = 0; k < STAT_STEP_COUNT; ++k)
                sgd->PushUnsignedInt(statistic[i].data[j][k]);

        // Warenstatistik
        for (unsigned j = 0; j < STAT_MERCHANDISE_TYPE_COUNT; ++j)
            for (unsigned k = 0; k < STAT_STEP_COUNT; ++k)
                sgd->PushUnsignedShort(statistic[i].merchandiseData[j][k]);

        sgd->PushUnsignedShort(statistic[i].currentIndex);
        sgd->PushUnsignedShort(statistic[i].counter);
    }
    for (unsigned i = 0; i < STAT_TYPE_COUNT; ++i)
        sgd->PushUnsignedInt(statisticCurrentData[i]);

    for (unsigned i = 0; i < STAT_MERCHANDISE_TYPE_COUNT; ++i)
        sgd->PushUnsignedShort(statisticCurrentMerchandiseData[i]);

    // Serialize Pacts:
    for (unsigned i = 0; i < MAX_PLAYERS; ++i)
    {
        for (unsigned u = 0; u < PACTS_COUNT; ++u)
        {
            pacts[i][u].Serialize(sgd);
        }
    }

    sgd->PushBool(emergency);
}

void GameClientPlayer::Deserialize(SerializedGameData* sgd)
{
    // Ehemaligen PS auslesen
    PlayerState origin_ps = PlayerState(sgd->PopUnsignedChar());
    // Nur richtige Spieler serialisieren
    if(!(origin_ps == PS_OCCUPIED || origin_ps == PS_KI))
        return;

    sgd->PopObjectList(warehouses, GOT_UNKNOWN);
    sgd->PopObjectList(harbors, GOT_NOB_HARBORBUILDING);

    //sgd->PopObjectList(unoccupied_roads,GOT_ROADSEGMENT);
    sgd->PopObjectList(roads, GOT_ROADSEGMENT);

    unsigned list_size = sgd->PopUnsignedInt();
    for(unsigned i = 0; i < list_size; ++i)
    {
        JobNeeded nj;
        nj.job = Job(sgd->PopUnsignedChar());
        nj.workplace = sgd->PopObject<noRoadNode>(
                           GOT_UNKNOWN);
        jobs_wanted.push_back(nj);

    }

    for(unsigned i = 0; i < 30; ++i)
        sgd->PopObjectList(buildings[i], GOT_NOB_USUAL);

    sgd->PopObjectList(building_sites, GOT_BUILDINGSITE);

    sgd->PopObjectList(military_buildings, GOT_NOB_MILITARY);

    sgd->PopObjectList(ware_list, GOT_WARE);

    sgd->PopObjectList(flagworkers, GOT_UNKNOWN);

    sgd->PopObjectVector(ships, GOT_SHIP);

    for(unsigned i = 0; i < 5; ++i)
        defenders[i] = sgd->PopBool();
    defenders_pos = sgd->PopUnsignedShort();

    hqx = sgd->PopUnsignedShort();
    hqy = sgd->PopUnsignedShort();

    for(unsigned i = 0; i < WARE_TYPES_COUNT; ++i)
    {
        for (unsigned bldType = 0; bldType < BUILDING_TYPES_COUNT; ++bldType)
        {
            distribution[i].percent_buildings[bldType] = sgd->PopUnsignedChar();
        }
        list_size = sgd->PopUnsignedInt();
        for(unsigned z = 0; z < list_size; ++z)
            distribution[i].client_buildings.push_back(BuildingType(sgd->PopUnsignedChar()));
        unsigned goal_count = sgd->PopUnsignedInt();
        distribution[i].goals.resize(goal_count);
        for(unsigned z = 0; z < goal_count; ++z)
            distribution[i].goals[z] = sgd->PopUnsignedChar();
        distribution[i].selected_goal = sgd->PopUnsignedInt();
    }

    order_type = sgd->PopUnsignedChar();

    for(unsigned i = 0; i < 31; ++i)
        build_order[i] = sgd->PopUnsignedChar();

    char str[256] = "";
    for(unsigned char i = 0; i < 31; ++i)
    {
        char tmp[256];
        sprintf(tmp, "%u ", i);
        strcat(str, tmp);
    }
    strcat(str, "\n");
    puts(str);

    sgd->PopRawData(transport, WARE_TYPES_COUNT);

    for(unsigned i = 0; i < MILITARY_SETTINGS_COUNT; ++i)
        military_settings[i] = sgd->PopUnsignedChar();

    for(unsigned i = 0; i < 12; ++i)
        tools_settings[i] = sgd->PopUnsignedChar();

    // qx:tools
    for (unsigned i = 0; i < TOOL_COUNT; ++i)
        tools_ordered[i] = sgd->PopUnsignedChar();
    for (unsigned i = 0; i < TOOL_COUNT; ++i)
        tools_ordered_delta[i] = 0;

    for(unsigned i = 0; i < WARE_TYPES_COUNT; ++i)
        global_inventory.goods[i] = sgd->PopUnsignedInt();
    for(unsigned i = 0; i < JOB_TYPES_COUNT; ++i)
        global_inventory.people[i] = sgd->PopUnsignedInt();

    // Visuelle Einstellungen festlegen

    // für Statistik
    for (unsigned i = 0; i < STAT_TIME_COUNT; ++i)
    {
        // normale Statistik
        for (unsigned j = 0; j < STAT_TYPE_COUNT; ++j)
            for (unsigned k = 0; k < STAT_STEP_COUNT; ++k)
                statistic[i].data[j][k] = sgd->PopUnsignedInt();

        // Warenstatistik
        for (unsigned j = 0; j < STAT_MERCHANDISE_TYPE_COUNT; ++j)
            for (unsigned k = 0; k < STAT_STEP_COUNT; ++k)
                statistic[i].merchandiseData[j][k] = sgd->PopUnsignedShort();

        statistic[i].currentIndex = sgd->PopUnsignedShort();
        statistic[i].counter = sgd->PopUnsignedShort();
    }
    for (unsigned i = 0; i < STAT_TYPE_COUNT; ++i)
        statisticCurrentData[i] = sgd->PopUnsignedInt();

    for (unsigned i = 0; i < STAT_MERCHANDISE_TYPE_COUNT; ++i)
        statisticCurrentMerchandiseData[i] = sgd->PopUnsignedShort();

    // Deserialize Pacts:
    for (unsigned i = 0; i < MAX_PLAYERS; ++i)
    {
        for (unsigned u = 0; u < PACTS_COUNT; ++u)
        {
            pacts[i][u] = GameClientPlayer::Pact(sgd);
        }
    }

    emergency = sgd->PopBool();
}

void GameClientPlayer::SwapPlayer(GameClientPlayer& two)
{
    GamePlayerInfo::SwapPlayer(two);

    Swap(this->is_lagging, two.is_lagging);
    Swap(this->gc_queue, two.gc_queue);
}

nobBaseWarehouse* GameClientPlayer::FindWarehouse(const noRoadNode* const start, bool (*IsWarehouseGood)(nobBaseWarehouse*, const void*),
        const RoadSegment* const forbidden, const bool to_wh, const void* param, const bool use_boat_roads, unsigned* const length)
{
    nobBaseWarehouse* best = 0;

//  unsigned char path = 0xFF, tpath = 0xFF;
    unsigned tlength = 0xFFFFFFFF, best_length = 0xFFFFFFFF;

    for(std::list<nobBaseWarehouse*>::iterator w = warehouses.begin(); w != warehouses.end(); ++w)
    {
        // Lagerhaus geeignet?
        if(IsWarehouseGood(*w, param))
        {
			//now check if there is at least a chance that the next wh is closer than current best because pathfinding takes time
			if(gwg->CalcDistance(start->GetX(),start->GetY(),(*w)->GetX(),(*w)->GetY()) > best_length)
				continue;
            // Bei der erlaubten Benutzung von BootsstraÃen Waren-Pathfinding benutzen wenns zu nem Lagerhaus gehn soll start <-> ziel tauschen bei der wegfindung
            if(gwg->FindPathOnRoads(to_wh ? start : *w, to_wh ? *w : start, use_boat_roads, &tlength, NULL, NULL, forbidden, true, best_length))
            {
                if(tlength < best_length || !best)
                {
//                  path = tpath;
                    best_length = tlength;
                    best = (*w);
                }
            }
        }
    }

    if(length)
        *length = best_length;

    return best;
}

void GameClientPlayer::NewRoad(RoadSegment* const rs)
{
    // Zu den StraÃen hinzufgen, da's ja ne neue ist
    roads.push_back(rs);

    // Alle StraÃen müssen nun gucken, ob sie einen Weg zu einem Warehouse finden
    FindWarehouseForAllRoads();

    // Alle StraÃen müssen gucken, ob sie einen Esel bekommen können
    for(std::list<RoadSegment*>::iterator it = roads.begin(); it != roads.end(); ++it)
        (*it)->TryGetDonkey();

    // Alle Arbeitsplätze müssen nun gucken, ob sie einen Weg zu einem Lagerhaus mit entsprechender Arbeitskraft finden
    FindWarehouseForAllJobs(JOB_NOTHING);

    // Alle Baustellen müssen nun gucken, ob sie ihr benötigtes Baumaterial bekommen (evtl war vorher die StraÃe zum Lagerhaus unterbrochen
    FindMaterialForBuildingSites();

    // Alle Lost-Wares müssen gucken, ob sie ein Lagerhaus finden
    FindClientForLostWares();

    // Alle Militärgebäude müssen ihre Truppen überprüfen und können nun ggf. neue bestellen
    // und müssen prüfen, ob sie evtl Gold bekommen
    for(std::list<nobMilitary*>::iterator it = military_buildings.begin(); it != military_buildings.end(); ++it)
    {
        (*it)->RegulateTroops();
        (*it)->SearchCoins();
    }
}



void GameClientPlayer::FindClientForLostWares()
{
    // Alle Lost-Wares müssen gucken, ob sie ein Lagerhaus finden
    for(std::list<Ware*>::iterator it = ware_list.begin(); it != ware_list.end(); ++it)
    {
        if((*it)->IsLostWare())
        {
            (*it)->FindRouteToWarehouse();
        }
    }
}

void GameClientPlayer::RoadDestroyed()
{
    // Alle Waren, die an Flagge liegen und in Lagerhäusern, müssen gucken, ob sie ihr Ziel noch erreichen können, jetzt wo eine StraÃe fehlt
    for(std::list<Ware*>::iterator it = ware_list.begin(); it != ware_list.end(); )
    {
        if((*it)->LieAtFlag()) // Liegt die Flagge an einer Flagge, muss ihr Weg neu berechnet werden
        {
			unsigned char last_next_dir = (*it)->GetNextDir();
			(*it)->RecalcRoute();			
			//special case: ware was lost some time ago and the new goal is at this flag and not a warehouse,hq,harbor and the "flip-route" picked so a carrier would pick up the ware carry it away from goal then back and drop
			//it off at the goal was just destroyed? -> try to pick another flip route or tell the goal about failure.
			if((*it)->goal && (*it)->GetNextDir()==1 && (*it)->GetLocation()->GetX()==(*it)->goal->GetFlag()->GetX() && (*it)->GetLocation()->GetY()==(*it)->goal->GetFlag()->GetY() && (((*it)->goal->GetBuildingType()!=BLD_STOREHOUSE && (*it)->goal->GetBuildingType()!=BLD_HEADQUARTERS && (*it)->goal->GetBuildingType()!=BLD_HARBORBUILDING) || (*it)->goal->GetType()==NOP_BUILDINGSITE))
			{
				//LOG.lprintf("road destroyed special at %i,%i gf: %u \n", (*it)->GetLocation()->GetX(),(*it)->GetLocation()->GetY(),GAMECLIENT.GetGFNumber());
				unsigned gotfliproute=1;
				for(unsigned i=2;i<7;i++)
				{
					if((*it)->GetLocation()->routes[i%6])
					{
						gotfliproute=i;
						break;
					}
				}
				if(gotfliproute!=1)
				{
					(*it)->SetNextDir(gotfliproute%6);
				}
				else //no route to goal -> notify goal, try to send ware to a warehouse and if that fails as well set goal = 0 to mark this ware as lost
				{
					(*it)->NotifyGoalAboutLostWare();
					nobBaseWarehouse* wh = gwg->GetPlayer((*it)->GetLocation()->GetPlayer())->FindWarehouse((*it)->GetLocation(), FW::Condition_StoreWare, 0, true, &(*it)->type, true);
					if(wh)
					{
						(*it)->goal = wh;
						(*it)->SetNextDir(gwg->FindPathForWareOnRoads((*it)->GetLocation(), (*it)->goal, NULL, &(*it)->next_harbor));
						wh->TakeWare((*it));
					}
					else
					{
						(*it)->goal=0;
					}
				}
			}
			//end of special case

			// notify carriers/flags about news if there are any
			if((*it)->GetNextDir() != 0xFF && (*it)->GetNextDir()!=last_next_dir)
				(*it)->GetLocation()->routes[(*it)->GetNextDir()]->AddWareJob((*it)->GetLocation());
			//if the next direction changed: notify current flag that transport in the old direction might not longer be required
			if((*it)->GetNextDir()!=last_next_dir)
				(*it)->RemoveWareJobForCurrentDir(last_next_dir);
		}
        else if((*it)->LieInWarehouse())
        {
            if(!(*it)->FindRouteFromWarehouse())
            {
                Ware* ware = *it;

                // Ware aus der Warteliste des Lagerhauses entfernen
                static_cast<nobBaseWarehouse*>((*it)->GetLocation())->CancelWare(ware);
                // Das Ziel wird nun nich mehr beliefert
                ware->NotifyGoalAboutLostWare();
                // Ware aus der Liste raus
                it = ware_list.erase(it);
                continue;
            }
        }
        else if((*it)->LieInHarborBuilding())
        {
            // Weg neu berechnen
            (*it)->RecalcRoute();
        }

        ++it;
    }

    // Alle Häfen müssen ihre Figuren den Weg überprüfen lassen
    for(std::list<nobHarborBuilding*>::iterator it = harbors.begin();
            it != harbors.end(); ++it)
    {
        (*it)->ExamineShipRouteOfPeople();
    }
}

/// Hafen zur Warenhausliste hinzufügen
void GameClientPlayer::AddHarbor(nobHarborBuilding* hb)
{
    harbors.push_back(hb);

    // Schiff durchgehen und denen Bescheid sagen
    for(unsigned i = 0; i < ships.size(); ++i)
        ships[i]->NewHarborBuilt(hb);
}

bool GameClientPlayer::FindCarrierForRoad(RoadSegment* rs)
{
    unsigned length[2];
    nobBaseWarehouse* best[2];

    // Braucht der ein Boot?
    if(rs->GetRoadType() == RoadSegment::RT_BOAT)
    {
        // dann braucht man Träger UND Boot
        FW::Param_WareAndJob p = { {GD_BOAT, 1}, {JOB_HELPER, 1} };
        best[0] = FindWarehouse(rs->GetF1(), FW::Condition_WareAndJob, rs, 0, &p, false, &length[0]);
        // 2. Flagge des Weges
        best[1] = FindWarehouse(rs->GetF2(), FW::Condition_WareAndJob, rs, 0, &p, false, &length[1]);
    }
    else
    {
        // 1. Flagge des Weges
        FW::Param_Job p = { JOB_HELPER, 1 };
        best[0] = FindWarehouse(rs->GetF1(), FW::Condition_Job, rs, 0, &p, false, &length[0]);
        // 2. Flagge des Weges
        best[1] = FindWarehouse(rs->GetF2(), FW::Condition_Job, rs, 0, &p, false, &length[1]);
    }

    // überhaupt nen Weg gefunden?
    // Welche Flagge benutzen?
    if(best[0] && (length[0] < length[1]))
        best[0]->OrderCarrier(rs->GetF1(), rs);
    else if(best[1])
        best[1]->OrderCarrier(rs->GetF2(), rs);
    else
        return false;

    return true;
}

void GameClientPlayer::RecalcDistribution()
{
    RecalcDistributionOfWare(GD_FISH);
    RecalcDistributionOfWare(GD_GRAIN);
    RecalcDistributionOfWare(GD_IRON);
    RecalcDistributionOfWare(GD_COAL);
    RecalcDistributionOfWare(GD_WOOD);
    RecalcDistributionOfWare(GD_BOARDS);
    RecalcDistributionOfWare(GD_WATER);
}

void GameClientPlayer::RecalcDistributionOfWare(const GoodType ware)
{
    // Punktesystem zur Verteilung, in der Liste alle Gebäude sammeln, die die Ware wollen
    distribution[ware].client_buildings.clear();

    // 1. Anteile der einzelnen Waren ausrechnen

    // Liste von Gebäudetypen, die die Waren wollen
    std::list<BuildingWhichWantWare> bwww_list;

    unsigned goal_count = 0;

    for(unsigned char i = 0; i < 40; ++i)
    {
        if(distribution[ware].percent_buildings[i])
        {
            distribution[ware].client_buildings.push_back(static_cast<BuildingType>(i));
            goal_count += distribution[ware].percent_buildings[i];
            BuildingWhichWantWare bwww = {distribution[ware].percent_buildings[i], i};
            bwww_list.push_back(bwww);
        }
    }

    // TODO: evtl noch die counts miteinander kürzen (ggt berechnen)

    // Array für die Gebäudtypen erstellen

    distribution[ware].goals.clear();
    distribution[ware].goals.resize(goal_count);

//  LOG.lprintf("goal_count[%u] = %u\n", ware, goal_count);

    unsigned pos = 0;

    // just drop them in the list, the distribution will be handled by going through this list using a prime as step (see GameClientPlayer::FindClientForWare)
    for(std::list<BuildingWhichWantWare>::iterator it = bwww_list.begin(); it != bwww_list.end(); ++it)
    {
        for (unsigned char i = 0; i < it->count; ++i)
        {
            distribution[ware].goals[pos++] = it->building;
        }

//      LOG.lprintf("\t[%u] = %u\n", it->building, it->count);
    }

    /*
        for(std::list<BuildingWhichWantWare>::iterator it = bwww_list.begin();it!=bwww_list.end();++it)
        {
            it->count = 0;
        }

        distribution[ware].selected_goal = 0;

        LOG.lprintf("distribution\n");
        for (unsigned i = 0; i < distribution[ware].goals.size(); i++)
        {
            distribution[ware].selected_goal = (distribution[ware].selected_goal + 907) % unsigned(distribution[ware].goals.size());

            for(std::list<BuildingWhichWantWare>::iterator it = bwww_list.begin();it!=bwww_list.end();++it)
            {
                if (it->building == distribution[ware].goals[distribution[ware].selected_goal])
                {
                    it->count++;
                }
            }
            LOG.lprintf("\t[%u] %u\n", i, distribution[ware].goals[distribution[ware].selected_goal]);
        }

        LOG.lprintf("TEST\n");
        for(std::list<BuildingWhichWantWare>::iterator it = bwww_list.begin();it!=bwww_list.end();++it)
        {

            LOG.lprintf("\t[%u] = %u\n", it->building, it->count);
        }*/

    // Und ordentlich schütteln ;)
    //RandomShuffle(distribution[ware].goals,distribution[ware].goal_count);


    //for(unsigned char i = 0;i<distribution[ware].goal_count;++i)
    //  LOG.lprintf("%u ",distribution[ware].goals[i]);
    //LOG.lprintf("\n");


    // Alles fängt wieder von vorne an...
    distribution[ware].selected_goal = 0;
}

void GameClientPlayer::FindWarehouseForAllRoads()
{
    for(std::list<RoadSegment*>::iterator it = roads.begin(); it != roads.end(); ++it)
    {
        if(!(*it)->hasCarrier(0))
            FindCarrierForRoad(*it);
    }
}

void GameClientPlayer::FindMaterialForBuildingSites()
{
    for(std::list<noBuildingSite*>::iterator it = building_sites.begin(); it != building_sites.end(); ++it)
        (*it)->OrderConstructionMaterial();
}

void GameClientPlayer::AddJobWanted(const Job job, noRoadNode* workplace)
{
    // Und gleich suchen
    if(!FindWarehouseForJob(job, workplace))
    {
        JobNeeded jn = { job, workplace };
        jobs_wanted.push_back(jn);
    }
}

void GameClientPlayer::JobNotWanted(noRoadNode* workplace,bool all)
{
    for(std::list<JobNeeded>::iterator it = jobs_wanted.begin(); it != jobs_wanted.end(); ++it)
    {
        if(it->workplace == workplace)
        {
            it = jobs_wanted.erase(it);
			if(!all || it==jobs_wanted.end())
				return;
        }
    }
}

void GameClientPlayer::OneJobNotWanted(const Job job, noRoadNode* workplace)
{
    for(std::list<JobNeeded>::iterator it = jobs_wanted.begin(); it != jobs_wanted.end(); ++it)
    {
        if(it->workplace == workplace && it->job == job)
        {
            jobs_wanted.erase(it);
            return;
        }
    }
}

bool GameClientPlayer::FindWarehouseForJob(const Job job, noRoadNode* goal)
{
    FW::Param_Job p = { job, 1 };
    nobBaseWarehouse* wh = FindWarehouse(goal, FW::Condition_Job, 0, false, &p, false);

    if(wh)
    {
        // Es wurde ein Lagerhaus gefunden, wo es den geforderten Beruf gibt, also den Typen zur Arbeit rufen
        wh->OrderJob(job, goal, true);
        return true;
    }

    return false;
}

void GameClientPlayer::FindWarehouseForAllJobs(const Job job)
{
    for(std::list<JobNeeded>::iterator it = jobs_wanted.begin(); it != jobs_wanted.end(); )
    {
        if(job == JOB_NOTHING || it->job == job)
        {
            if(FindWarehouseForJob(it->job, it->workplace))
                it = jobs_wanted.erase(it);
            else
                ++it;
        }
        else
            ++it;
    }
}

Ware* GameClientPlayer::OrderWare(const GoodType ware, noBaseBuilding* goal)
{
    /// Gibt es ein Lagerhaus mit dieser Ware?
    FW::Param_Ware p = { ware, 1 };
    nobBaseWarehouse* wh = FindWarehouse(goal, FW::Condition_Ware, 0, true, &p, true);

    if(wh)
    {
        // Prüfe ob Notfallprogramm aktiv
        if (!emergency)
            return wh->OrderWare(ware, goal);
        else
        {
            // Wenn Notfallprogramm aktiv nur an Holzfäller und Sägewerke Bretter/Steine liefern
            if ((ware != GD_BOARDS && ware != GD_STONES) || goal->GetBuildingType() == BLD_WOODCUTTER || goal->GetBuildingType() == BLD_SAWMILL)
                return wh->OrderWare(ware, goal);
            else
                return 0;
        }
    }	
    else //no warehouse can deliver the ware -> check all our wares for lost wares that might match the order
	{
		unsigned tlength = 0xFFFFFFFF, best_length = 0xFFFFFFFF;
		Ware* tempbest=0;
		for(std::list<Ware*>::iterator it = ware_list.begin(); it != ware_list.end(); ++it)
		{
			if((*it)->IsLostWare() && (*it)->type==ware)
			{ //got a lost ware with a road to goal -> find best
				if(tlength=(*it)->CheckNewGoalForLostWare(goal)<best_length)
				{
					best_length=tlength;
					tempbest=(*it);	
				}
			}
		}
		if(tempbest)
		{
			tempbest->SetNewGoalForLostWare(goal);
			return tempbest;
		}
	}
	return 0;
}

nofCarrier* GameClientPlayer::OrderDonkey(RoadSegment* road)
{
    unsigned length[2];
    nobBaseWarehouse* best[2];

    // 1. Flagge des Weges
    FW::Param_Job p = { JOB_PACKDONKEY, 1 };
    best[0] = FindWarehouse(road->GetF1(), FW::Condition_Job, road, 0, &p, false, &length[0]);
    // 2. Flagge des Weges
    best[1] = FindWarehouse(road->GetF2(), FW::Condition_Job, road, 0, &p, false, &length[1]);

    // überhaupt nen Weg gefunden?
    // Welche Flagge benutzen?
    if(best[0] && (length[0] < length[1]))
        return best[0]->OrderDonkey(road, road->GetF1());
    else if(best[1])
        return best[1]->OrderDonkey(road, road->GetF2());
    else
        return 0;
}

RoadSegment* GameClientPlayer::FindRoadForDonkey(noRoadNode* start, noRoadNode** goal)
{
    // Bisher höchste Trägerproduktivität und die entsprechende StraÃe dazu
    unsigned best_productivity = 0;
    RoadSegment* best_road = 0;
    // Beste Flagge dieser StraÃe
    *goal = 0;

    for(std::list<RoadSegment*>::iterator it = roads.begin(); it != roads.end(); ++it)
    {
        // Braucht die StraÃe einen Esel?
        if((*it)->NeedDonkey())
        {
            // Beste Flagge von diesem Weg, und beste Wegstrecke
            noRoadNode* current_best_goal = 0;
            // Weg zu beiden Flaggen berechnen
            unsigned length1 = 0, length2 = 0;
            gwg->FindHumanPathOnRoads(start, (*it)->GetF1(), &length1, NULL, *it);
            gwg->FindHumanPathOnRoads(start, (*it)->GetF2(), &length2, NULL, *it);

            // Wenn man zu einer Flagge nich kommt, die jeweils andere nehmen
            if(!length1)
                current_best_goal = (length2) ? (*it)->GetF2() : 0;
            else if(length2)
                current_best_goal = (length1) ? (*it)->GetF1() : 0;
            else
            {
                // ansonsten die kürzeste von beiden
                current_best_goal = (length1 < length2) ? (*it)->GetF1() : (*it)->GetF2();
            }

            // Kein Weg führt hin, nächste StraÃe bitte
            if(!current_best_goal)
                continue;

            // Jeweiligen Weg bestimmen
            unsigned current_best_way = ((*it)->GetF1() == current_best_goal) ? length1 : length2;

            // Produktivität ausrechnen, *10 die Produktivität + die Wegstrecke, damit die
            // auch noch mit einberechnet wird
            unsigned current_productivity = 10 * (*it)->getCarrier(0)->GetProductivity() + current_best_way;

            // Besser als der bisher beste?
            if(current_productivity > best_productivity)
            {
                // Dann wird der vom Thron gestoÃen
                best_productivity = current_productivity;
                best_road = (*it);
                *goal = current_best_goal;
            }

        }
    }

    return best_road;
}

struct ClientForWare
{
    noBaseBuilding* bb;
    unsigned estimate;  // points minus half the optimal distance
    unsigned points;

    ClientForWare(noBaseBuilding* bb, unsigned estimate, unsigned points) : bb(bb), estimate(estimate), points(points) {}

    bool operator<(const ClientForWare b) const
    {
        if (estimate == b.estimate)
        {
            return(points > b.points);
        }
        return(estimate > b.estimate);
    }
};

noBaseBuilding* GameClientPlayer::FindClientForWare(Ware* ware)
{
    // Wenn es eine Goldmünze ist, wird das Ziel auf eine andere Art und Weise berechnet
    if(ware->type == GD_COINS)
        return FindClientForCoin(ware);

    noBaseBuilding* bb = 0;
    unsigned best_points = 0;
    unsigned points;

    // Warentyp herausfinden
    GoodType gt = ware->type;
    // Warentyp für Client-Gebäude
    GoodType gt_clients = ware->type;
    // Andere Nahrung als Fisch ansehen, da nur dieser als Nahrung für Bergwerke und in der Verteilung
    // akzeptiert wird
    if(gt_clients == GD_BREAD || gt_clients == GD_MEAT)
        gt_clients = GD_FISH;

    std::vector<ClientForWare> cfw;

    noRoadNode* start = ware->GetLocation();

    // Bretter und Steine können evtl. auch Häfen für Expeditionen gebrauchen
    if(gt_clients == GD_STONES || gt_clients == GD_BOARDS)
    {
        for(std::list<nobHarborBuilding*>::iterator it = harbors.begin(); it != harbors.end(); ++it)
        {
            points = (*it)->CalcDistributionPoints(gt);

            if (points)
            {
                points += 10 * 30; // Verteilung existiert nicht, Expeditionen haben allerdings hohe Priorität

                cfw.push_back(ClientForWare(*it, points - (gwg->CalcDistance(start->GetX(), start->GetY(), (*it)->GetX(), (*it)->GetY()) / 2), points));
            }
        }
    }

    for(std::list<BuildingType>::iterator it = distribution[gt_clients].client_buildings.begin();
            it != distribution[gt_clients].client_buildings.end(); ++it)
    {
        // BLD_HEADQUARTERS sind Baustellen!!, da HQs ja sowieso nicht gebaut werden können
        if(*it == BLD_HEADQUARTERS)
        {
            // Bei Baustellen die Extraliste abfragen
            for(std::list<noBuildingSite*>::iterator i = building_sites.begin(); i != building_sites.end(); ++i)
            {
                points = (*i)->CalcDistributionPoints(ware->GetLocation(), gt);

                if (points)
                {
                    points += distribution[gt].percent_buildings[BLD_HEADQUARTERS] * 30;

                    cfw.push_back(ClientForWare(*i, points - (gwg->CalcDistance(start->GetX(), start->GetY(), (*i)->GetX(), (*i)->GetY()) / 2), points));
                }
            }
        }
        else
        {
            // Für übrige Gebäude
            for(std::list<nobUsual*>::iterator i = buildings[*it - 10].begin(); i != buildings[*it - 10].end(); ++i)
            {
                points = (*i)->CalcDistributionPoints(ware->GetLocation(), gt);

                // Wenn 0, dann braucht er die Ware nicht
                if (points)
                {
                    if (distribution[gt].goals.size())
                    {
                        if ((*i)->GetBuildingType() == static_cast<BuildingType>(distribution[gt].goals[distribution[gt].selected_goal]))
                        {
                            points += 300;
                        }
                        else if (points >= 300)   // avoid overflows (async!)
                        {
                            points -= 300;
                        }
                    }

                    cfw.push_back(ClientForWare(*i, points - (gwg->CalcDistance(start->GetX(), start->GetY(), (*i)->GetX(), (*i)->GetY()) / 2), points));
                }
            }
        }
    }

    // sort our clients, highest score first
    std::sort(cfw.begin(), cfw.end());

    noBaseBuilding* last_bb = NULL;
    for (std::vector<ClientForWare>::iterator it = cfw.begin(); it != cfw.end(); ++it)
    {
        unsigned path_length;

        // If our estimate is worse (or equal) best_points, the real value cannot be better.
        // As our list is sorted, further entries cannot be better either, so stop searching.
        if ((*it).estimate <= best_points)
            break;

        // get rid of double building entries. TODO: why are there double entries!?
        if ((*it).bb == last_bb)
            continue;

        last_bb = (*it).bb;

        // Find path ONLY if it may be better. Pathfinding is limited to the worst path score that would lead to a better score.
        // This eliminates the worst case scenario where all nodes in a split road network would be hit by the pathfinding only
        // to conclude that there is no possible path.
        if (gwg->FindPathForWareOnRoads(start, (*it).bb, &path_length, NULL, ((*it).points - best_points) * 2 - 1) != 0xFF)
        {
            unsigned score = (*it).points - (path_length / 2);

            // As we have limited our pathfinding to take a maximum of (points - best_points) * 2 - 1 steps,
            // path_length / 2 can at most be points - best_points - 1, so the score will be greater than best_points. :)
            assert(score > best_points);

            best_points = score;
            bb = (*it).bb;
        }
    }

    if(bb && distribution[gt].goals.size())
        distribution[gt].selected_goal = (distribution[gt].selected_goal + 907) % unsigned(distribution[gt].goals.size());

    // Wenn kein Abnehmer gefunden wurde, muss es halt in ein Lagerhaus
    if(!bb)
    {
        // Zuerst Einlagernde Lagerhäuser durchgehen
        bb = FindWarehouse(ware->GetLocation(), FW::Condition_WantStoreWare, 0, true, &gt, true);
        // Wenn das nichts wurde, dann auch restliche Lagerhäuser mit einbeziehen
        if(!bb)
            bb = FindWarehouse(ware->GetLocation(), FW::Condition_StoreWare, 0, true, &gt, true);
    }

    // Abnehmer Bescheid sagen
    if(bb)
        bb->TakeWare(ware);

    return bb;
}

nobBaseMilitary* GameClientPlayer::FindClientForCoin(Ware* ware)
{
    nobBaseMilitary* bb = 0;
    unsigned best_points = 0, points;

    // Militärgebäude durchgehen
    for(std::list<nobMilitary*>::iterator it = military_buildings.begin(); it != military_buildings.end(); ++it)
    {
        unsigned way_points;

        points = (*it)->CalcCoinsPoints();
        // Wenn 0, will er gar keine Münzen (Goldzufuhr gestoppt)
        if(points)
        {
            // Weg dorthin berechnen
            if(gwg->FindPathForWareOnRoads(ware->GetLocation(), *it, &way_points) != 0xFF)
            {
                // Die Wegpunkte noch davon abziehen
                points -= way_points;
                // Besser als der bisher Beste?
                if(points > best_points)
                {
                    best_points = points;
                    bb = *it;
                }
            }

        }
    }

    // Wenn kein Abnehmer gefunden wurde, muss es halt in ein Lagerhaus
    if(!bb)
        bb = FindWarehouse(ware->GetLocation(), FW::Condition_StoreWare, 0, true, &ware->type, true);

    // Abnehmer Bescheid sagen
    if(bb)
        bb->TakeWare(ware);

    return bb;
}

void GameClientPlayer::AddBuildingSite(noBuildingSite* building_site)
{
    building_sites.push_back(building_site);
}

void GameClientPlayer::RemoveBuildingSite(noBuildingSite
        * building_site)
{
    building_sites.remove(building_site);

    if(building_site->GetBuildingType() == BLD_HARBORBUILDING)
        gwg->RemoveHarborBuildingSiteFromSea(building_site);
}

void GameClientPlayer::AddUsualBuilding(nobUsual* building)
{
    buildings[building->GetBuildingType() - 10].push_back(building);
    ChangeStatisticValue(STAT_BUILDINGS, 1);
}

void GameClientPlayer::RemoveUsualBuilding(nobUsual* building)
{
    buildings[building->GetBuildingType() - 10].remove(building);
    ChangeStatisticValue(STAT_BUILDINGS, -1);
}

void GameClientPlayer::AddMilitaryBuilding(nobMilitary* building)
{
    military_buildings.push_back(building);
    ChangeStatisticValue(STAT_BUILDINGS, 1);
}

void GameClientPlayer::RemoveMilitaryBuilding(nobMilitary* building)
{
    military_buildings.remove(building);
    ChangeStatisticValue(STAT_BUILDINGS, -1);
    TestDefeat();
}

/// Gibt Liste von Gebäuden des Spieler zurück
const std::list<nobUsual*>& GameClientPlayer::GetBuildings(const BuildingType type) const
{
    assert(type >= 10);

    return buildings[type - 10];
}

/// Liefert die Anzahl aller Gebäude einzeln
void GameClientPlayer::GetBuildingCount(BuildingCount& bc) const
{
    memset(&bc, 0, sizeof(bc));

    // Normale Gebäude zählen
    for(unsigned i = 0; i < 30; ++i)
        bc.building_counts[i + 10] = buildings[i].size();
    // Lagerhäuser zählen
    for(std::list<nobBaseWarehouse*>::const_iterator it = warehouses.begin(); it != warehouses.end(); ++it)
        ++bc.building_counts[(*it)->GetBuildingType()];
    // Militärgebäude zählen
    for(std::list<nobMilitary*>::const_iterator it = military_buildings.begin(); it != military_buildings.end(); ++it)
        ++bc.building_counts[(*it)->GetBuildingType()];
    // Baustellen zählen
    for(std::list<noBuildingSite*>::const_iterator it = building_sites.begin(); it != building_sites.end(); ++it)
        ++bc.building_site_counts[(*it)->GetBuildingType()];
}


/// Berechnet die durschnittlichen Produktivität eines jeden Gebäudetyps
/// (erwartet als Argument ein 40-er Array!)
void GameClientPlayer::CalcProductivities(std::vector<unsigned short>& productivities)
{
    assert(productivities.size() == 40);

    for(unsigned i = 0; i < 30; ++i)
    {
        // Durschnittliche Produktivität errrechnen, indem man die Produktivitäten aller Gebäude summiert
        // und den Mittelwert bildet
        unsigned total_productivity = 0;

        for(std::list<nobUsual*>::iterator it = buildings[i].begin(); it != buildings[i].end(); ++it)
            total_productivity += *(*it)->GetProduktivityPointer();

        if(buildings[i].size())
            total_productivity /= buildings[i].size();

        productivities[i + 10] = static_cast<unsigned short>(total_productivity);
    }
}

/// Berechnet die durschnittlichen Produktivität aller Gebäude
unsigned short GameClientPlayer::CalcAverageProductivitiy()
{
    unsigned total_productivity = 0;
    unsigned total_count = 0;
    for(unsigned i = 0; i < 30; ++i)
    {
        // Durschnittliche Produktivität errrechnen, indem man die Produktivitäten aller Gebäude summiert
        // und den Mittelwert bildet
        for(std::list<nobUsual*>::iterator it = buildings[i].begin(); it != buildings[i].end(); ++it)
            total_productivity += *(*it)->GetProduktivityPointer();

        if(buildings[i].size())
            total_count += buildings[i].size();
    }
    if (total_count == 0)
        total_count = 1;

    return total_productivity / total_count;
}


unsigned GameClientPlayer::GetBuidingSitePriority(const noBuildingSite* building_site)
{
    if(order_type)
    {
        // Spezielle Reihenfolge

        // Typ in der Reihenfolge suchen und Position als Priorität zurückgeben
        for(unsigned i = 0; i < 31; ++i)
        {
            if(building_site->GetBuildingType() == static_cast<BuildingType>(build_order[i]))
            {
                /*  char str[256];
                sprintf(str,"gf = %u, pr = %u\n",
                GameClient::inst().GetGFNumber(), i);
                GameClient::inst().AddToGameLog(str);*/
                return i;
            }
        }
    }
    else
    {
        // Reihenfolge der Bauaufträge, also was zuerst in Auftrag gegeben wurde, wird zuerst gebaut
        unsigned i = 0;
        for(std::list<noBuildingSite*>::iterator it = building_sites.begin(); it != building_sites.end(); ++it, ++i)
        {
            if(building_site == *it)
                return i;
        }
    }

    LOG.lprintf("GameClientPlayer::GetBuidingSitePriority: ERROR: BuildingSite or type of it not found in the list!\n");
    return 0xFFFFFFFF;
}

void GameClientPlayer::ConvertTransportData(const std::vector<unsigned char>& transport_data)
{
    // Im Replay visulle Einstellungen auf die wirklichen setzen
    if(GameClient::inst().IsReplayModeOn())
        GameClient::inst().visual_settings.transport_order = transport_data;

    // Mit Hilfe der Standardbelegung lässt sich das recht einfach konvertieren:
    for(unsigned i = 0; i < 35; ++i)
    {
        for(unsigned z = 0; z < 14; ++z)
        {
            if(transport_data[z] == STD_TRANSPORT[i])
            {
                transport[i] = z;
                break;
            }
        }

    }
}

bool GameClientPlayer::IsAlly(const unsigned char player) const
{
    // Der Spieler ist ja auch zu sich selber verbündet ;
    if(playerid == player)
        return true;
    else
        return (GetPactState(TREATY_OF_ALLIANCE, player) == GameClientPlayer::ACCEPTED);

}

/// Darf der andere Spieler von mir angegriffen werden?
bool GameClientPlayer::IsPlayerAttackable(const unsigned char player) const
{
    // Verbündete dürfen nicht angegriffen werden
    if(IsAlly(player))
        return false;
    else
        // Ansonsten darf bei bestehendem Nichtangriffspakt ebenfalls nicht angegriffen werden
        return (GetPactState(NON_AGGRESSION_PACT, player) != GameClientPlayer::ACCEPTED);
}


void GameClientPlayer::OrderTroops(nobMilitary* goal, unsigned count,bool ignoresettingsendweakfirst)
{
    // Solange Lagerhäuser nach Soldaten absuchen, bis entweder keins mehr übrig ist oder alle Soldaten bestellt sind
    nobBaseWarehouse* wh;
    do
    {
        unsigned param_count = 1;
        wh = FindWarehouse(goal, FW::Condition_Troops, 0, false, &param_count, false);
        if(wh)
        {
            unsigned order_count = min(wh->GetSoldiersCount(), count);
            count -= order_count;
            wh->OrderTroops(goal, order_count,ignoresettingsendweakfirst);
        }
    }
    while(count && wh);
}


void GameClientPlayer::RegulateAllTroops()
{
    for(std::list<nobMilitary*>::iterator it = military_buildings.begin(); it != military_buildings.end(); ++it)
        (*it)->RegulateTroops();
}

/// Prüft von allen Militärgebäuden die Fahnen neu
void GameClientPlayer::RecalcMilitaryFlags()
{
    for(std::list<nobMilitary*>::iterator it = military_buildings.begin(); it != military_buildings.end(); ++it)
        (*it)->LookForEnemyBuildings(NULL);
}

/// Sucht für EINEN Soldaten ein neues Militärgebäude, als Argument wird Referenz auf die
/// entsprechende Soldatenanzahl im Lagerhaus verlangt
void GameClientPlayer::NewSoldierAvailable(const unsigned& soldier_count)
{
    // solange laufen lassen, bis soldier_count = 0, d.h. der Soldat irgendwohin geschickt wurde
    // Zuerst nach unbesetzten Militärgebäude schauen
    for(std::list<nobMilitary*>::iterator it = military_buildings.begin(); it != military_buildings.end() && soldier_count; ++it)
    {
        if((*it)->IsNewBuilt())
            (*it)->RegulateTroops();
    }

    if(!soldier_count)
        return;

    // Als nächstes Gebäude in Grenznähe
    for(std::list<nobMilitary*>::iterator it = military_buildings.begin(); it != military_buildings.end() && soldier_count; ++it)
    {
        if((*it)->GetFrontierDistance() == 2)
            (*it)->RegulateTroops();
    }

    if(!soldier_count)
        return;

    // Und den Rest ggf.
    for(std::list<nobMilitary*>::iterator it = military_buildings.begin(); it != military_buildings.end() && soldier_count; ++it)
	{
		//already checked? -> skip
		if((*it)->GetFrontierDistance() == 2 || (*it)->IsNewBuilt())
			continue;
		(*it)->RegulateTroops();
		if(!soldier_count) //used the soldier?
			return;
	}

}

void GameClientPlayer::CallFlagWorker(const unsigned short x, const unsigned short y, const Job job)
{
    /// Flagge rausfinden
    noFlag* flag = gwg->GetSpecObj<noFlag>(x, y);
    /// Lagerhaus mit Geologen finden
    FW::Param_Job p = { job, 1 };
    nobBaseWarehouse* wh = FindWarehouse(flag, FW::Condition_Job, 0, false, &p, false);

    /// Wenns eins gibt, dann rufen
    if(wh)
        wh->OrderJob(job, flag, true);
}


void GameClientPlayer::FlagDestroyed(noFlag* flag)
{
    // Alle durchgehen und ggf. sagen, dass sie keine Flagge mehr haben, wenn das ihre Flagge war, die zerstört wurde
    for(std::list<nofFlagWorker*>::iterator it = flagworkers.begin(); it != flagworkers.end();)
    {
        if((*it)->GetFlag() == flag)
        {
            (*it)->LostWork();
            it = flagworkers.erase(it);
        }
        else
            ++it;
    }
}

void GameClientPlayer::RefreshDefenderList()
{
    /// Die Verteidigungsliste muss erneuert werden
    memset(defenders, 0, 5);
    for(unsigned i = 0; i < 5; ++i)
        defenders[i] = (i < military_settings[2] * 5 / MILITARY_SETTINGS_SCALE[2]);
    // und ordentlich schütteln
    RANDOM.Shuffle(defenders, 5);

    defenders_pos = 0;
}

void GameClientPlayer::ChangeMilitarySettings(const std::vector<unsigned char>& military_settings)
{
    // Im Replay visulle Einstellungen auf die wirklichen setzen
    if(GameClient::inst().IsReplayModeOn())
        GameClient::inst().visual_settings.military_settings = military_settings;

    for(unsigned i = 0; i < military_settings.size(); ++i)
    {
        // Sicherstellen, dass im validen Bereich
        assert(military_settings[i] <= MILITARY_SETTINGS_SCALE[i]);
        this->military_settings[i] = military_settings[i];
    }
    /// Truppen müssen neu kalkuliert werden
    RegulateAllTroops();
    /// Die Verteidigungsliste muss erneuert werden
    RefreshDefenderList();
}

/// Setzt neue Werkzeugeinstellungen
void GameClientPlayer::ChangeToolsSettings(const std::vector<unsigned char>& tools_settings)
{
    // Im Replay visulle Einstellungen auf die wirklichen setzen
    if(GameClient::inst().IsReplayModeOn())
        GameClient::inst().visual_settings.tools_settings = tools_settings;

    this->tools_settings = tools_settings;
}

/// Setzt neue Verteilungseinstellungen
void GameClientPlayer::ChangeDistribution(const std::vector<unsigned char>& distribution_settings)
{
    // Im Replay visulle Einstellungen auf die wirklichen setzen
    if(GameClient::inst().IsReplayModeOn())
        GameClient::inst().visual_settings.distribution = distribution_settings;

    distribution[GD_FISH].percent_buildings[BLD_GRANITEMINE] = distribution_settings[0];
    distribution[GD_FISH].percent_buildings[BLD_COALMINE] = distribution_settings[1];
    distribution[GD_FISH].percent_buildings[BLD_IRONMINE] = distribution_settings[2];
    distribution[GD_FISH].percent_buildings[BLD_GOLDMINE] = distribution_settings[3];

    distribution[GD_GRAIN].percent_buildings[BLD_MILL] = distribution_settings[4];
    distribution[GD_GRAIN].percent_buildings[BLD_PIGFARM] = distribution_settings[5];
    distribution[GD_GRAIN].percent_buildings[BLD_DONKEYBREEDER] = distribution_settings[6];
    distribution[GD_GRAIN].percent_buildings[BLD_BREWERY] = distribution_settings[7];
    distribution[GD_GRAIN].percent_buildings[BLD_CHARBURNER] = distribution_settings[8];

    distribution[GD_IRON].percent_buildings[BLD_ARMORY] = distribution_settings[9];
    distribution[GD_IRON].percent_buildings[BLD_METALWORKS] = distribution_settings[10];

    distribution[GD_COAL].percent_buildings[BLD_ARMORY] = distribution_settings[11];
    distribution[GD_COAL].percent_buildings[BLD_IRONSMELTER] = distribution_settings[12];
    distribution[GD_COAL].percent_buildings[BLD_MINT] = distribution_settings[13];

    distribution[GD_WOOD].percent_buildings[BLD_SAWMILL] = distribution_settings[14];
    distribution[GD_WOOD].percent_buildings[BLD_CHARBURNER] = distribution_settings[15];

    distribution[GD_BOARDS].percent_buildings[BLD_HEADQUARTERS] = distribution_settings[16];
    distribution[GD_BOARDS].percent_buildings[BLD_METALWORKS] = distribution_settings[17];
    distribution[GD_BOARDS].percent_buildings[BLD_SHIPYARD] = distribution_settings[18];

    distribution[GD_WATER].percent_buildings[BLD_BAKERY] = distribution_settings[19];
    distribution[GD_WATER].percent_buildings[BLD_BREWERY] = distribution_settings[20];
    distribution[GD_WATER].percent_buildings[BLD_PIGFARM] = distribution_settings[21];
    distribution[GD_WATER].percent_buildings[BLD_DONKEYBREEDER] = distribution_settings[22];

    RecalcDistribution();
}

/// Setzt neue Baureihenfolge-Einstellungen
void GameClientPlayer::ChangeBuildOrder(const unsigned char order_type, const std::vector<unsigned char>& oder_data)
{
    // Im Replay visulle Einstellungen auf die wirklichen setzen
    if(GameClient::inst().IsReplayModeOn())
    {
        GameClient::inst().visual_settings.order_type = order_type;
        GameClient::inst().visual_settings.build_order = oder_data;
    }

    this->order_type = order_type;
    for(unsigned i = 0; i < oder_data.size(); ++i)
        this->build_order[i] = oder_data[i];
}

bool GameClientPlayer::ShouldSendDefender()
{
    // Wenn wir schon am Ende sind, muss die Verteidgungsliste erneuert werden
    if(defenders_pos == 4)
        RefreshDefenderList();

    return defenders[defenders_pos++];
}

void GameClientPlayer::TestDefeat()
{
    // Nicht schon besiegt?
    // Keine Militärgebäude, keine Lagerhäuser (HQ,Häfen) -> kein Land --> verloren
    if(!defeated && !military_buildings.size() && !warehouses.size())
    {
        defeated = true;

        // GUI Bescheid sagen
		if(gwg->GetGameInterface())
			gwg->GetGameInterface()->GI_PlayerDefeated(playerid);
		else
			LOG.lprintf("Warning: Player %i defeated but could not find GameInterface (GameClientPlayer.cpp::TestDefeat()\n",playerid);
    }
}

//void GameClientPlayer::GetInventory(unsigned int *wares, unsigned int *figures)
//{
//  // todo: waren in richtige reihenfolge bringen...
//  static GoodType ware_map[31] = {
//      GD_WOOD, GD_BOARDS, GD_STONES, GD_HAM,
//      GD_GRAIN, GD_FLOUR, GD_FISH, GD_MEAT, GD_BREAD,
//      GD_WATER, GD_BEER, GD_COAL, GD_IRONORE,
//      GD_GOLD, GD_IRON, GD_COINS, GD_TONGS, GD_AXE,
//      GD_SAW, GD_PICKAXE, GD_HAMMER, GD_SHOVEL,
//      GD_CRUCIBLE, GD_RODANDLINE, GD_SCYTHE, GD_CLEAVER, GD_ROLLINGPIN,
//      GD_BOW, GD_SWORD, GD_SHIELDROMANS, GD_BOAT
//  };
//
//  /*static Job figure_map[30] = {
//      JOB_HELPER, JOB_BUILDER, JOB_PLANER, JOB_WOODCUTTER,
//      JOB_FORESTER, JOB_STONEMASON, JOB_FISHER, JOB_HUNTER, JOB_CARPENTER,
//      JOB_FARMER, JOB_PIGBREEDER, JOB_DONKEYBREEDER, JOB_MILLER,
//      JOB_BAKER, JOB_BUTCHER, JOB_BREWER, JOB_MINER, JOB_IRONFOUNDER,
//      JOB_ARMORER, JOB_MINTER, JOB_METALWORKER, JOB_SHIPWRIGHT,
//      JOB_GEOLOGIST, JOB_SCOUT, JOB_PACKDONKEY, JOB_PRIVATE, JOB_PRIVATEFIRSTCLASS,
//      JOB_SERGEANT, JOB_OFFICER, JOB_GENERAL
//  };*/
//
//  // Warenlisten der Warenhäuser sammeln
//  for(std::list<nobBaseWarehouse*>::iterator wh = warehouses.begin(); wh.valid(); ++wh)
//      (*wh)->GetInventory(wares, figures);
//
//  if(wares)
//  {
//      // einzelne Waren sammeln
//      for(std::list<Ware*>::iterator we = ware_list.begin(); we.valid(); ++we)
//      {
//          ++(wares[ware_map[(*we)->type]]);
//      }
//  }
//
//  // Todo: einzelne Figuren sammeln
//  if(figures)
//  {
//  }
//}

void GameClientPlayer::Surrender()
{
    defeated = true;

    // GUI Bescheid sagen
    gwg->GetGameInterface()->GI_PlayerDefeated(playerid);
}

void GameClientPlayer::SetStatisticValue(StatisticType type, unsigned int value)
{
    statisticCurrentData[type] = value;
}

void GameClientPlayer::ChangeStatisticValue(StatisticType type, int change)
{
    assert (statisticCurrentData[type] + change >= 0);
    statisticCurrentData[type] += change;
}

void GameClientPlayer::IncreaseMerchandiseStatistic(GoodType type)
{
    // Einsortieren...
    switch(type)
    {
        case GD_WOOD: statisticCurrentMerchandiseData[0]++; break;
        case GD_BOARDS: statisticCurrentMerchandiseData[1]++; break;
        case GD_STONES: statisticCurrentMerchandiseData[2]++; break;
        case GD_FISH: case GD_BREAD: case GD_MEAT: statisticCurrentMerchandiseData[3]++; break;
        case GD_WATER: statisticCurrentMerchandiseData[4]++; break;
        case GD_BEER: statisticCurrentMerchandiseData[5]++; break;
        case GD_COAL: statisticCurrentMerchandiseData[6]++; break;
        case GD_IRONORE: statisticCurrentMerchandiseData[7]++; break;
        case GD_GOLD: statisticCurrentMerchandiseData[8]++; break;
        case GD_IRON: statisticCurrentMerchandiseData[9]++; break;
        case GD_COINS: statisticCurrentMerchandiseData[10]++; break;
        case GD_TONGS: case GD_AXE: case GD_SAW: case GD_PICKAXE: case GD_HAMMER: case GD_SHOVEL:
        case GD_CRUCIBLE: case GD_RODANDLINE: case GD_SCYTHE: case GD_CLEAVER: case GD_ROLLINGPIN:
        case GD_BOW: statisticCurrentMerchandiseData[11]++; break;
        case GD_SHIELDVIKINGS: case GD_SHIELDAFRICANS: case GD_SHIELDROMANS: case GD_SHIELDJAPANESE:
        case GD_SWORD: statisticCurrentMerchandiseData[12]++; break;
        case GD_BOAT: statisticCurrentMerchandiseData[13]++; break;
        default:
            break;
    }

}

/// Calculates current statistics
void GameClientPlayer::CalcStatistics()
{
    // Waren aus der Inventur zählen
    statisticCurrentData[STAT_MERCHANDISE] = 0;
    for (unsigned int i = 0; i < WARE_TYPES_COUNT; ++i)
        statisticCurrentData[STAT_MERCHANDISE] += global_inventory.goods[i];

    // Bevölkerung aus der Inventur zählen
    statisticCurrentData[STAT_INHABITANTS] = 0;
    for (unsigned int i = 0; i < JOB_TYPES_COUNT; ++i)
        statisticCurrentData[STAT_INHABITANTS] += global_inventory.people[i];

    // Militär aus der Inventur zählen
    statisticCurrentData[STAT_MILITARY] =
        global_inventory.people[JOB_PRIVATE]
        + global_inventory.people[JOB_PRIVATEFIRSTCLASS] * 2
        + global_inventory.people[JOB_SERGEANT] * 3
        + global_inventory.people[JOB_OFFICER] * 4
        + global_inventory.people[JOB_GENERAL] * 5;


    // Produktivität berechnen
    statisticCurrentData[STAT_PRODUCTIVITY] = CalcAverageProductivitiy();

    // Total points for tournament games
    statisticCurrentData[STAT_TOURNAMENT] = statisticCurrentData[STAT_MILITARY]
                                            + 3 * statisticCurrentData[STAT_VANQUISHED] ;
}

void GameClientPlayer::StatisticStep()
{
    CalcStatistics();

    // 15-min-Statistik ein Feld weiterschieben
    for (unsigned int i = 0; i < STAT_TYPE_COUNT; ++i)
    {
        statistic[STAT_15M].data[i][incrStatIndex(statistic[STAT_15M].currentIndex)] = statisticCurrentData[i];
    }
    for (unsigned int i = 0; i < STAT_MERCHANDISE_TYPE_COUNT; ++i)
    {
        statistic[STAT_15M].merchandiseData[i][incrStatIndex(statistic[STAT_15M].currentIndex)] = statisticCurrentMerchandiseData[i];
    }
    statistic[STAT_15M].currentIndex = incrStatIndex(statistic[STAT_15M].currentIndex);

    statistic[STAT_15M].counter++;

    // Prüfen ob 4mal 15-min-Statistik weitergeschoben wurde, wenn ja: 1-h-Statistik weiterschieben
    // und aktuellen Wert der 15min-Statistik benutzen
    // gleiches für die 4h und 16h Statistik
    for (unsigned t = STAT_15M; t < STAT_16H; t++)
    {
        if (statistic[t].counter == 4)
        {
            statistic[t].counter = 0;
            for (unsigned int i = 0; i < STAT_TYPE_COUNT; ++i)
            {
                statistic[t + 1].data[i][incrStatIndex(statistic[t + 1].currentIndex)] = statisticCurrentData[i];
            }

            // Summe für den Zeitraum berechnen (immer 4 Zeitschritte der jeweils kleineren Statistik)
            for (unsigned int i = 0; i < STAT_MERCHANDISE_TYPE_COUNT; ++i)
            {
                statistic[t + 1].merchandiseData[i][incrStatIndex(statistic[t + 1].currentIndex)] = statisticCurrentMerchandiseData[i]
                        + statistic[t].merchandiseData[i][decrStatIndex(statistic[t].currentIndex, 1)]
                        + statistic[t].merchandiseData[i][decrStatIndex(statistic[t].currentIndex, 2)]
                        + statistic[t].merchandiseData[i][decrStatIndex(statistic[t].currentIndex, 3)];
            }

            statistic[t + 1].currentIndex = incrStatIndex(statistic[t + 1].currentIndex);
            statistic[t + 1].counter++;
        }
    }

    // Warenstatistikzähler nullen
    for (unsigned int i = 0; i < STAT_MERCHANDISE_TYPE_COUNT; ++i)
    {
        statisticCurrentMerchandiseData[i] = 0;
    }
}

GameClientPlayer::Pact::Pact(SerializedGameData* ser)
    : accepted(ser->PopBool()), duration(ser->PopUnsignedInt()),
      start(ser->PopUnsignedInt()), want_cancel (ser->PopBool()) { }

void GameClientPlayer::Pact::Serialize(SerializedGameData* ser)
{
    ser->PushBool(accepted);
    ser->PushUnsignedInt(duration);
    ser->PushUnsignedInt(start);
    ser->PushBool(want_cancel);
}

/// Macht Bündnisvorschlag an diesen Spieler
void GameClientPlayer::SuggestPact(const unsigned char other_player, const PactType pt, const unsigned duration)
{
    pacts[other_player][pt].accepted = false;
    pacts[other_player][pt].duration = duration;
    pacts[other_player][pt].start = GameClient::inst().GetGFNumber();

    // Post-Message generieren, wenn dieser Pakt den lokalen Spieler betrifft
    if(other_player == GameClient::inst().GetPlayerID())
        GameClient::inst().SendPostMessage(new DiplomacyPostQuestion(pacts[other_player][pt].start, playerid, pt, duration));
}

/// Akzeptiert ein bestimmtes Bündnis, welches an diesen Spieler gemacht wurde
void GameClientPlayer::AcceptPact(const unsigned id, const PactType pt, const unsigned char other_player)
{
    if(pacts[other_player][pt].accepted == false && pacts[other_player][pt].start == id)
    {
        // Pakt einwickeln
        MakePact(pt, other_player, pacts[other_player][pt].duration);
        GameClient::inst().GetPlayer(other_player)->MakePact(pt, playerid, pacts[other_player][pt].duration);

        // Besetzung der Militärgebäude der jeweiligen Spieler überprüfen, da ja jetzt neue Feinde oder neue
        // Verbündete sich in Grenznähe befinden könnten
        this->RegulateAllTroops();
        GameClient::inst().GetPlayer(other_player)->RecalcMilitaryFlags();

        // Ggf. den GUI Bescheid sagen, um Sichtbarkeiten etc. neu zu berechnen
        if(pt == TREATY_OF_ALLIANCE && (GameClient::inst().GetPlayerID() == playerid || GameClient::inst().GetPlayerID() == other_player))
        {
            if(gwg->GetGameInterface())
                gwg->GetGameInterface()->GI_TreatyOfAllianceChanged();
        }
    }
}

/// Bündnis (real, d.h. spielentscheidend) abschlieÃen
void GameClientPlayer::MakePact(const PactType pt, const unsigned char other_player, const unsigned duration)
{
    pacts[other_player][pt].accepted = true;
    pacts[other_player][pt].start = GameClient::inst().GetGFNumber();
    pacts[other_player][pt].duration = duration;
    pacts[other_player][pt].want_cancel = false;

    // Den Spielern eine Informationsnachricht schicken
    if(GameClient::inst().GetPlayerID() == playerid)
        GameClient::inst().SendPostMessage(new DiplomacyPostInfo(other_player, DiplomacyPostInfo::ACCEPT, pt));

}

/// Zeigt an, ob ein Pakt besteht
GameClientPlayer::PactState GameClientPlayer::GetPactState(const PactType pt, const unsigned char other_player) const
{
    // Prüfen, ob Bündnis in Kraft ist
    if(pacts[other_player][pt].duration)
    {
        if(!pacts[other_player][pt].accepted)
            return IN_PROGRESS;

        if(pacts[other_player][pt].duration == 0xFFFFFFFF)
        {
            if(pacts[other_player][pt].accepted)
                return ACCEPTED;
        }
        else if(GameClient::inst().GetGFNumber() <= pacts[other_player][pt].start
                + pacts[other_player][pt].duration )
            return ACCEPTED;

    }

    return NO_PACT;
}

///all allied players get a letter with the location
void GameClientPlayer::NotifyAlliesOfLocation(MapCoord x, MapCoord y, unsigned char allyplayerid)
{	
	for(unsigned i = 0; i < GameClient::inst().GetPlayerCount(); ++i)
    {
        GameClientPlayer* p = GameClient::inst().GetPlayer(i);
		if(i != allyplayerid && p->IsAlly(allyplayerid+1) && GameClient::inst().GetPlayerID() == i)
		{	            		
            GameClient::inst().SendPostMessage(new PostMsgWithLocation(_("Your ally wishes to notify you of this location"), PMC_DIPLOMACY, x, y));
		}
	}
}

/// Gibt die verbleibende Dauer zurück, die ein Bündnis noch laufen wird (0xFFFFFFFF = für immer)
unsigned GameClientPlayer::GetRemainingPactTime(const PactType pt, const unsigned char other_player) const
{
    if(pacts[other_player][pt].duration)
    {
        if(pacts[other_player][pt].accepted)
        {
            if(pacts[other_player][pt].duration == 0xFFFFFFFF)
                return 0xFFFFFFFF;
            else if(GameClient::inst().GetGFNumber() <= pacts[other_player][pt].start + pacts[other_player][pt].duration)
                return ((pacts[other_player][pt].start + pacts[other_player][pt].duration) - GameClient::inst().GetGFNumber());
        }
    }

    return 0;
}

/// Gibt Einverständnis, dass dieser Spieler den Pakt auflösen will
/// Falls dieser Spieler einen Bündnisvorschlag gemacht hat, wird dieser dagegen zurückgenommen
void GameClientPlayer::CancelPact(const PactType pt, const unsigned char other_player)
{
    // Besteht bereits ein Bündnis?
    if(pacts[other_player][pt].accepted)
    {
        // Vermerken, dass der Spieler das Bündnis auflösen will
        pacts[other_player][pt].want_cancel = true;

        // Will der andere Spieler das Bündnis auch auflösen?
        if(GameClient::inst().GetPlayer(other_player)->pacts[playerid][pt].want_cancel)
        {
            // Dann wird das Bündnis aufgelöst
            pacts[other_player][pt].accepted = false;
            pacts[other_player][pt].duration = 0;
            pacts[other_player][pt].want_cancel = false;

            GameClient::inst().GetPlayer(other_player)->pacts[playerid][pt].accepted = false;
            GameClient::inst().GetPlayer(other_player)->pacts[playerid][pt].duration = 0;
            GameClient::inst().GetPlayer(other_player)->pacts[playerid][pt].want_cancel = false;

            // Den Spielern eine Informationsnachricht schicken
            if(GameClient::inst().GetPlayerID() == playerid || GameClient::inst().GetPlayerID() == other_player)
            {
                // Anderen Spieler von sich aus ermitteln
                unsigned char client_other_player = (GameClient::inst().GetPlayerID() == playerid) ? other_player : playerid;
                GameClient::inst().SendPostMessage(new DiplomacyPostInfo(client_other_player, DiplomacyPostInfo::CANCEL, pt));
            }

            // Ggf. den GUI Bescheid sagen, um Sichtbarkeiten etc. neu zu berechnen
            if(pt == TREATY_OF_ALLIANCE && (GameClient::inst().GetPlayerID() == playerid
                                            || GameClient::inst().GetPlayerID() == other_player))
            {
                if(gwg->GetGameInterface())
                    gwg->GetGameInterface()->GI_TreatyOfAllianceChanged();
            }
        }
        // Ansonsten den anderen Spieler fragen, ob der das auch so sieht
        else if(other_player == GameClient::inst().GetPlayerID())
            GameClient::inst().SendPostMessage(new DiplomacyPostQuestion(pacts[other_player][pt].start, playerid, pt));
    }
    else
    {
        // Es besteht kein Bündnis, also unseren Bündnisvorschlag wieder zurücknehmen
        pacts[other_player][pt].duration = 0;
    }
}

void GameClientPlayer::MakeStartPacts()
{
    // Zu den Spielern im selben Team Bündnisse (sowohl Bündnisvertrag als auch Nichtangriffspakt) aufbauen
    for(unsigned i = 0; i < GameClient::inst().GetPlayerCount(); ++i)
    {
        GameClientPlayer* p = GameClient::inst().GetPlayer(i);
        if(GetFixedTeam(team) == GetFixedTeam(p->team) && GetFixedTeam(team) >= TM_TEAM1 && GetFixedTeam(team) <= TM_TEAM4)
        {
            for(unsigned z = 0; z < PACTS_COUNT; ++z)
            {
                pacts[i][z].accepted = true;
                pacts[i][z].duration = 0xFFFFFFFF;
                pacts[i][z].start = 0;
                pacts[i][z].want_cancel = false;
            }
        }
    }
}

Team GameClientPlayer::GetFixedTeam(Team rawteam)
{
    if(rawteam == TM_RANDOMTEAM)
        return TM_TEAM1;
    if(rawteam > TM_TEAM4)
        return Team(rawteam - 3);
    return rawteam;
}

bool GameClientPlayer::IsWareDependent(Ware* ware)
{
    for(std::list<nobBaseWarehouse*>::iterator it = warehouses.begin(); it != warehouses.end(); ++it)
    {
        if((*it)->IsWareDependent(ware))
            return true;
    }

    return false;
}

/// Registriert ein Schiff beim Einwohnermeldeamt
void GameClientPlayer::RegisterShip(noShip* ship)
{
    ships.push_back(ship);
    // Evtl bekommt das Schiffchen gleich was zu tun?
    GetJobForShip(ship);
}

struct ShipForHarbor
{
    noShip* ship;
    uint32_t estimate;

    ShipForHarbor(noShip* ship, uint32_t estimate) : ship(ship), estimate(estimate) {}

    bool operator<(const ShipForHarbor b) const
    {
        return(estimate < b.estimate);
    }
};

/// Schiff für Hafen bestellen
bool GameClientPlayer::OrderShip(nobHarborBuilding* hb)
{
    std::vector<ShipForHarbor> sfh;

    // we need more ships than those that are already on their way? limit search to idle ships
    if (GetShipsToHarbor(hb) < hb->GetNeededShipsCount())
    {
        for (std::vector<noShip*>::iterator it = ships.begin(); it != ships.end(); ++it)
        {
            if ((*it)->IsIdling() && gwg->IsAtThisSea(gwg->GetHarborPointID(hb->GetX(), hb->GetY()), (*it)->GetSeaID()))
            {
                sfh.push_back(ShipForHarbor(*it, gwg->CalcDistance(hb->GetX(), hb->GetY(), (*it)->GetX(), (*it)->GetY())));
            }
        }
    }
    else
    {
        for (std::vector<noShip*>::iterator it = ships.begin(); it != ships.end(); ++it)
        {
            if ((*it)->IsIdling())
            {
                if (gwg->IsAtThisSea(gwg->GetHarborPointID(hb->GetX(), hb->GetY()), (*it)->GetSeaID()))
                {
                    sfh.push_back(ShipForHarbor(*it, gwg->CalcDistance(hb->GetX(), hb->GetY(), (*it)->GetX(), (*it)->GetY())));
                }
            }
            else if ((*it)->IsGoingToHarbor(hb))
            {
                sfh.push_back(ShipForHarbor(*it, gwg->CalcDistance(hb->GetX(), hb->GetY(), (*it)->GetX(), (*it)->GetY())));
            }
        }
    }

    std::sort(sfh.begin(), sfh.end());

    noShip* best_ship = NULL;
    uint32_t best_distance = std::numeric_limits<uint32_t>::max();
    std::vector<unsigned char> best_route;

    for (std::vector<ShipForHarbor>::iterator it = sfh.begin(); it != sfh.end(); ++it)
    {
        uint32_t distance;
        std::vector<unsigned char> route;

        // the estimate (air-line distance) for this and all other ships in the list is already worse than what we found? disregard the rest
        if ((*it).estimate >= best_distance)
        {
            break;
        }

        MapCoord dest_x, dest_y;
        noShip* ship = (*it).ship;

        gwg->GetCoastalPoint(hb->GetHarborPosID(), &dest_x, &dest_y, ship->GetSeaID());

        // ship already there?
        if ((ship->GetX() == dest_x) && (ship->GetY() == dest_y))
        {
            ship->AssignHarborId(hb->GetHarborPosID());
            hb->ShipArrived(ship);
            return(true);
        }

        if (gwg->FindShipPath(ship->GetX(), ship->GetY(), dest_x, dest_y, &route, &distance))
        {
            if (distance < best_distance)
            {
                best_ship = ship;
                best_distance = distance;
                best_route = route;
            }
        }
    }

    // only order ships not already on their way
    if (best_ship && best_ship->IsIdling())
    {
        best_ship->GoToHarbor(hb, best_route);

        return(true);
    }

    return(false);
}
//
///// Meldet EIN bestelltes Schiff wieder ab
//void GameClientPlayer::RemoveOrderedShip(nobHarborBuilding * hb)
//{
//  for(std::list<nobHarborBuilding*>::iterator it = ships_needed.begin();it!=ships_needed.end();++it)
//  {
//      if(*it == hb)
//      {
//          ships_needed.erase(it);
//          return;
//      }
//  }
//}


/// Meldet das Schiff wieder ab
void GameClientPlayer::RemoveShip(noShip* ship)
{
    for(unsigned i = 0; i < ships.size(); ++i)
    {
        if(ships[i] == ship)
        {
            ships.erase(ships.begin() + i);
            return;
        }
    }
}

/// Versucht, für ein untätiges Schiff eine Arbeit zu suchen
void GameClientPlayer::GetJobForShip(noShip* ship)
{
    // Evtl. steht irgendwo eine Expedition an und das Schiff kann diese übernehmen
    nobHarborBuilding* best = 0;
    int best_points = 0;
    std::vector<unsigned char> best_route;

    // Beste Weglänge, die ein Schiff zurücklegen muss, welches gerade nichts zu tun hat
    for(std::list<nobHarborBuilding*>::iterator it = harbors.begin(); it != harbors.end(); ++it)
    {
        // Braucht der Hafen noch Schiffe?
        if((*it)->GetNeededShipsCount() == 0)
            continue;

        // Anzahl der Schiffe ermitteln, die diesen Hafen bereits anfahren
        unsigned ships_coming = GetShipsToHarbor(*it);

        // Evtl. kommen schon genug?
        if((*it)->GetNeededShipsCount() <= ships_coming)
            continue;

        // liegen wir am gleichen Meer?
        if(gwg->IsAtThisSea((*it)->GetHarborPosID(), ship->GetSeaID()))
        {
            MapCoord dest_x, dest_y;
            gwg->GetCoastalPoint((*it)->GetHarborPosID(), &dest_x, &dest_y, ship->GetSeaID());

            // Evtl. sind wir schon da?
            if(ship->GetX() == dest_x && ship->GetY() == dest_y)
            {
                ship->AssignHarborId((*it)->GetHarborPosID());
                (*it)->ShipArrived(ship);
                return;
            }

            unsigned length;
            std::vector<unsigned char> route;

            if(gwg->FindShipPath(ship->GetX(), ship->GetY(), dest_x, dest_y, &route, &length))
            {
                // Punkte ausrechnen
                int points = (*it)->GetNeedForShip(ships_coming) - length;
                if(points > best_points || best == NULL)
                {
                    best = *it;
                    best_points = points;
                    best_route = route;
                }
            }
        }
    }

    // Einen Hafen gefunden?
    if(best)
        // Dann bekommt das gleich der Hafen
        ship->GoToHarbor(best, best_route);
}


/// Gibt die ID eines Schiffes zurück
unsigned GameClientPlayer::GetShipID(const noShip* const ship) const
{
    for(unsigned i = 0; i < ships.size(); ++i)
        if(ships[i] == ship)
            return i;

    return 0xFFFFFFFF;
}

/// Gibt ein Schiff anhand der ID zurück bzw. NULL, wenn keines mit der ID existiert
noShip* GameClientPlayer::GetShipByID(const unsigned ship_id) const
{
    if(ship_id >= ships.size())
        return NULL;
    else
        return ships[ship_id];
}


/// Gibt eine Liste mit allen Häfen dieses Spieler zurück, die an ein bestimmtes Meer angrenzen
void GameClientPlayer::GetHarborBuildings(std::vector<nobHarborBuilding*>& harbor_buildings,
        const unsigned short sea_id) const
{
    for(std::list<nobBaseWarehouse*>::const_iterator it = warehouses.begin(); it != warehouses.end(); ++it)
    {
        if((*it)->GetBuildingType() == BLD_HARBORBUILDING)
        {
            bool existing = false;
            for(unsigned i = 0; i < harbor_buildings.size(); ++i)
            {
                if(harbor_buildings[i] == *it)
                {
                    existing = true;
                    break;
                }
            }

            if(existing)
                continue;

            if(gwg->IsAtThisSea(static_cast<nobHarborBuilding*>(*it)->GetHarborPosID(), sea_id))
                harbor_buildings.push_back(static_cast<nobHarborBuilding*>(*it));
        }
    }

}


/// Gibt die Anzahl der Schiffe, die einen bestimmten Hafen ansteuern, zurück
unsigned GameClientPlayer::GetShipsToHarbor(nobHarborBuilding* hb) const
{
    unsigned count = 0;
    for(unsigned i = 0; i < ships.size(); ++i)
    {
        if(ships[i]->IsGoingToHarbor(hb))
            ++count;
    }

    return count;
}


/// Gibt der Wirtschaft Bescheid, dass ein Hafen zerstört wurde
void GameClientPlayer::HarborDestroyed(nobHarborBuilding* hb)
{
    // Schiffen Bescheid sagen
    for(unsigned i = 0; i < ships.size(); ++i)
        ships[i]->HarborDestroyed(hb);
}


/// Sucht einen Hafen in der Nähe, wo dieses Schiff seine Waren abladen kann
/// gibt true zurück, falls erfolgreich
bool GameClientPlayer::FindHarborForUnloading(noShip* ship, const MapCoord start_x, const MapCoord start_y, unsigned* goal_harbor_id,
        std::vector<unsigned char>* route, nobHarborBuilding* exception)
{
    nobHarborBuilding* best = NULL;
    unsigned best_distance = 0xffffffff;

    for(std::list<nobHarborBuilding*>::iterator it = harbors.begin(); it != harbors.end(); ++it)
    {
        nobHarborBuilding* hb = *it;
        // Bestimmten Hafen ausschlieÃen
        if(hb == exception)
            continue;

        // Prüfen, ob Hafen an das Meer, wo sich das Schiff gerade befindet, angrenzt
        if(!gwg->IsAtThisSea(hb->GetHarborPosID(), ship->GetSeaID()))
            continue;

        // Distanz ermitteln zwischen Schiff und Hafen, Schiff kann natürlich auch über Kartenränder fahren
        unsigned distance = gwg->CalcDistance(ship->GetX(), ship->GetY(), hb->GetX(), hb->GetY());

        // Kürzerer Weg als bisher bestes Ziel?
        if(distance < best_distance)
        {
            best_distance = distance;
            best = hb;
        }
    }

    // Hafen gefunden?
    if(best)
    {
        // Weg dorthin suchen
        MapCoord dx, dy;
        gwg->GetCoastalPoint(best->GetHarborPosID(), &dx, &dy, ship->GetSeaID());
        route->clear();
        *goal_harbor_id = best->GetHarborPosID();
        // Weg dorthin gefunden?
        if(gwg->FindShipPath(start_x, start_y, dx, dy, route, NULL))
            return true;
        else
            return false;
    }
    else
        return false;
}

void GameClientPlayer::TestForEmergencyProgramm()
{
    // we are already defeated, do not even think about an emergency program - it's too late :-(
    if (defeated || !warehouses.size())
    {
        return;
    }

    // In Lagern vorhandene Bretter und Steine zählen
    unsigned boards = 0;
    unsigned stones = 0;
    for(std::list<nobBaseWarehouse*>::iterator w = warehouses.begin(); w != warehouses.end(); ++w)
    {
        boards += (*w)->GetInventory()->goods[GD_BOARDS];
        stones += (*w)->GetInventory()->goods[GD_STONES];
		/*std::list<Ware*> testwares = (*w)->GetDependentWares();
		unsigned c=0;
		unsigned bad=-1;
		for(std::list<Ware*>::iterator a = testwares.begin(); a!= testwares.end(); ++a)
		{
			c++;
			if((*a)->goal->GetX()!=(*w)->GetX() || (*a)->goal->GetY()!=(*w)->GetY())
			{
				bad=c;
				LOG.lprintf("bad ware id %i, type %i, player %i, wareloc %i,%i, goal loc %i,%i expecting wh %i,%i warenumber %i \n",(*a)->GetObjId(),(*a)->type,playerid,(*a)->GetLocation()?(*a)->GetLocation()->GetX():0,(*a)->GetLocation()?(*a)->GetLocation()->GetY():0, (*a)->goal->GetX(),(*a)->goal->GetY(),(*w)->GetX(),(*w)->GetY(),c);
			}			
		}*/
    }
	/*	
	unsigned c=0;
	for(std::list<JobNeeded>::iterator it = jobs_wanted.begin(); it != jobs_wanted.end(); ++it)
	{
		c++;
		if(!(it->workplace->GetX()<0||it->workplace->GetX()>300||it->workplace->GetY()<0||it->workplace->GetY()>300) && it->workplace->GetPlayer()!=playerid)
		{
			LOG.lprintf("soon bad job type %i at %i,%i player %i, jobc %i, owner of workplace %i \n",it->job,it->workplace->GetX(),it->workplace->GetY(),playerid,c,it->workplace->GetPlayer());
		}			
		if(it->workplace->GetX()<0||it->workplace->GetX()>300||it->workplace->GetY()<0||it->workplace->GetY()>300)
			LOG.lprintf("NOW HORRIBLE job needed type %i at %i,%i player %i, jobc %i \n",it->job,it->workplace->GetX(),it->workplace->GetY(),playerid,c);
	}*/

    // Holzfäller und Sägewerke zählen, -10 ftw
    unsigned woodcutter = buildings[BLD_WOODCUTTER - 10].size();
    unsigned sawmills = buildings[BLD_SAWMILL - 10].size();

    // Wenn nötig, Notfallprogramm auslösen
    if ((boards <= 10 || stones <= 10) && (woodcutter == 0 || sawmills == 0) && (!isDefeated()))
    {
        if (!emergency)
        {
            emergency = true;
            if(GameClient::inst().GetPlayerID() == this->playerid)
                GAMECLIENT.SendPostMessage(new PostMsg(_("The emergency program has been activated."), PMC_GENERAL));
        }
    }
    else
    {
        // Sobald Notfall vorbei, Notfallprogramm beenden, evtl. Baustellen wieder mit Kram versorgen
        if (emergency)
        {
            emergency = false;
            if(GameClient::inst().GetPlayerID() == this->playerid)
                GAMECLIENT.SendPostMessage(new PostMsg(_("The emergency program has been deactivated."), PMC_GENERAL));
            FindMaterialForBuildingSites();
        }
    }
}

/// Testet die Bündnisse, ob sie nicht schon abgelaufen sind
void GameClientPlayer::TestPacts()
{
    for(unsigned i = 0; i < GameClient::inst().GetPlayerCount(); ++i)
    {
        // Wenn wir merken, dass der Friedensvertrag abgelaufen ist, berechnen wir die Sichtbarkeiten neu
        if(GetPactState(TREATY_OF_ALLIANCE, i) == NO_PACT && pacts[i][TREATY_OF_ALLIANCE].duration != 0)
        {
            pacts[i][TREATY_OF_ALLIANCE].duration = 0;
            if(GameClient::inst().GetPlayerID() == playerid)
            {
                // Ggf. den GUI Bescheid sagen, um Sichtbarkeiten etc. neu zu berechnen
                if(gwg->GetGameInterface())
                    gwg->GetGameInterface()->GI_TreatyOfAllianceChanged();
            }
        }
    }
}


bool GameClientPlayer::CanBuildCatapult() const
{
    // Wenn ADDON_LIMIT_CATAPULTS nicht aktiv ist, bauen immer erlaubt
    if(!GameClient::inst().GetGGS().isEnabled(ADDON_LIMIT_CATAPULTS))
        return true;

    BuildingCount bc;
    GetBuildingCount(bc);

    unsigned int max = 0;

    // proportional?
    if(GameClient::inst().GetGGS().getSelection(ADDON_LIMIT_CATAPULTS) == 1)
    {
        max = int(bc.building_counts[BLD_BARRACKS] * 0.125 +
                  bc.building_counts[BLD_GUARDHOUSE] * 0.25 +
                  bc.building_counts[BLD_WATCHTOWER] * 0.5 +
                  bc.building_counts[BLD_FORTRESS] + 0.111); // to avoid rounding errors
    }
    else
    {
        const unsigned int limits[6] = { 0, 3, 5, 10, 20, 30};
        max = limits[GameClient::inst().GetGGS().getSelection(ADDON_LIMIT_CATAPULTS) - 2];
    }

    if(bc.building_counts[BLD_CATAPULT] + bc.building_site_counts[BLD_CATAPULT] >= max)
        return false;
    else
        return true;
}

/// A ship has discovered new hostile territory --> determines if this is new
/// i.e. there is a sufficient distance to older locations
/// Returns true if yes and false if not
bool GameClientPlayer::ShipDiscoveredHostileTerritory(const Point<MapCoord> location)
{
    // Prüfen, ob Abstand zu bisherigen Punkten nicht zu klein
    for(unsigned i = 0; i < enemies_discovered_by_ships.size(); ++i)
    {
        if(gwg->CalcDistance(enemies_discovered_by_ships[i].x, enemies_discovered_by_ships[i].y, location.x, location.y) < 30)
            return false;
    }


    // Nein? Dann haben wir ein neues Territorium gefunden
    enemies_discovered_by_ships.push_back(location);

    return true;
}

/// For debug only
bool GameClientPlayer::CheckDependentFigure(noFigure* fig)
{
    for(std::list<nobBaseWarehouse*>::iterator it = warehouses.begin(); it != warehouses.end(); ++it)
    {
        if((*it)->CheckDependentFigure(fig))
            return true;
    }
    return false;
}

/// Get available wares/figures which can THIS player (usually ally of wh->player) send to warehouse wh
unsigned GameClientPlayer::GetAvailableWaresForTrading(nobBaseWarehouse* wh, const GoodType gt, const Job job) const
{
    unsigned count = 0;

    for(std::list<nobBaseWarehouse*>::const_iterator it = warehouses.begin(); it != warehouses.end(); ++it)
    {
        // Find a trade path from this warehouse to wh?
        TradeRoute* tr;
        gwg->CreateTradeRoute((*it)->GetPos(), wh->GetPos(), playerid, &tr);

        // Found a path?
        if(tr->GetNextDir() != 0xff)
        {
            // Then consider this warehouse
            if(gt != GD_NOTHING)
                count += (*it)->GetAvailableWaresForTrading(gt);
            else if(job != JOB_NOTHING)
                count += (*it)->GetAvailableFiguresForTrading(job);
        }
        delete tr;
    }

    return count;

}

struct WarehouseDistanceComperator
{
    // Reference warehouse, to which we want to calc the distance
    static nobBaseWarehouse* ref;
    /// GameWorld
    static GameWorldGame* gwg;

    /// Set Parameters
    static void SetParameters(nobBaseWarehouse* ref, GameWorldGame* gwg)
    {
        WarehouseDistanceComperator::ref = ref;
        WarehouseDistanceComperator::gwg = gwg;
    }


    static bool Compare(nobBaseWarehouse* const wh1, nobBaseWarehouse* const wh2)
    {
        return (gwg->CalcDistance(wh1->GetPos(), ref->GetPos()) < gwg->CalcDistance(wh2->GetPos(), ref->GetPos()));
    }
};

nobBaseWarehouse* WarehouseDistanceComperator::ref;
GameWorldGame* WarehouseDistanceComperator::gwg;

/// Send wares to warehouse wh
void GameClientPlayer::Trade(nobBaseWarehouse* wh, const GoodType gt, const Job job, unsigned count) const
{
    if(count < 1) //block trolling player who wants to send empty tradecaravan, sending packdonkeys is forbidden because it crashes the game
        return;
    std::list<nobBaseWarehouse*> whs(warehouses);
    WarehouseDistanceComperator::SetParameters(wh, gwg);
    whs.sort(WarehouseDistanceComperator::Compare);
    for(std::list<nobBaseWarehouse*>::const_iterator it = warehouses.begin(); it != warehouses.end(); ++it)
    {
        // Find a trade path from this warehouse to wh?
        TradeRoute* tr;
        gwg->CreateTradeRoute((*it)->GetFlag()->GetPos(), wh->GetFlag()->GetPos(), playerid, &tr);

        // Found a path?
        if(tr->IsValid())
        {
            unsigned available = 0;
            // Then consider this warehouse
            if(gt != GD_NOTHING)
                available = (*it)->GetAvailableWaresForTrading(gt);
            else if(job != JOB_NOTHING)
                available = (*it)->GetAvailableFiguresForTrading(job);

            available = min(available, count);
            count -= available;
            if(available > 0) //we dont have anything to send .. so dont start a new traderoute from here!
                (*it)->StartTradeCaravane(gt, job, available, *tr, wh);


        }
        delete tr;

        if(!count)
            return;
    }

}

