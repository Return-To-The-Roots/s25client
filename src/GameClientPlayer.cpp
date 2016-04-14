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
#include "GameClientPlayer.h"
#include "GameClient.h"
#include "Random.h"
#include "PostMsg.h"
#include "RoadSegment.h"
#include "Ware.h"

#include "nodeObjs/noFlag.h"
#include "buildings/noBuildingSite.h"
#include "buildings/nobUsual.h"
#include "buildings/nobMilitary.h"
#include "figures/nofFlagWorker.h"
#include "figures/nofCarrier.h"
#include "nodeObjs/noShip.h"
#include "buildings/nobHarborBuilding.h"
#include "FindWhConditions.h"
#include "gameData/MilitaryConsts.h"
#include "gameData/ShieldConsts.h"

#include "GameInterface.h"
#include "gameData/PlayerConsts.h"
#include "gameTypes/BuildingTypes.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/JobTypes.h"
#include "gameTypes/PactTypes.h"
#include "gameTypes/MessageTypes.h"
#include "world/TradeRoute.h"
#include "SerializedGameData.h"
#include "pathfinding/RoadPathFinder.h"
#include "TradePathCache.h"
#include "libutil/src/Log.h"
#include <stdint.h>
#include <limits>

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

// Standardbelegung der Transportreihenfolge festlegen
const boost::array<unsigned char, WARE_TYPES_COUNT> STD_TRANSPORT =
{{
    2, 12, 12, 12, 12, 12, 12, 12, 12, 12, 10, 10, 12, 12, 12, 13, 1, 3, 11, 11, 11, 1, 9, 7, 8, 1, 1, 11, 0, 4, 5, 6, 11, 11, 1
}};

GameClientPlayer::GameClientPlayer(const unsigned playerid):
		GamePlayerInfo(playerid), gwg(NULL), is_lagging(false),
		hqPos(MapPoint::Invalid()), emergency(false)
{
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
    Distributions& visDistribution = GAMECLIENT.visual_settings.distribution; //-V807
    visDistribution[0] = distribution[GD_FISH].percent_buildings[BLD_GRANITEMINE] = 3;
    visDistribution[1] = distribution[GD_FISH].percent_buildings[BLD_COALMINE] = 5;
    visDistribution[2] = distribution[GD_FISH].percent_buildings[BLD_IRONMINE] = 7;
    visDistribution[3] = distribution[GD_FISH].percent_buildings[BLD_GOLDMINE] = 10;

    visDistribution[4] = distribution[GD_GRAIN].percent_buildings[BLD_MILL] = 5;
    visDistribution[5] = distribution[GD_GRAIN].percent_buildings[BLD_PIGFARM] = 3;
    visDistribution[6] = distribution[GD_GRAIN].percent_buildings[BLD_DONKEYBREEDER] = 2;
    visDistribution[7] = distribution[GD_GRAIN].percent_buildings[BLD_BREWERY] = 3;
    visDistribution[8] = distribution[GD_GRAIN].percent_buildings[BLD_CHARBURNER] = 3;

    visDistribution[9] = distribution[GD_IRON].percent_buildings[BLD_ARMORY] = 8;
    visDistribution[10] = distribution[GD_IRON].percent_buildings[BLD_METALWORKS] = 4;

    visDistribution[11] = distribution[GD_COAL].percent_buildings[BLD_ARMORY] = 8;
    visDistribution[12] = distribution[GD_COAL].percent_buildings[BLD_IRONSMELTER] = 7;
    visDistribution[13] = distribution[GD_COAL].percent_buildings[BLD_MINT] = 10;

    visDistribution[14] = distribution[GD_WOOD].percent_buildings[BLD_SAWMILL] = 8;
    visDistribution[15] = distribution[GD_WOOD].percent_buildings[BLD_CHARBURNER] = 3;

    visDistribution[16] = distribution[GD_BOARDS].percent_buildings[BLD_HEADQUARTERS] = 10;
    visDistribution[17] = distribution[GD_BOARDS].percent_buildings[BLD_METALWORKS] = 4;
    visDistribution[18] = distribution[GD_BOARDS].percent_buildings[BLD_SHIPYARD] = 2;

    visDistribution[19] = distribution[GD_WATER].percent_buildings[BLD_BAKERY] = 6;
    visDistribution[20] = distribution[GD_WATER].percent_buildings[BLD_BREWERY] = 3;
    visDistribution[21] = distribution[GD_WATER].percent_buildings[BLD_PIGFARM] = 2;
    visDistribution[22] = distribution[GD_WATER].percent_buildings[BLD_DONKEYBREEDER] = 3;

    RecalcDistribution();

    GAMECLIENT.visual_settings.order_type = orderType_ = 0;

    // Baureihenfolge füllen (0 ist das HQ!)
    for(unsigned char i = 1, j = 0; i < BLD_COUNT; ++i)
    {
        // Diese Ids sind noch nicht besetzt
        if(
              i == BLD_NOTHING2 ||
              i == BLD_NOTHING3 ||
              i == BLD_NOTHING4 ||
              i == BLD_NOTHING5 ||
              i == BLD_NOTHING6 ||
              i == BLD_NOTHING7 ||
              i == BLD_NOTHING9
          )
            continue;

        GAMECLIENT.visual_settings.build_order[j] = build_order[j] = i;
        ++j;
    }

    // Transportreihenfolge festlegen
    transport = STD_TRANSPORT;

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

    // military settings (military-window-slider, in 10th percent)
    militarySettings_[0] = 10; //-V525
    militarySettings_[1] = 3;
    militarySettings_[2] = 5;
    militarySettings_[3] = 3;
    militarySettings_[4] = 2;
    militarySettings_[5] = 4;
    militarySettings_[6] = MILITARY_SETTINGS_SCALE[6];
    militarySettings_[7] = MILITARY_SETTINGS_SCALE[7];
    GAMECLIENT.visual_settings.military_settings = militarySettings_;

    // metalwork tool request

    // manually
    for (unsigned i = 0; i < TOOL_COUNT; ++i)
    {
        tools_ordered[i] = 0;
        tools_ordered_delta[i] = 0;
    }

    // percentage (tool-settings-window-slider, in 10th percent)
    toolsSettings_[0]  = 1;
    toolsSettings_[1]  = 4;
    toolsSettings_[2]  = 2;
    toolsSettings_[3]  = 5;
    toolsSettings_[4]  = 7;
    toolsSettings_[5]  = 1;
    toolsSettings_[6]  = 3;
    toolsSettings_[7]  = 1;
    toolsSettings_[8]  = 2;
    toolsSettings_[9]  = 1;
    toolsSettings_[10] = 2;
    toolsSettings_[11] = 1;
    GAMECLIENT.visual_settings.tools_settings = toolsSettings_;

    // Standardeinstellungen kopieren
    GAMECLIENT.default_settings = GAMECLIENT.visual_settings;

    defenders_pos = 0;
    for(unsigned i = 0; i < 5; ++i)
        defenders[i] = true;

    // Inventur nullen
    global_inventory.clear();

    // Statistiken mit 0en füllen
    memset(&statistic[STAT_15M], 0, sizeof(statistic[STAT_15M]));
    memset(&statistic[STAT_1H], 0, sizeof(statistic[STAT_1H]));
    memset(&statistic[STAT_4H], 0, sizeof(statistic[STAT_4H]));
    memset(&statistic[STAT_16H], 0, sizeof(statistic[STAT_16H]));
    memset(&statisticCurrentData, 0, sizeof(statisticCurrentData));
    memset(&statisticCurrentMerchandiseData, 0, sizeof(statisticCurrentMerchandiseData));
}

void GameClientPlayer::Serialize(SerializedGameData& sgd)
{
    // PlayerStatus speichern, ehemalig
    sgd.PushUnsignedChar(static_cast<unsigned char>(ps));

    // Nur richtige Spieler serialisieren
    if(!(ps == PS_OCCUPIED || ps == PS_KI))
        return;

    sgd.PushObjectContainer(warehouses, false);
    sgd.PushObjectContainer(harbors, true);

    //sgd.PushObjectContainer(unoccupied_roads,true);
    sgd.PushObjectContainer(roads, true);

    sgd.PushUnsignedInt(jobs_wanted.size());
    for(std::list<JobNeeded>::iterator it = jobs_wanted.begin(); it != jobs_wanted.end(); ++it)
    {
        sgd.PushUnsignedChar(it->job);
        sgd.PushObject(it->workplace, false);
    }

    for(unsigned i = 0; i < 30; ++i)
        sgd.PushObjectContainer(buildings[i], true);

    sgd.PushObjectContainer(building_sites, true);

    sgd.PushObjectContainer(military_buildings, true);

    sgd.PushObjectContainer(ware_list, true);

    sgd.PushObjectContainer(flagworkers, false);

    sgd.PushObjectContainer(ships, true);

    for(unsigned i = 0; i < 5; ++i)
        sgd.PushBool(defenders[i]);
    sgd.PushUnsignedShort(defenders_pos);

    sgd.PushMapPoint(hqPos);

    for(unsigned i = 0; i < WARE_TYPES_COUNT; ++i)
    {
        for (unsigned bldType = 0; bldType < BUILDING_TYPES_COUNT; ++bldType)
        {
            sgd.PushUnsignedChar(distribution[i].percent_buildings[bldType]);
        }
        sgd.PushUnsignedInt(distribution[i].client_buildings.size());
        for(std::list<BuildingType>::iterator it = distribution[i].client_buildings.begin(); it != distribution[i].client_buildings.end(); ++it)
            sgd.PushUnsignedChar(*it);
        sgd.PushUnsignedInt(unsigned(distribution[i].goals.size()));
        for(unsigned z = 0; z < distribution[i].goals.size(); ++z)
            sgd.PushUnsignedChar(distribution[i].goals[z]);
        sgd.PushUnsignedInt(distribution[i].selected_goal);
    }

    sgd.PushUnsignedChar(orderType_);

    for(unsigned i = 0; i < build_order.size(); ++i)
        sgd.PushUnsignedChar(build_order[i]);

    sgd.PushRawData(transport.elems, transport.size());

    for(unsigned i = 0; i < MILITARY_SETTINGS_COUNT; ++i)
        sgd.PushUnsignedChar(militarySettings_[i]);

    for(unsigned i = 0; i < 12; ++i)
        sgd.PushUnsignedChar(toolsSettings_[i]);

    //qx:tools
    for (unsigned i = 0; i < TOOL_COUNT; ++i)
        sgd.PushUnsignedChar(tools_ordered[i]);

    for(unsigned i = 0; i < WARE_TYPES_COUNT; ++i)
        sgd.PushUnsignedInt(global_inventory.goods[i]);
    for(unsigned i = 0; i < JOB_TYPES_COUNT; ++i)
        sgd.PushUnsignedInt(global_inventory.people[i]);

    // für Statistik
    for (unsigned i = 0; i < STAT_TIME_COUNT; ++i)
    {
        // normale Statistik
        for (unsigned j = 0; j < STAT_TYPE_COUNT; ++j)
            for (unsigned k = 0; k < STAT_STEP_COUNT; ++k)
                sgd.PushUnsignedInt(statistic[i].data[j][k]);

        // Warenstatistik
        for (unsigned j = 0; j < STAT_MERCHANDISE_TYPE_COUNT; ++j)
            for (unsigned k = 0; k < STAT_STEP_COUNT; ++k)
                sgd.PushUnsignedShort(statistic[i].merchandiseData[j][k]);

        sgd.PushUnsignedShort(statistic[i].currentIndex);
        sgd.PushUnsignedShort(statistic[i].counter);
    }
    for (unsigned i = 0; i < STAT_TYPE_COUNT; ++i)
        sgd.PushUnsignedInt(statisticCurrentData[i]);

    for (unsigned i = 0; i < STAT_MERCHANDISE_TYPE_COUNT; ++i)
        sgd.PushUnsignedShort(statisticCurrentMerchandiseData[i]);

    // Serialize Pacts:
    for (unsigned i = 0; i < MAX_PLAYERS; ++i)
    {
        for (unsigned u = 0; u < PACTS_COUNT; ++u)
        {
            pacts[i][u].Serialize(sgd);
        }
    }

    sgd.PushBool(emergency);
}

void GameClientPlayer::Deserialize(SerializedGameData& sgd)
{
    // Ehemaligen PS auslesen
    PlayerState origin_ps = PlayerState(sgd.PopUnsignedChar());
    // Nur richtige Spieler serialisieren
    if(!(origin_ps == PS_OCCUPIED || origin_ps == PS_KI))
        return;

    sgd.PopObjectContainer(warehouses, GOT_UNKNOWN);
    sgd.PopObjectContainer(harbors, GOT_NOB_HARBORBUILDING);

    //sgd.PopObjectContainer(unoccupied_roads,GOT_ROADSEGMENT);
    sgd.PopObjectContainer(roads, GOT_ROADSEGMENT);

    unsigned list_size = sgd.PopUnsignedInt();
    for(unsigned i = 0; i < list_size; ++i)
    {
        JobNeeded nj;
        nj.job = Job(sgd.PopUnsignedChar());
        nj.workplace = sgd.PopObject<noRoadNode>(
                           GOT_UNKNOWN);
        jobs_wanted.push_back(nj);

    }

    for(unsigned i = 0; i < 30; ++i)
        sgd.PopObjectContainer(buildings[i], GOT_NOB_USUAL);

    sgd.PopObjectContainer(building_sites, GOT_BUILDINGSITE);

    sgd.PopObjectContainer(military_buildings, GOT_NOB_MILITARY);

    sgd.PopObjectContainer(ware_list, GOT_WARE);

    sgd.PopObjectContainer(flagworkers, GOT_UNKNOWN);

    sgd.PopObjectContainer(ships, GOT_SHIP);

    for(unsigned i = 0; i < 5; ++i)
        defenders[i] = sgd.PopBool();
    defenders_pos = sgd.PopUnsignedShort();

    hqPos = sgd.PopMapPoint();

    for(unsigned i = 0; i < WARE_TYPES_COUNT; ++i)
    {
        for (unsigned bldType = 0; bldType < BUILDING_TYPES_COUNT; ++bldType)
        {
            distribution[i].percent_buildings[bldType] = sgd.PopUnsignedChar();
        }
        list_size = sgd.PopUnsignedInt();
        for(unsigned z = 0; z < list_size; ++z)
            distribution[i].client_buildings.push_back(BuildingType(sgd.PopUnsignedChar()));
        unsigned goal_count = sgd.PopUnsignedInt();
        distribution[i].goals.resize(goal_count);
        for(unsigned z = 0; z < goal_count; ++z)
            distribution[i].goals[z] = sgd.PopUnsignedChar();
        distribution[i].selected_goal = sgd.PopUnsignedInt();
    }

    orderType_ = sgd.PopUnsignedChar();

    for(unsigned i = 0; i < build_order.size(); ++i)
        build_order[i] = sgd.PopUnsignedChar();

    sgd.PopRawData(transport.elems, transport.size());

    for(unsigned i = 0; i < MILITARY_SETTINGS_COUNT; ++i)
        militarySettings_[i] = sgd.PopUnsignedChar();

    for(unsigned i = 0; i < 12; ++i)
        toolsSettings_[i] = sgd.PopUnsignedChar();

    // qx:tools
    for (unsigned i = 0; i < TOOL_COUNT; ++i)
        tools_ordered[i] = sgd.PopUnsignedChar();
    for (unsigned i = 0; i < TOOL_COUNT; ++i)
        tools_ordered_delta[i] = 0;

    for(unsigned i = 0; i < WARE_TYPES_COUNT; ++i)
        global_inventory.goods[i] = sgd.PopUnsignedInt();
    for(unsigned i = 0; i < JOB_TYPES_COUNT; ++i)
        global_inventory.people[i] = sgd.PopUnsignedInt();

    // Visuelle Einstellungen festlegen

    // für Statistik
    for (unsigned i = 0; i < STAT_TIME_COUNT; ++i)
    {
        // normale Statistik
        for (unsigned j = 0; j < STAT_TYPE_COUNT; ++j)
            for (unsigned k = 0; k < STAT_STEP_COUNT; ++k)
                statistic[i].data[j][k] = sgd.PopUnsignedInt();

        // Warenstatistik
        for (unsigned j = 0; j < STAT_MERCHANDISE_TYPE_COUNT; ++j)
            for (unsigned k = 0; k < STAT_STEP_COUNT; ++k)
                statistic[i].merchandiseData[j][k] = sgd.PopUnsignedShort();

        statistic[i].currentIndex = sgd.PopUnsignedShort();
        statistic[i].counter = sgd.PopUnsignedShort();
    }
    for (unsigned i = 0; i < STAT_TYPE_COUNT; ++i)
        statisticCurrentData[i] = sgd.PopUnsignedInt();

    for (unsigned i = 0; i < STAT_MERCHANDISE_TYPE_COUNT; ++i)
        statisticCurrentMerchandiseData[i] = sgd.PopUnsignedShort();

    // Deserialize Pacts:
    for (unsigned i = 0; i < MAX_PLAYERS; ++i)
    {
        for (unsigned u = 0; u < PACTS_COUNT; ++u)
        {
            pacts[i][u] = GameClientPlayer::Pact(sgd);
        }
    }

    emergency = sgd.PopBool();
}

template<class T_IsWarehouseGood>
nobBaseWarehouse* GameClientPlayer::FindWarehouse(const noRoadNode& start, const T_IsWarehouseGood& isWarehouseGood, const bool to_wh, const bool use_boat_roads,
    unsigned* const length, const RoadSegment* const forbidden, bool record) const
{
    nobBaseWarehouse* best = NULL;

    unsigned tlength = 0xFFFFFFFF, best_length = 0xFFFFFFFF;

    for(std::list<nobBaseWarehouse*>::const_iterator itWh = warehouses.begin(); itWh != warehouses.end(); ++itWh)
    {
        // Lagerhaus geeignet?
        RTTR_Assert(*itWh);
        nobBaseWarehouse& wh = **itWh;
        if(!isWarehouseGood(wh))
            continue;

        if(start.GetPos() == wh.GetPos())
        {
            // We are already there -> Take it
            if(length)
                *length = 0;
            return &wh;
        }

		//now check if there is at least a chance that the next wh is closer than current best because pathfinding takes time
		if(gwg->CalcDistance(start.GetPos(), wh.GetPos()) > best_length)
			continue;
        // Bei der erlaubten Benutzung von Bootsstraßen Waren-Pathfinding benutzen wenns zu nem Lagerhaus gehn soll start <-> ziel tauschen bei der wegfindung
        if(gwg->GetRoadPathFinder().FindPath(to_wh ? start : wh, to_wh ? wh : start, record, use_boat_roads, best_length, forbidden, &tlength))
        {
            if(tlength < best_length || !best)
            {
                best_length = tlength;
                best = &wh;
            }
        }
    }

    if(length)
        *length = best_length;

    return best;
}

void GameClientPlayer::NewRoad(RoadSegment* const rs)
{
    // Zu den Straßen hinzufgen, da's ja ne neue ist
    roads.push_back(rs);

    // Alle Straßen müssen nun gucken, ob sie einen Weg zu einem Warehouse finden
    FindWarehouseForAllRoads();

    // Alle Straßen müssen gucken, ob sie einen Esel bekommen können
    for(std::list<RoadSegment*>::iterator it = roads.begin(); it != roads.end(); ++it)
        (*it)->TryGetDonkey();

    // Alle Arbeitsplätze müssen nun gucken, ob sie einen Weg zu einem Lagerhaus mit entsprechender Arbeitskraft finden
    FindWarehouseForAllJobs(JOB_NOTHING);

    // Alle Baustellen müssen nun gucken, ob sie ihr benötigtes Baumaterial bekommen (evtl war vorher die Straße zum Lagerhaus unterbrochen
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
            if((*it)->FindRouteToWarehouse() && (*it)->IsWaitingAtFlag())
                (*it)->CallCarrier();
        }
    }
}

void GameClientPlayer::RoadDestroyed()
{
    // Alle Waren, die an Flagge liegen und in Lagerhäusern, müssen gucken, ob sie ihr Ziel noch erreichen können, jetzt wo eine Straße fehlt
    for(std::list<Ware*>::iterator it = ware_list.begin(); it != ware_list.end(); )
    {
        Ware* ware = *it;
        if(ware->IsWaitingAtFlag()) // Liegt die Flagge an einer Flagge, muss ihr Weg neu berechnet werden
        {
			unsigned char last_next_dir = ware->GetNextDir();
			ware->RecalcRoute();			
			//special case: ware was lost some time ago and the new goal is at this flag and not a warehouse,hq,harbor and the "flip-route" picked so a carrier would pick up the ware carry it away from goal then back and drop
			//it off at the goal was just destroyed? -> try to pick another flip route or tell the goal about failure.
            noRoadNode& wareLocation = *ware->GetLocation();
            noBaseBuilding* wareGoal = ware->GetGoal();
			if(wareGoal && ware->GetNextDir()==1 && wareLocation.GetPos() == wareGoal->GetFlag()->GetPos() && ((wareGoal->GetBuildingType()!=BLD_STOREHOUSE && wareGoal->GetBuildingType()!=BLD_HEADQUARTERS && wareGoal->GetBuildingType()!=BLD_HARBORBUILDING) || wareGoal->GetType()==NOP_BUILDINGSITE))
			{
				//LOG.lprintf("road destroyed special at %i,%i gf: %u \n", (*it)->GetLocation()->GetX(),(*it)->GetLocation()->GetY(),GAMECLIENT.GetGFNumber());
				unsigned gotfliproute = 1;
				for(unsigned i=2; i<7; i++)
				{
					if(wareLocation.routes[i%6])
					{
						gotfliproute = i;
						break;
					}
				}
				if(gotfliproute != 1)
				{
					ware->SetNextDir(gotfliproute%6);
				}
				else //no route to goal -> notify goal, try to send ware to a warehouse
				{
					ware->NotifyGoalAboutLostWare();
                    ware->FindRouteToWarehouse();
				}
			}
			//end of special case

			// notify carriers/flags about news if there are any
			if(ware->GetNextDir() != last_next_dir)
            {
                //notify current flag that transport in the old direction might not longer be required
                ware->RemoveWareJobForDir(last_next_dir);
				if(ware->GetNextDir() != 0xFF)
                    ware->CallCarrier();
            }
		}
        else if(ware->IsWaitingInWarehouse())
        {
            if(!ware->IsRouteToGoal())
            {
                Ware* ware = *it;

                // Ware aus der Warteliste des Lagerhauses entfernen
                static_cast<nobBaseWarehouse*>(ware->GetLocation())->CancelWare(ware);
                // Das Ziel wird nun nich mehr beliefert
                ware->NotifyGoalAboutLostWare();
                // Ware aus der Liste raus
                it = ware_list.erase(it);
                // And trash it
                deletePtr(ware);
                continue;
            }
        }
        else if(ware->IsWaitingForShip())
        {
            // Weg neu berechnen
            ware->RecalcRoute();
        }

        ++it;
    }

    // Alle Häfen müssen ihre Figuren den Weg überprüfen lassen
    for(std::list<nobHarborBuilding*>::iterator it = harbors.begin(); it != harbors.end(); ++it)
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
    RTTR_Assert(rs->GetF1() != NULL && rs->GetF2() != NULL);
    boost::array<unsigned, 2> length;
    boost::array<nobBaseWarehouse*, 2> best;

    // Braucht der ein Boot?
    if(rs->GetRoadType() == RoadSegment::RT_BOAT)
    {
        // dann braucht man Träger UND Boot
        best[0] = FindWarehouse(*rs->GetF1(), FW::HasWareAndFigure(GD_BOAT, JOB_HELPER, false), false, false, &length[0], rs);
        // 2. Flagge des Weges
        best[1] = FindWarehouse(*rs->GetF2(), FW::HasWareAndFigure(GD_BOAT, JOB_HELPER, false), false, false, &length[1], rs);
    }
    else
    {
        // 1. Flagge des Weges
        best[0] = FindWarehouse(*rs->GetF1(), FW::HasFigure(JOB_HELPER, false), false, false, &length[0], rs);
        // 2. Flagge des Weges
        best[1] = FindWarehouse(*rs->GetF2(), FW::HasFigure(JOB_HELPER, false), false, false, &length[1], rs);
    }

    // überhaupt nen Weg gefunden?
    // Welche Flagge benutzen?
    if(best[0] && (!best[1] || length[0] < length[1]))
        best[0]->OrderCarrier(*rs->GetF1(), *rs);
    else if(best[1])
        best[1]->OrderCarrier(*rs->GetF2(), *rs);
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
    for(std::list<JobNeeded>::iterator it = jobs_wanted.begin(); it != jobs_wanted.end(); )
    {
        if(it->workplace == workplace)
        {
            it = jobs_wanted.erase(it);
            if(!all)
                return;
        }
        else
        {
            ++it;
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
    nobBaseWarehouse* wh = FindWarehouse(*goal, FW::HasFigure(job, true), false, false);

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
    nobBaseWarehouse* wh = FindWarehouse(*goal, FW::HasMinWares(ware, 1), false, true);

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
                return NULL;
        }
    }	
    else //no warehouse can deliver the ware -> check all our wares for lost wares that might match the order
	{
		unsigned bestLength = 0xFFFFFFFF;
		Ware* bestWare = NULL;
		for(std::list<Ware*>::iterator it = ware_list.begin(); it != ware_list.end(); ++it)
		{
			if((*it)->IsLostWare() && (*it)->type==ware)
			{
                //got a lost ware with a road to goal -> find best
                unsigned curLength = (*it)->CheckNewGoalForLostWare(goal);
				if(curLength < bestLength)
				{
					bestLength = curLength;
					bestWare = (*it);	
				}
			}
		}
		if(bestWare)
		{
			bestWare->SetNewGoalForLostWare(goal);
			return bestWare;
		}
	}
	return NULL;
}

nofCarrier* GameClientPlayer::OrderDonkey(RoadSegment* road)
{
    unsigned length[2];
    nobBaseWarehouse* best[2];

    // 1. Flagge des Weges
    best[0] = FindWarehouse(*road->GetF1(), FW::HasFigure(JOB_PACKDONKEY, false), false, false, &length[0], road);
    // 2. Flagge des Weges
    best[1] = FindWarehouse(*road->GetF2(), FW::HasFigure(JOB_PACKDONKEY, false), false, false, &length[1], road);

    // überhaupt nen Weg gefunden?
    // Welche Flagge benutzen?
    if(best[0] && (!best[1] || length[0] < length[1]))
        return best[0]->OrderDonkey(road, road->GetF1());
    else if(best[1])
        return best[1]->OrderDonkey(road, road->GetF2());
    else
        return NULL;
}

RoadSegment* GameClientPlayer::FindRoadForDonkey(noRoadNode* start, noRoadNode** goal)
{
    // Bisher höchste Trägerproduktivität und die entsprechende Straße dazu
    unsigned best_productivity = 0;
    RoadSegment* best_road = NULL;
    // Beste Flagge dieser Straße
    *goal = NULL;

    for(std::list<RoadSegment*>::iterator it = roads.begin(); it != roads.end(); ++it)
    {
        // Braucht die Straße einen Esel?
        if((*it)->NeedDonkey())
        {
            // Beste Flagge von diesem Weg, und beste Wegstrecke
            noRoadNode* current_best_goal = 0;
            // Weg zu beiden Flaggen berechnen
            unsigned length1, length2;
            bool isF1Reachable = gwg->FindHumanPathOnRoads(*start, *(*it)->GetF1(), &length1, NULL, *it) != 0xFF;
            bool isF2Reachable = gwg->FindHumanPathOnRoads(*start, *(*it)->GetF2(), &length2, NULL, *it) != 0xFF;

            // Wenn man zu einer Flagge nich kommt, die jeweils andere nehmen
            if(!isF1Reachable)
                current_best_goal = (isF2Reachable) ? (*it)->GetF2() : 0;
            else if(!isF2Reachable)
                current_best_goal = (isF1Reachable) ? (*it)->GetF1() : 0;
            else
            {
                // ansonsten die kürzeste von beiden
                current_best_goal = (length1 < length2) ? (*it)->GetF1() : (*it)->GetF2();
            }

            // Kein Weg führt hin, nächste Straße bitte
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
                // Dann wird der vom Thron gestoßen
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

    bool operator<(const ClientForWare& b) const
    {
		// use estimate, points and absolute location (in that order) for sorting
		// absolute location must be different -> tiebreaker in case everything else is the same
        if (estimate != b.estimate)
        {
			return(estimate > b.estimate);
        }
		else
		{
			if (points != b.points)
			{
				return(points > b.points);
			}
			else
			{
				if (bb->GetX() != b.bb->GetX())
					return bb->GetX() < b.bb->GetX();
				else
					return bb->GetY() < b.bb->GetY();
			}
		}
    }
};

noBaseBuilding* GameClientPlayer::FindClientForWare(Ware* ware)
{
    // Wenn es eine Goldmünze ist, wird das Ziel auf eine andere Art und Weise berechnet
    if(ware->type == GD_COINS)
        return FindClientForCoin(ware);

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
        for(std::list<nobHarborBuilding*>::const_iterator it = harbors.begin(); it != harbors.end(); ++it)
        {
            points = (*it)->CalcDistributionPoints(gt);

            if (points)
            {
                points += 10 * 30; // Verteilung existiert nicht, Expeditionen haben allerdings hohe Priorität

                unsigned distance = gwg->CalcDistance(start->GetPos(), (*it)->GetPos()) / 2;
                cfw.push_back(ClientForWare(*it, points > distance ? points - distance : 0, points));
            }
        }
    }

    for(std::list<BuildingType>::const_iterator it = distribution[gt_clients].client_buildings.begin(); it != distribution[gt_clients].client_buildings.end(); ++it)
    {
        // BLD_HEADQUARTERS sind Baustellen!!, da HQs ja sowieso nicht gebaut werden können
        if(*it == BLD_HEADQUARTERS)
        {
            // Bei Baustellen die Extraliste abfragen
            for(std::list<noBuildingSite*>::const_iterator i = building_sites.begin(); i != building_sites.end(); ++i)
            {
                points = (*i)->CalcDistributionPoints(ware->GetLocation(), gt);

                if (points)
                {
                    points += distribution[gt].percent_buildings[BLD_HEADQUARTERS] * 30;

                    unsigned distance = gwg->CalcDistance(start->GetPos(), (*i)->GetPos()) / 2;
                    cfw.push_back(ClientForWare(*i, points > distance ? points - distance : 0, points));
                }
            }
        }
        else
        {
            // Für übrige Gebäude
            for(std::list<nobUsual*>::const_iterator i = buildings[*it - 10].begin(); i != buildings[*it - 10].end(); ++i)
            {
                points = (*i)->CalcDistributionPoints(ware->GetLocation(), gt);

                // Wenn 0, dann braucht er die Ware nicht
                if (points)
                {
                    if (!distribution[gt].goals.empty())
                    {
                        if ((*i)->GetBuildingType() == static_cast<BuildingType>(distribution[gt].goals[distribution[gt].selected_goal]))
                            points += 300;
                        else if (points >= 300)   // avoid overflows (async!)
                            points -= 300;
                        else
                            points = 0;
                    }

                    unsigned distance = gwg->CalcDistance(start->GetPos(), (*i)->GetPos()) / 2;
                    cfw.push_back(ClientForWare(*i, points > distance ? points - distance : 0, points));
                }
            }
        }
    }

    // sort our clients, highest score first
    std::sort(cfw.begin(), cfw.end());

    noBaseBuilding* last_bb = NULL;
    noBaseBuilding* bb = NULL;
    unsigned best_points = 0;
    for (std::vector<ClientForWare>::iterator it = cfw.begin(); it != cfw.end(); ++it)
    {
        unsigned path_length;

        // If our estimate is worse (or equal) best_points, the real value cannot be better.
        // As our list is sorted, further entries cannot be better either, so stop searching.
        if (it->estimate <= best_points)
            break;

        // get rid of double building entries. TODO: why are there double entries!?
        if (it->bb == last_bb)
            continue;

        last_bb = it->bb;

        // Just to be sure no underflow happens...
        if(it->points < best_points + 1)
            continue;

        // Find path ONLY if it may be better. Pathfinding is limited to the worst path score that would lead to a better score.
        // This eliminates the worst case scenario where all nodes in a split road network would be hit by the pathfinding only
        // to conclude that there is no possible path.
        if (gwg->FindPathForWareOnRoads(*start, *it->bb, &path_length, NULL, (it->points - best_points) * 2 - 1) != 0xFF)
        {
            unsigned score = it->points - (path_length / 2);

            // As we have limited our pathfinding to take a maximum of (points - best_points) * 2 - 1 steps,
            // path_length / 2 can at most be points - best_points - 1, so the score will be greater than best_points. :)
            RTTR_Assert(score > best_points);

            best_points = score;
            bb = it->bb;
        }
    }

    if(bb && !distribution[gt].goals.empty())
        distribution[gt].selected_goal = (distribution[gt].selected_goal + 907) % unsigned(distribution[gt].goals.size());

    // Wenn kein Abnehmer gefunden wurde, muss es halt in ein Lagerhaus
    if(!bb)
        bb = FindWarehouseForWare(*ware);

    return bb;
}

nobBaseWarehouse* GameClientPlayer::FindWarehouseForWare(const Ware& ware) const
{
    // Check whs that collect this ware
    nobBaseWarehouse* wh = FindWarehouse(*ware.GetLocation(), FW::CollectsWare(ware.type), true, true);
    // If there is none, check those that accept it
    if(!wh)
    {
        // First find the ones, that do not send it right away (IMPORTANT: This avoids sending a ware to the wh that is sending the ware out)
        wh = FindWarehouse(*ware.GetLocation(), FW::AcceptsWareButNoSend(ware.type), true, true);
        // The others only if this fails
        if(!wh)
            wh = FindWarehouse(*ware.GetLocation(), FW::AcceptsWare(ware.type), true, true);
    }
    return wh;
}

nobBaseMilitary* GameClientPlayer::FindClientForCoin(Ware* ware) const
{
    nobBaseMilitary* bb = NULL;
    unsigned best_points = 0, points;

    // Militärgebäude durchgehen
    for(std::list<nobMilitary*>::const_iterator it = military_buildings.begin(); it != military_buildings.end(); ++it)
    {
        unsigned way_points;

        points = (*it)->CalcCoinsPoints();
        // Wenn 0, will er gar keine Münzen (Goldzufuhr gestoppt)
        if(points)
        {
            // Weg dorthin berechnen
            if(gwg->FindPathForWareOnRoads(*ware->GetLocation(), **it, &way_points) != 0xFF)
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
        bb = FindWarehouseForWare(*ware);

    return bb;
}

void GameClientPlayer::AddBuildingSite(noBuildingSite* building_site)
{
    building_sites.push_back(building_site);
}

void GameClientPlayer::RemoveBuildingSite(noBuildingSite* building_site)
{
    RTTR_Assert(helpers::contains(building_sites, building_site));
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
    RTTR_Assert(helpers::contains(buildings[building->GetBuildingType() - 10], building));
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
    RTTR_Assert(helpers::contains(military_buildings, building));
    military_buildings.remove(building);
    ChangeStatisticValue(STAT_BUILDINGS, -1);
    TestDefeat();
}

/// Gibt Liste von Gebäuden des Spieler zurück
const std::list<nobUsual*>& GameClientPlayer::GetBuildings(const BuildingType type) const
{
    RTTR_Assert(type >= 10);

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
    RTTR_Assert(productivities.size() == 40);

    for(unsigned i = 0; i < 30; ++i)
    {
        // Durschnittliche Produktivität errrechnen, indem man die Produktivitäten aller Gebäude summiert
        // und den Mittelwert bildet
        unsigned total_productivity = 0;

        for(std::list<nobUsual*>::iterator it = buildings[i].begin(); it != buildings[i].end(); ++it)
            total_productivity += (*it)->GetProductivity();

        if(!buildings[i].empty())
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
            total_productivity += (*it)->GetProductivity();

        if(!buildings[i].empty())
            total_count += buildings[i].size();
    }
    if (total_count == 0)
        total_count = 1;

    return total_productivity / total_count;
}


unsigned GameClientPlayer::GetBuidingSitePriority(const noBuildingSite* building_site)
{
    if(orderType_)
    {
        // Spezielle Reihenfolge

        // Typ in der Reihenfolge suchen und Position als Priorität zurückgeben
        for(unsigned i = 0; i < build_order.size(); ++i)
        {
            if(building_site->GetBuildingType() == static_cast<BuildingType>(build_order[i]))
                return i;
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
    RTTR_Assert(false);
    // We may want to multiply this value so don't return the absolute max value
    return std::numeric_limits<unsigned>::max() / 1000;
}

void GameClientPlayer::ConvertTransportData(const TransportOrders& transport_data)
{
    // Im Replay visulle Einstellungen auf die wirklichen setzen
    if(GAMECLIENT.IsReplayModeOn())
        GAMECLIENT.visual_settings.transport_order = transport_data;

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
        wh = FindWarehouse(*goal, FW::HasMinSoldiers(1), false, false);
        if(wh)
        {
            unsigned order_count = std::min(wh->GetSoldiersCount(), count);
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

/// Sucht für Soldaten ein neues Militärgebäude, als Argument wird Referenz auf die
/// entsprechende Soldatenanzahl im Lagerhaus verlangt
void GameClientPlayer::NewSoldiersAvailable(const unsigned& soldier_count)
{
    RTTR_Assert(soldier_count > 0);
    // solange laufen lassen, bis soldier_count = 0, d.h. der Soldat irgendwohin geschickt wurde
    // Zuerst nach unbesetzten Militärgebäude schauen
    for(std::list<nobMilitary*>::iterator it = military_buildings.begin(); it != military_buildings.end(); ++it)
    {
        if((*it)->IsNewBuilt())
        {
            (*it)->RegulateTroops();
            // Used that soldier? Go out
            if(!soldier_count)
                return;
        }
    }

    // Als nächstes Gebäude in Grenznähe
    for(std::list<nobMilitary*>::iterator it = military_buildings.begin(); it != military_buildings.end(); ++it)
    {
        if((*it)->GetFrontierDistance() == 2)
        {
            (*it)->RegulateTroops();
            // Used that soldier? Go out
            if(!soldier_count)
                return;
        }
    }

    // Und den Rest ggf.
    for(std::list<nobMilitary*>::iterator it = military_buildings.begin(); it != military_buildings.end(); ++it)
	{
		//already checked? -> skip
		if((*it)->GetFrontierDistance() == 2 || (*it)->IsNewBuilt())
			continue;
		(*it)->RegulateTroops();
		if(!soldier_count) //used the soldier?
			return;
	}

}

void GameClientPlayer::CallFlagWorker(const MapPoint pt, const Job job)
{
    noFlag* flag = gwg->GetSpecObj<noFlag>(pt);
    /// Find wh with given job type (e.g. geologist, scout, ...)
    nobBaseWarehouse* wh = FindWarehouse(*flag, FW::HasFigure(job, true), false, false);

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
    for(unsigned i = 0; i < defenders.size(); ++i)
        defenders[i] = (i < militarySettings_[2] * 5 / MILITARY_SETTINGS_SCALE[2]);
    // und ordentlich schütteln
    RANDOM.Shuffle(&defenders[0], 5);

    defenders_pos = 0;
}

void GameClientPlayer::ChangeMilitarySettings(const boost::array<unsigned char, MILITARY_SETTINGS_COUNT>& military_settings)
{
    // Im Replay visulle Einstellungen auf die wirklichen setzen
    if(GAMECLIENT.IsReplayModeOn())
        GAMECLIENT.visual_settings.military_settings = military_settings;

    for(unsigned i = 0; i < military_settings.size(); ++i)
    {
        // Sicherstellen, dass im validen Bereich
        RTTR_Assert(military_settings[i] <= MILITARY_SETTINGS_SCALE[i]);
        this->militarySettings_[i] = military_settings[i];
    }
    /// Truppen müssen neu kalkuliert werden
    RegulateAllTroops();
    /// Die Verteidigungsliste muss erneuert werden
    RefreshDefenderList();
}

/// Setzt neue Werkzeugeinstellungen
void GameClientPlayer::ChangeToolsSettings(const ToolSettings& tools_settings)
{
    // Im Replay visulle Einstellungen auf die wirklichen setzen
    if(GAMECLIENT.IsReplayModeOn())
        GAMECLIENT.visual_settings.tools_settings = tools_settings;

    this->toolsSettings_ = tools_settings;
}

/// Setzt neue Verteilungseinstellungen
void GameClientPlayer::ChangeDistribution(const Distributions& distribution_settings)
{
    // Im Replay visulle Einstellungen auf die wirklichen setzen
    if(GAMECLIENT.IsReplayModeOn())
        GAMECLIENT.visual_settings.distribution = distribution_settings;

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
void GameClientPlayer::ChangeBuildOrder(const unsigned char order_type, const BuildOrders& order_data)
{
    // Im Replay visulle Einstellungen auf die wirklichen setzen
    if(GAMECLIENT.IsReplayModeOn())
    {
        GAMECLIENT.visual_settings.order_type = order_type;
        GAMECLIENT.visual_settings.build_order = order_data;
    }

    this->orderType_ = order_type;
    this->build_order = order_data;
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
    if(!defeated && military_buildings.empty() && warehouses.empty())
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
//  for(std::list<nobBaseWarehouse*>::iterator wh = warehouses.begin(); wh != warehouses.end(); ++wh)
//      (*wh)->GetInventory(wares, figures);
//
//  if(wares)
//  {
//      // einzelne Waren sammeln
//      for(std::list<Ware*>::iterator we = ware_list.begin(); we != ware_list.end(); ++we)
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

GameClientPlayer::Pact::Pact(SerializedGameData& sgd)
    : duration(sgd.PopUnsignedInt()), start(sgd.PopUnsignedInt()), accepted(sgd.PopBool()), want_cancel (sgd.PopBool()) { }

void GameClientPlayer::Pact::Serialize(SerializedGameData& sgd)
{
    sgd.PushUnsignedInt(duration);
    sgd.PushUnsignedInt(start);
    sgd.PushBool(accepted);
    sgd.PushBool(want_cancel);
}

void GameClientPlayer::PactChanged(const PactType pt)
{
    // Recheck military flags as the border (to an enemy) might have changed
    RecalcMilitaryFlags();

    // Ggf. den GUI Bescheid sagen, um Sichtbarkeiten etc. neu zu berechnen
    if(pt == TREATY_OF_ALLIANCE && GAMECLIENT.GetPlayerID() == playerid)
    {
        if(gwg->GetGameInterface())
            gwg->GetGameInterface()->GI_TreatyOfAllianceChanged();
    }
}

void GameClientPlayer::SuggestPact(const unsigned char targetPlayer, const PactType pt, const unsigned duration)
{
    pacts[targetPlayer][pt].accepted = false;
    pacts[targetPlayer][pt].duration = duration;
    pacts[targetPlayer][pt].start = GAMECLIENT.GetGFNumber();

    // Post-Message generieren, wenn dieser Pakt den lokalen Spieler betrifft
    if(targetPlayer == GAMECLIENT.GetPlayerID())
        GAMECLIENT.SendPostMessage(new DiplomacyPostQuestion(pacts[targetPlayer][pt].start, playerid, pt, duration));
}

void GameClientPlayer::AcceptPact(const unsigned id, const PactType pt, const unsigned char targetPlayer)
{
    if(!pacts[targetPlayer][pt].accepted && pacts[targetPlayer][pt].start == id)
    {
        MakePact(pt, targetPlayer, pacts[targetPlayer][pt].duration);
        gwg->GetPlayer(targetPlayer).MakePact(pt, playerid, pacts[targetPlayer][pt].duration);
        PactChanged(pt);
        gwg->GetPlayer(targetPlayer).PactChanged(pt);
    }
}

/// Bündnis (real, d.h. spielentscheidend) abschließen
void GameClientPlayer::MakePact(const PactType pt, const unsigned char other_player, const unsigned duration)
{
    pacts[other_player][pt].accepted = true;
    pacts[other_player][pt].start = GAMECLIENT.GetGFNumber();
    pacts[other_player][pt].duration = duration;
    pacts[other_player][pt].want_cancel = false;

    // Send this player a message
    if(GAMECLIENT.GetPlayerID() == playerid)
        GAMECLIENT.SendPostMessage(new DiplomacyPostInfo(other_player, DiplomacyPostInfo::ACCEPT, pt));
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
        else if(GAMECLIENT.GetGFNumber() <= pacts[other_player][pt].start + pacts[other_player][pt].duration )
            return ACCEPTED;
    }

    return NO_PACT;
}

///all allied players get a letter with the location
void GameClientPlayer::NotifyAlliesOfLocation(const MapPoint pt)
{	
	for(unsigned i = 0; i < GAMECLIENT.GetPlayerCount(); ++i)
    {
		if(i != playerid && IsAlly(i) && GAMECLIENT.GetPlayerID() == i)
            GAMECLIENT.SendPostMessage(new PostMsgWithLocation(_("Your ally wishes to notify you of this location"), PMC_DIPLOMACY, pt));
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
            else if(GAMECLIENT.GetGFNumber() <= pacts[other_player][pt].start + pacts[other_player][pt].duration)
                return ((pacts[other_player][pt].start + pacts[other_player][pt].duration) - GAMECLIENT.GetGFNumber());
        }
    }

    return 0;
}

/// Gibt Einverständnis, dass dieser Spieler den Pakt auflösen will
/// Falls dieser Spieler einen Bündnisvorschlag gemacht hat, wird dieser dagegen zurückgenommen
void GameClientPlayer::CancelPact(const PactType pt, const unsigned char otherPlayerIdx)
{
    // Besteht bereits ein Bündnis?
    if(pacts[otherPlayerIdx][pt].accepted)
    {
        // Vermerken, dass der Spieler das Bündnis auflösen will
        pacts[otherPlayerIdx][pt].want_cancel = true;

        // Will der andere Spieler das Bündnis auch auflösen?
        GameClientPlayer& otherPlayer = GAMECLIENT.GetPlayer(otherPlayerIdx);
        if(otherPlayer.pacts[playerid][pt].want_cancel)
        {
            // Dann wird das Bündnis aufgelöst
            pacts[otherPlayerIdx][pt].accepted = false;
            pacts[otherPlayerIdx][pt].duration = 0;
            pacts[otherPlayerIdx][pt].want_cancel = false;

            otherPlayer.pacts[playerid][pt].accepted = false;
            otherPlayer.pacts[playerid][pt].duration = 0;
            otherPlayer.pacts[playerid][pt].want_cancel = false;

            // Den Spielern eine Informationsnachricht schicken
            if(GAMECLIENT.GetPlayerID() == playerid || GAMECLIENT.GetPlayerID() == otherPlayerIdx)
            {
                // Anderen Spieler von sich aus ermitteln
                unsigned char client_other_player = (GAMECLIENT.GetPlayerID() == playerid) ? otherPlayerIdx : playerid;
                GAMECLIENT.SendPostMessage(new DiplomacyPostInfo(client_other_player, DiplomacyPostInfo::CANCEL, pt));
            }
            PactChanged(pt);
            otherPlayer.PactChanged(pt);
        }
        // Ansonsten den anderen Spieler fragen, ob der das auch so sieht
        else if(otherPlayerIdx == GAMECLIENT.GetPlayerID())
            GAMECLIENT.SendPostMessage(new DiplomacyPostQuestion(pacts[otherPlayerIdx][pt].start, playerid, pt));
    }
    else
    {
        // Es besteht kein Bündnis, also unseren Bündnisvorschlag wieder zurücknehmen
        pacts[otherPlayerIdx][pt].duration = 0;
    }
}

void GameClientPlayer::MakeStartPacts()
{
    // Zu den Spielern im selben Team Bündnisse (sowohl Bündnisvertrag als auch Nichtangriffspakt) aufbauen
    for(unsigned i = 0; i < GAMECLIENT.GetPlayerCount(); ++i)
    {
        GameClientPlayer& p = GAMECLIENT.GetPlayer(i);
        if(GetFixedTeam(team) == GetFixedTeam(p.team) && GetFixedTeam(team) >= TM_TEAM1 && GetFixedTeam(team) <= TM_TEAM4)
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

void GameClientPlayer::IncreaseInventoryWare(const GoodType ware, const unsigned count)
{
    global_inventory.Add(ConvertShields(ware), count);
}

void GameClientPlayer::DecreaseInventoryWare(const GoodType ware, const unsigned count)
{
    global_inventory.Remove(ConvertShields(ware), count);
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

    bool operator<(const ShipForHarbor& b) const
    {
		return (estimate < b.estimate) || (estimate == b.estimate && ship->GetObjId() < b.ship->GetObjId());
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
            if ((*it)->IsIdling() && gwg->IsAtThisSea(gwg->GetHarborPointID(hb->GetPos()), (*it)->GetSeaID()))
            {
                sfh.push_back(ShipForHarbor(*it, gwg->CalcDistance(hb->GetPos(), (*it)->GetPos())));
            }
        }
    }
    else
    {
        for (std::vector<noShip*>::iterator it = ships.begin(); it != ships.end(); ++it)
        {
            if ((*it)->IsIdling())
            {
                if (gwg->IsAtThisSea(gwg->GetHarborPointID(hb->GetPos()), (*it)->GetSeaID()))
                {
                    sfh.push_back(ShipForHarbor(*it, gwg->CalcDistance(hb->GetPos(), (*it)->GetPos())));
                }
            }
            else if ((*it)->IsGoingToHarbor(hb))
            {
                sfh.push_back(ShipForHarbor(*it, gwg->CalcDistance(hb->GetPos(), (*it)->GetPos())));
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
        if (it->estimate >= best_distance)
        {
            break;
        }

        noShip* ship = it->ship;

        MapPoint dest = gwg->GetCoastalPoint(hb->GetHarborPosID(), ship->GetSeaID());

        // ship already there?
        if (ship->GetPos() == dest)
        {
            hb->ShipArrived(ship);
            return(true);
        }

        if (gwg->FindShipPath(ship->GetPos(), dest, &route, &distance))
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
            MapPoint dest = gwg->GetCoastalPoint((*it)->GetHarborPosID(), ship->GetSeaID());

            // Evtl. sind wir schon da?
            if(ship->GetPos() == dest)
            {
                (*it)->ShipArrived(ship);
                return;
            }

            unsigned length;
            std::vector<unsigned char> route;

            if(gwg->FindShipPath(ship->GetPos(), dest, &route, &length))
            {
                // Punkte ausrechnen
                int points = (*it)->GetNeedForShip(ships_coming) - length;
                if(points > best_points || !best)
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
void GameClientPlayer::GetHarborBuildings(std::vector<nobHarborBuilding*>& harbor_buildings, const unsigned short sea_id) const
{
    for(std::list<nobHarborBuilding*>::const_iterator it = harbors.begin(); it != harbors.end(); ++it)
    {
        if(helpers::contains(harbor_buildings, *it))
            continue;

        if(gwg->IsAtThisSea((*it)->GetHarborPosID(), sea_id))
            harbor_buildings.push_back(*it);
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
bool GameClientPlayer::FindHarborForUnloading(noShip* ship, const MapPoint start, unsigned* goal_harbor_id,
        std::vector<unsigned char>* route, nobHarborBuilding* exception)
{
    nobHarborBuilding* best = NULL;
    unsigned best_distance = 0xffffffff;

    for(std::list<nobHarborBuilding*>::iterator it = harbors.begin(); it != harbors.end(); ++it)
    {
        nobHarborBuilding* hb = *it;
        // Bestimmten Hafen ausschließen
        if(hb == exception)
            continue;

        // Prüfen, ob Hafen an das Meer, wo sich das Schiff gerade befindet, angrenzt
        if(!gwg->IsAtThisSea(hb->GetHarborPosID(), ship->GetSeaID()))
            continue;

        // Distanz ermitteln zwischen Schiff und Hafen, Schiff kann natürlich auch über Kartenränder fahren
        unsigned distance = gwg->CalcDistance(ship->GetPos(), hb->GetPos());

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
        MapPoint dest = gwg->GetCoastalPoint(best->GetHarborPosID(), ship->GetSeaID());
        route->clear();
        *goal_harbor_id = best->GetHarborPosID();
        // Weg dorthin gefunden?
        if(start == dest || gwg->FindShipPath(start, dest, route, NULL))
            return true;
    }
    
    return false;
}

void GameClientPlayer::TestForEmergencyProgramm()
{
    // we are already defeated, do not even think about an emergency program - it's too late :-(
    if (defeated || warehouses.empty())
    {
        return;
    }

    // In Lagern vorhandene Bretter und Steine zählen
    unsigned boards = 0;
    unsigned stones = 0;
    for(std::list<nobBaseWarehouse*>::iterator w = warehouses.begin(); w != warehouses.end(); ++w)
    {
        boards += (*w)->GetInventory().goods[GD_BOARDS];
        stones += (*w)->GetInventory().goods[GD_STONES];
    }

    // Holzfäller und Sägewerke zählen, -10 ftw
    unsigned woodcutter = buildings[BLD_WOODCUTTER - 10].size();
    unsigned sawmills = buildings[BLD_SAWMILL - 10].size();

    // Wenn nötig, Notfallprogramm auslösen
    if ((boards <= 10 || stones <= 10) && (woodcutter == 0 || sawmills == 0) && (!isDefeated()))
    {
        if (!emergency)
        {
            emergency = true;
            if(GAMECLIENT.GetPlayerID() == this->playerid)
                GAMECLIENT.SendPostMessage(new PostMsg(_("The emergency program has been activated."), PMC_GENERAL));
        }
    }
    else
    {
        // Sobald Notfall vorbei, Notfallprogramm beenden, evtl. Baustellen wieder mit Kram versorgen
        if (emergency)
        {
            emergency = false;
            if(GAMECLIENT.GetPlayerID() == this->playerid)
                GAMECLIENT.SendPostMessage(new PostMsg(_("The emergency program has been deactivated."), PMC_GENERAL));
            FindMaterialForBuildingSites();
        }
    }
}

/// Testet die Bündnisse, ob sie nicht schon abgelaufen sind
void GameClientPlayer::TestPacts()
{
    for(unsigned i = 0; i < GAMECLIENT.GetPlayerCount(); ++i)
    {
        if(i==playerid)
            continue;

        for(unsigned pactId = 0; pactId < PACTS_COUNT; pactId++)
        {
            // Pact not running
            if(pacts[i][pactId].duration == 0)
                continue;
            if(GetPactState(PactType(pactId), i) == NO_PACT)
            {
                // Pact was running but is expired -> Cancel for both players
                pacts[i][pactId].duration = 0;
                RTTR_Assert(gwg->GetPlayer(i).pacts[playerid][pactId].duration);
                gwg->GetPlayer(i).pacts[playerid][pactId].duration = 0;
                // And notify
                PactChanged(PactType(pactId));
                gwg->GetPlayer(i).PactChanged(PactType(pactId));
            }
        }
    }
}


bool GameClientPlayer::CanBuildCatapult() const
{
    // Wenn AddonId::LIMIT_CATAPULTS nicht aktiv ist, bauen immer erlaubt
    if(!GAMECLIENT.GetGGS().isEnabled(AddonId::LIMIT_CATAPULTS)) //-V807
        return true;

    BuildingCount bc;
    GetBuildingCount(bc);

    unsigned int max = 0;

    // proportional?
    if(GAMECLIENT.GetGGS().getSelection(AddonId::LIMIT_CATAPULTS) == 1)
    {
        max = int(bc.building_counts[BLD_BARRACKS] * 0.125 +
                  bc.building_counts[BLD_GUARDHOUSE] * 0.25 +
                  bc.building_counts[BLD_WATCHTOWER] * 0.5 +
                  bc.building_counts[BLD_FORTRESS] + 0.111); // to avoid rounding errors
    }
    else if(GAMECLIENT.GetGGS().getSelection(AddonId::LIMIT_CATAPULTS) < 8)
    {
        const boost::array<unsigned, 6> limits = {{ 0, 3, 5, 10, 20, 30}};
        max = limits[GAMECLIENT.GetGGS().getSelection(AddonId::LIMIT_CATAPULTS) - 2];
    }

    return bc.building_counts[BLD_CATAPULT] + bc.building_site_counts[BLD_CATAPULT] < max;
}

/// A ship has discovered new hostile territory --> determines if this is new
/// i.e. there is a sufficient distance to older locations
/// Returns true if yes and false if not
bool GameClientPlayer::ShipDiscoveredHostileTerritory(const MapPoint location)
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
bool GameClientPlayer::IsDependentFigure(noFigure* fig)
{
    for(std::list<nobBaseWarehouse*>::iterator it = warehouses.begin(); it != warehouses.end(); ++it)
    {
        if((*it)->IsDependentFigure(fig))
            return true;
    }
    return false;
}

std::vector<nobBaseWarehouse*> GameClientPlayer::GetWarehousesForTrading(nobBaseWarehouse& goalWh) const
{
    std::vector<nobBaseWarehouse*> result;

    // Don't try to trade with us!
    if(goalWh.GetPlayer() == playerid)
        return result;

    const MapPoint goalFlagPos = goalWh.GetFlag()->GetPos();

    for(std::list<nobBaseWarehouse*>::const_iterator it = warehouses.begin(); it != warehouses.end(); ++it)
    {
        // Is there a trade path from this warehouse to wh? (flag to flag)
        if(TradePathCache::inst().PathExists(*gwg, (*it)->GetFlag()->GetPos(), goalFlagPos, playerid))
            result.push_back(*it);
    }

    return result;
}

struct WarehouseDistanceComparator
{
    // Reference warehouse, to which we want to calc the distance
    const nobBaseWarehouse& refWareHouse_;
    /// GameWorld
    const GameWorldGame& gwg_;

    WarehouseDistanceComparator(const nobBaseWarehouse& refWareHouse, const GameWorldGame& gwg): refWareHouse_(refWareHouse), gwg_(gwg)
    {}

    bool operator()(nobBaseWarehouse* const wh1, nobBaseWarehouse* const wh2)
    {
        unsigned dist1 = gwg_.CalcDistance(wh1->GetPos(), refWareHouse_.GetPos());
        unsigned dist2 = gwg_.CalcDistance(wh2->GetPos(), refWareHouse_.GetPos());
        return (dist1 < dist2 ) || (dist1 == dist2 && wh1->GetObjId() < wh2->GetObjId());
    }
};

/// Send wares to warehouse wh
void GameClientPlayer::Trade(nobBaseWarehouse* goalWh, const GoodType gt, const Job job, unsigned count) const
{
    if(count == 0)
        return;

    // Don't try to trade with us!
    if(goalWh->GetPlayer() == playerid)
        return;

    const MapPoint goalFlagPos = goalWh->GetFlag()->GetPos();

    std::vector<nobBaseWarehouse*> whs(warehouses.begin(), warehouses.end());
    std::sort(whs.begin(), whs.end(), WarehouseDistanceComparator(*goalWh, *gwg));
    for(std::vector<nobBaseWarehouse*>::const_iterator it = whs.begin(); it != whs.end(); ++it)
    {
        // Get available wares
        unsigned available = 0;
        if(gt != GD_NOTHING)
            available = (*it)->GetAvailableWaresForTrading(gt);
        else{
            RTTR_Assert(job != JOB_NOTHING);
            available = (*it)->GetAvailableFiguresForTrading(job);
        }
        if(available == 0)
            continue;

        available = std::min(available, count);

        // Find a trade path from flag to flag
        TradeRoute tr(*gwg, playerid, (*it)->GetFlag()->GetPos(), goalFlagPos);

        // Found a path?
        if(tr.IsValid())
        {
            // Add to cache for future searches
            TradePathCache::inst().AddEntry(*gwg, tr.GetTradePath(), playerid);

            (*it)->StartTradeCaravane(gt, job, available, tr, goalWh);
            count -= available;
            if(count == 0)
                return;
        }
    }

}

#define INSTANTIATE_FINDWH(Cond) template nobBaseWarehouse* GameClientPlayer::FindWarehouse\
                                 (const noRoadNode&, const Cond&, const bool, const bool, unsigned* const, const RoadSegment* const, bool) const

INSTANTIATE_FINDWH(FW::HasMinWares);
INSTANTIATE_FINDWH(FW::HasFigure);
INSTANTIATE_FINDWH(FW::HasWareAndFigure);
INSTANTIATE_FINDWH(FW::HasMinSoldiers);
INSTANTIATE_FINDWH(FW::AcceptsWare);
INSTANTIATE_FINDWH(FW::AcceptsFigure);
INSTANTIATE_FINDWH(FW::CollectsWare);
INSTANTIATE_FINDWH(FW::CollectsFigure);
INSTANTIATE_FINDWH(FW::HasWareButNoCollect);
INSTANTIATE_FINDWH(FW::HasFigureButNoCollect);
INSTANTIATE_FINDWH(FW::AcceptsFigureButNoSend);
INSTANTIATE_FINDWH(FW::NoCondition);

#undef INSTANTIATE_FINDWH
