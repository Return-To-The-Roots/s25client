// $Id: AIJHHelper.cpp 9589 2015-02-01 09:38:05Z marcus $
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


#include "AIJHHelper.h"
#include "defines.h"
#include "AIPlayerJH.h"
#include "AIConstruction.h"

#include "GameClientPlayer.h"
#include "GameWorld.h"
#include "GameCommands.h"
#include "GamePlayerList.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobHQ.h"
#include "buildings/noBuildingSite.h"
#include "MapGeometry.h"
#include "AIInterface.h"

#include <iostream>


AIJH::Job::Job(AIPlayerJH* aijh)
    : aijh(aijh), status(AIJH::JOB_WAITING)
{
    aii = aijh->GetInterface();
}

void AIJH::BuildJob::ExecuteJob()
{
	//are we allowed to plan construction work in the area in this nwf?
	if(!aijh->GetConstruction()->CanStillConstructHere(around))
		return;

    if (status == AIJH::JOB_WAITING)
        status = AIJH::JOB_EXECUTING_START;

    switch (status)
    {
        case AIJH::JOB_EXECUTING_START:
        {
            TryToBuild();
        }
        break;

        case AIJH::JOB_EXECUTING_ROAD1:
        {
            BuildMainRoad();
        }
        break;

        case AIJH::JOB_EXECUTING_ROAD2:
        {
            TryToBuildSecondaryRoad();
        }
        break;
        case AIJH::JOB_EXECUTING_ROAD2_2:
        {
            // evtl noch pr�fen ob auch dieser Stra�enbau erfolgreich war?
            aijh->RecalcGround(target, route);
            status = AIJH::JOB_FINISHED;
        }
        break;

        default:
            assert(false);
            break;


    }

    // Evil harbour-hack
    //if (type == BLD_HARBORBUILDING && status == AIJH::JOB_FINISHED && target_x != 0xFFFF)
    //{
    //  aijh->AddBuildJob(BLD_SHIPYARD, target, true);
    //}

    // Fertig?
    if (status == AIJH::JOB_FAILED || status == AIJH::JOB_FINISHED)
        return;

    if ((target.x != 0xFFFF) && aii->IsMilitaryBuildingNearNode(target, aijh->GetPlayerID()) && type >= BLD_BARRACKS && type <= BLD_FORTRESS)
    {
        status = AIJH::JOB_FAILED;
#ifdef DEBUG_AI
        std::cout << "Player " << (unsigned)aijh->GetPlayerID() << ", Job failed: Military building too near for " << BUILDING_NAMES[type] << " at " << target.x << "/" << target.y << "." << std::endl;
#endif
        return;
    }
}

void AIJH::BuildJob::TryToBuild()
{
    MapPoint bPos = around;

    if (aii->GetBuildingSites().size() > 40)
    {
        return;
    }

    if (!aijh->GetConstruction()->Wanted(type))
    {
        status = AIJH::JOB_FINISHED;
        return;
    }

    bool foundPos = false;
    if (searchMode == SEARCHMODE_GLOBAL)
    {
        // TODO: tmp solution for testing: only woodcutter
        //hier machen f�r mehre geb�ude
        //erstmal wieder rausgenommen weil kaputt - todo: fix positionsearch
        if (type != BLD_WOODCUTTER)
        {
            searchMode = SEARCHMODE_RADIUS;
        }
        else
        {
            searchMode = SEARCHMODE_RADIUS;
            /*
            PositionSearch *search = aijh->CreatePositionSearch(bPos, AIJH::WOOD, BQ_HUT, 20, BLD_WOODCUTTER, true);
            SearchJob *job = new SearchJob(aijh, search);
            aijh->AddJob(job, true);
            status = AIJH::JOB_FINISHED;
            return;*/
        }

    }


    if (searchMode == SEARCHMODE_RADIUS)
    {
        switch(type)
        {
            case BLD_WOODCUTTER:
            {
                unsigned numWoodcutter = aijh->GetConstruction()->GetBuildingCount(BLD_WOODCUTTER);
                foundPos = aijh->FindBestPosition(bPos, AIJH::WOOD, BQ_HUT, (numWoodcutter > 2) ? 20 : 1 + aijh->GetConstruction()->GetBuildingCount(BLD_WOODCUTTER) * 10, 11);
                if(foundPos && !aijh->ValidTreeinRange(bPos))
                    foundPos = false;
                break;
            }
            case BLD_FORESTER:
				if ((aijh->GetConstruction()->GetBuildingCount(BLD_FORESTER)<2 || !aijh->GetConstruction()->OtherUsualBuildingInRadius(bPos, 35, BLD_FORESTER)) && (aijh->GetDensity(bPos, AIJH::PLANTSPACE, 7) > 30 || (aijh->GetDensity(bPos, AIJH::PLANTSPACE, 7) > 15 && aijh->GetConstruction()->GetBuildingCount(BLD_FORESTER) < 2)))
                    foundPos = aijh->FindBestPosition(bPos, AIJH::WOOD, BQ_HUT, 0, 11);
                break;
            case BLD_HUNTER:
            {
                //check if there are any animals in range
                if(aijh->HuntablesinRange(bPos, (2 << aijh->GetConstruction()->GetBuildingCount(BLD_HUNTER))))
                    foundPos = aijh->SimpleFindPosition(bPos, BUILDING_SIZE[type], 11);
                break;
            }
            case BLD_QUARRY:
            {
                unsigned numQuarries = aijh->GetConstruction()->GetBuildingCount(BLD_QUARRY);
                foundPos = aijh->FindBestPosition(bPos, AIJH::STONES, BQ_HUT, (numQuarries > 4) ? 40 : 1 + aijh->GetConstruction()->GetBuildingCount(BLD_QUARRY) * 10, 11);
                if(foundPos && !aijh->ValidStoneinRange(bPos))
                {
                    foundPos = false;
                    aijh->SetResourceMap(AIJH::STONES, aii->GetIdx(bPos), 0);
                }
                break;
            }
            case BLD_BARRACKS:
            case BLD_GUARDHOUSE:
            case BLD_WATCHTOWER:
            case BLD_FORTRESS:
                foundPos = aijh->FindBestPosition(bPos, AIJH::BORDERLAND, BUILDING_SIZE[type], 1, 11, true);
				//could we build a bigger military building? check if the location is surrounded by terrain that does not allow normal buildings (probably important map part)
				if(aii->GetBuildingQuality(bPos)!=BQ_MINE && aii->GetBuildingQuality(bPos)>BUILDING_SIZE[type] && aijh->BQsurroundcheck(bPos,6,true,10)<10)
				{
					//more than 80% is unbuildable in range 7 -> upgrade
					type=type<BLD_WATCHTOWER?BLD_WATCHTOWER:BLD_FORTRESS;
				}
                break;
            case BLD_GOLDMINE:
                foundPos = aijh->FindBestPosition(bPos, AIJH::GOLD, BQ_MINE, 11, true);
                break;
            case BLD_COALMINE:
                foundPos = aijh->FindBestPosition(bPos, AIJH::COAL, BQ_MINE, 11, true);
                break;
            case BLD_IRONMINE:
                foundPos = aijh->FindBestPosition(bPos, AIJH::IRONORE, BQ_MINE, 11, true);
                break;
            case BLD_GRANITEMINE:
                if(!aijh->ggs->isEnabled(ADDON_INEXHAUSTIBLE_GRANITEMINES)) //inexhaustible granite mines do not require granite
                    foundPos = aijh->FindBestPosition(bPos, AIJH::GRANITE, BQ_MINE, 11, true);
                else
                    foundPos = aijh->SimpleFindPosition(bPos, BQ_MINE, 11);
                break;

            case BLD_FISHERY:
                foundPos = aijh->FindBestPosition(bPos, AIJH::FISH, BQ_HUT, 11, true);
                if(foundPos && !aijh->ValidFishInRange(bPos))
                {
                    aijh->SetResourceMap(AIJH::FISH, aii->GetIdx(bPos), 0);
                    foundPos = false;
                }
                break;
            case BLD_STOREHOUSE:
                if(!aijh->GetConstruction()->OtherStoreInRadius(bPos, 15))
                    foundPos = aijh->SimpleFindPosition(bPos, BUILDING_SIZE[BLD_STOREHOUSE], 11);
                break;
            case BLD_HARBORBUILDING:
                foundPos = aijh->SimpleFindPosition(bPos, BUILDING_SIZE[type], 11);
                if(foundPos && !aijh->HarborPosRelevant(aijh->gwb->GetHarborPointID(bPos))) //bad harborspot detected DO NOT USE
                    foundPos = false;
                break;
            case BLD_SHIPYARD:
                foundPos = aijh->SimpleFindPosition(bPos, BUILDING_SIZE[type], 11);
                if(foundPos && aijh->IsInvalidShipyardPosition(bPos))
                    foundPos = false;
                break;
            case BLD_FARM:
                foundPos = aijh->FindBestPosition(bPos, AIJH::PLANTSPACE, BQ_CASTLE, 85, 11, true);
                break;
            case BLD_CATAPULT:
                foundPos = aijh->SimpleFindPosition(bPos, BUILDING_SIZE[type], 11);
                if(foundPos && aijh->BuildingNearby(bPos, BLD_CATAPULT, 8))
                    foundPos = false;
                break;
            default:
                foundPos = aijh->SimpleFindPosition(bPos, BUILDING_SIZE[type], 11);
                break;
        }
    }

    if (searchMode == SEARCHMODE_NONE)
    {
        foundPos = true;
        bPos = around;
    }


    if (!foundPos)
    {
        status = JOB_FAILED;
#ifdef DEBUG_AI
        std::cout << "Player " << (unsigned)aijh->GetPlayerID() << ", Job failed: No Position found for " << BUILDING_NAMES[type] << " around " << bPos.x << "/" << bPos.y << "." << std::endl;
#endif
        return;
    }

#ifdef DEBUG_AI
    if (type == BLD_FARM)
        std::cout << " Player " << (unsigned)aijh->GetPlayerID() << " built farm at " << bPos.x << "/" << bPos.y << " on value of " << aijh->resourceMaps[AIJH::PLANTSPACE][aii->GetIdx(bPos)] << std::endl;
#endif

    aii->SetBuildingSite(bPos, type);
    target = bPos;
    status = AIJH::JOB_EXECUTING_ROAD1;
	aijh->GetConstruction()->constructionlocations.push_back(target); // add new construction area to the list of active orders in the current nwf
	aijh->GetConstruction()->constructionorders[type]++;
    return;
}

void AIJH::BuildJob::BuildMainRoad()
{
    const noBuildingSite* bld;
    if (!(bld = aii->GetSpecObj<noBuildingSite>(target)))
    {
        // Pr�fen ob sich vielleicht die BQ ge�ndert hat und damit Bau unm�glich ist
        BuildingQuality bq = aii->GetBuildingQuality(target);
        if (!(bq >= BUILDING_SIZE[type] && bq < BQ_MINE) // normales Geb�ude
                && !(bq == BUILDING_SIZE[type]))    // auch Bergwerke
        {
            status = AIJH::JOB_FAILED;
#ifdef DEBUG_AI
            std::cout << "Player " << (unsigned)aijh->GetPlayerID() << ", Job failed: BQ changed for " << BUILDING_NAMES[type] << " at " << target.x << "/" << target.y << ". Retrying..." << std::endl;
#endif
            aijh->nodes[aii->GetIdx(target)].bq = bq;
            aijh->AddBuildJob(new AIJH::BuildJob(aijh, type, around));
            return;
        }
        return;
    }

    if (bld->GetBuildingType() != type)
    {
#ifdef DEBUG_AI
        std::cout << "Player " << (unsigned)aijh->GetPlayerID() << ", Job failed: Wrong Builingsite found for " << BUILDING_NAMES[type] << " at " << target.x << "/" << target.y << "." << std::endl;
#endif
        status = AIJH::JOB_FAILED;
        return;
    }
    const noFlag* houseFlag = aii->GetSpecObj<noFlag>(aii->GetNeighbour(target, 4));
    // Gucken noch nicht ans Wegnetz angeschlossen
    if (!aijh->GetConstruction()->IsConnectedToRoadSystem(houseFlag))
    {
        // Bau unm�glich?
        if (!aijh->GetConstruction()->ConnectFlagToRoadSytem(houseFlag, route))
        {
            status = AIJH::JOB_FAILED;
#ifdef DEBUG_AI
            std::cout << "Player " << (unsigned)aijh->GetPlayerID() << ", Job failed: Cannot connect " << BUILDING_NAMES[type] << " at " << target_x << "/" << target_y << ". Retrying..." << std::endl;
#endif
            aijh->nodes[aii->GetIdx(target)].reachable = false;
            aii->DestroyBuilding(target);
            aii->DestroyFlag(houseFlag->GetPos());
            aijh->AddBuildJob(new AIJH::BuildJob(aijh, type, around));
            return;
        }
        else
        {
			aijh->GetConstruction()->constructionlocations.push_back(target); // add new construction area to the list of active orders in the current nwf
            // Warten bis Weg da ist...
            //return;
        }
    }

    // Wir sind angeschlossen, BQ f�r den eben gebauten Weg aktualisieren
    //aijh->RecalcBQAround(target);
    //aijh->RecalcGround(target, route);

    switch(type)
    {
        case BLD_WOODCUTTER:
            break;
        case BLD_FORESTER:
            aijh->AddBuildJob(new AIJH::BuildJob(aijh, BLD_WOODCUTTER, target));
            break;
        case BLD_QUARRY:
            break;
        case BLD_BARRACKS:
        case BLD_GUARDHOUSE:
        case BLD_WATCHTOWER:
        case BLD_FORTRESS:
            break;
        case BLD_GOLDMINE:
            break;
        case BLD_COALMINE:
            break;
        case BLD_IRONMINE:
            //if(!(aijh->ggs->isEnabled(ADDON_INEXHAUSTIBLE_MINES)))
            break;
        case BLD_GRANITEMINE:
            break;
        case BLD_FISHERY:
            break;
        case BLD_STOREHOUSE:
            break;
        case BLD_HARBORBUILDING:
            break;
        case BLD_CHARBURNER:
        case BLD_FARM:
            aijh->SetFarmedNodes(target, true);
            break;
        case BLD_MILL:
            aijh->AddBuildJob(new AIJH::BuildJob(aijh, BLD_BAKERY, target));
            break;
        case BLD_PIGFARM:
            aijh->AddBuildJob(new AIJH::BuildJob(aijh, BLD_SLAUGHTERHOUSE, target));
            break;
        case BLD_BAKERY:
        case BLD_SLAUGHTERHOUSE:
        case BLD_BREWERY:
            aijh->AddBuildJob(new AIJH::BuildJob(aijh, BLD_WELL, target));
            break;

        default:
            break;
    }

    // Just 4 Fun Gelehrten rufen
    if (BUILDING_SIZE[type] == BQ_MINE)
    {
        aii->CallGeologist(houseFlag->GetPos());
    }
	if(type > BLD_FORTRESS)//not a military building? -> build secondary road now 
	{
		status = AIJH::JOB_EXECUTING_ROAD2;
		return TryToBuildSecondaryRoad();
	}
	else //military buildings only get 1 road
	{
		status = AIJH::JOB_FINISHED;
	}

}

void AIJH::BuildJob::TryToBuildSecondaryRoad()
{
    const noFlag* houseFlag = aii->GetSpecObj<noFlag>(aii->GetNeighbour(target, 4));

    if (!houseFlag)
    {
        // Baustelle wurde wohl zerst�rt, oh schreck!
        status = AIJH::JOB_FAILED;
#ifdef DEBUG_AI
        std::cout << "Player " << (unsigned)aijh->GetPlayerID() << ", Job failed: House flag is gone, " << BUILDING_NAMES[type] << " at " << target_x << "/" << target_y << ". Retrying..." << std::endl;
#endif
        aijh->AddBuildJob(new AIJH::BuildJob(aijh, type, around));
        return;
    }

    if (aijh->GetConstruction()->BuildAlternativeRoad(houseFlag, route))
	{
        status = AIJH::JOB_EXECUTING_ROAD2_2;
		aijh->GetConstruction()->constructionlocations.push_back(target); // add new construction area to the list of active orders in the current nw
	}
    else
        status = AIJH::JOB_FINISHED;
}

void AIJH::ExpandJob::ExecuteJob()
{

}


void AIJH::EventJob::ExecuteJob()//for now it is assumed that all these will be finished or failed after execution (no wait or progress)
{
    switch(ev->GetType())
    {
        case AIEvent::BuildingConquered:
        {
            AIEvent::Building* evb = dynamic_cast<AIEvent::Building*>(ev);
            aijh->HandleNewMilitaryBuilingOccupied(evb->GetPos());
            status = AIJH::JOB_FINISHED;
        }
        break;
        case AIEvent::BuildingLost:
        {
            AIEvent::Building* evb = dynamic_cast<AIEvent::Building*>(ev);
            aijh->HandleMilitaryBuilingLost(evb->GetPos());
            status = AIJH::JOB_FINISHED;
        }
        break;
        case AIEvent::BuildingDestroyed:
        {
            //todo maybe do sth about it?
            AIEvent::Building* evb = dynamic_cast<AIEvent::Building*>(ev);
            //at least for farms ai has to remove "farmed"
            if(evb->GetBuildingType() == BLD_FARM || evb->GetBuildingType() == BLD_HARBORBUILDING)
                aijh->HandleBuilingDestroyed(evb->GetPos(), evb->GetBuildingType());
            status = AIJH::JOB_FINISHED;
        }
        break;
        case AIEvent::NoMoreResourcesReachable:
        {
            AIEvent::Building* evb = dynamic_cast<AIEvent::Building*>(ev);
            aijh->HandleNoMoreResourcesReachable(evb->GetPos(), evb->GetBuildingType());
            status = AIJH::JOB_FINISHED;
        }
        break;
        case AIEvent::BorderChanged:
        {
            AIEvent::Building* evb = dynamic_cast<AIEvent::Building*>(ev);
            aijh->HandleBorderChanged(evb->GetPos());
            status = AIJH::JOB_FINISHED;
        }
        break;
        case AIEvent::BuildingFinished:
        {
            AIEvent::Building* evb = dynamic_cast<AIEvent::Building*>(ev);
            aijh->HandleBuildingFinished(evb->GetPos(), evb->GetBuildingType());
            status = AIJH::JOB_FINISHED;
        }
        break;
        case AIEvent::ExpeditionWaiting:
        {
            AIEvent::Location* lvb = dynamic_cast<AIEvent::Location*>(ev);
            aijh->HandleExpedition(lvb->GetPos());
            status = AIJH::JOB_FINISHED;
        }
        break;
        case AIEvent::TreeChopped:
        {
            AIEvent::Location* lvb = dynamic_cast<AIEvent::Location*>(ev);
            aijh->HandleTreeChopped(lvb->GetPos());
            status = AIJH::JOB_FINISHED;
        }
        break;
        case AIEvent::NewColonyFounded:
        {
            AIEvent::Location* lvb = dynamic_cast<AIEvent::Location*>(ev);
            aijh->HandleNewColonyFounded(lvb->GetPos());
            status = AIJH::JOB_FINISHED;
        }
        break;
        case AIEvent::ShipBuilt:
        {
            AIEvent::Location* lvb = dynamic_cast<AIEvent::Location*>(ev);
            aijh->HandleShipBuilt(lvb->GetPos());
            status = AIJH::JOB_FINISHED;
        }
        break;
        case AIEvent::RoadConstructionComplete:
        {
            AIEvent::Direction* dvb = dynamic_cast<AIEvent::Direction*>(ev);
            aijh->HandleRoadConstructionComplete(dvb->GetPos(), dvb->GetDirection());
            status = AIJH::JOB_FINISHED;
        }
        break;
        case AIEvent::RoadConstructionFailed:
        {
            AIEvent::Direction* dvb = dynamic_cast<AIEvent::Direction*>(ev);
            aijh->HandleRoadConstructionFailed(dvb->GetPos(), dvb->GetDirection());
            status = AIJH::JOB_FINISHED;
        }
        break;
		case AIEvent::LuaConstructionOrder:
        {
            AIEvent::Building* evb = dynamic_cast<AIEvent::Building*>(ev);
			aijh->ExecuteLuaConstructionOrder(evb->GetPos(),evb->GetBuildingType(),true);           
            status = AIJH::JOB_FINISHED;
        }
        break;
        default:
            //status = AIJH::JOB_FAILED;
            break;
    }

    //temp only:
    status = AIJH::JOB_FINISHED;
}


void AIJH::ConnectJob::ExecuteJob()
{
#ifdef DEBUG_AI
    std::cout << "Player " << (unsigned)aijh->GetPlayerID() << ", ConnectJob executed..." << std::endl;
#endif
	
	//can the ai still construct here? else return and try again later
	if (!aijh->GetConstruction()->CanStillConstructHere(flagPos))
		return; 

    const noFlag* flag = aii->GetSpecObj<noFlag>(flagPos);
	
    if (!flag)
    {
#ifdef DEBUG_AI
        std::cout << "Flag is gone." << std::endl;
#endif
        status = AIJH::JOB_FAILED;
        return;
    }

	//is flag of a military building and has some road connection alraedy (not necessarily to a warehouse so this is required to avoid multiple connections on mil buildings)
	if(aii->IsMilitaryBuildingOnNode(aii->GetNeighbour(flag->GetPos(),1)))
	{
		for(unsigned i=2;i<7;i++)
		{
			if(flag->routes[i%6])
			{
				status=AIJH::JOB_FINISHED;
				return;
			}
		}
	}

    // already connected?
    if (!aijh->GetConstruction()->IsConnectedToRoadSystem(flag))
    {
#ifdef DEBUG_AI
        std::cout << "Flag is not connected..." << std::endl;
#endif
        // building road possible?
        if (!aijh->GetConstruction()->ConnectFlagToRoadSytem(flag, route, 24))
        {
#ifdef DEBUG_AI
            std::cout << "Flag is not connectable." << std::endl;
#endif
            status = AIJH::JOB_FAILED;
            return;
        }
        else
        {
#ifdef DEBUG_AI
            std::cout << "Connecting flag..." << std::endl;
#endif
            // constructing road... wait...
			aijh->GetConstruction()->constructionlocations.push_back(flagPos); // add new construction area to the list of active orders in the current nwf
            return;
        }
    }
    else
    {
#ifdef DEBUG_AI
        std::cout << "Flag is connected." << std::endl;
#endif
        aijh->RecalcGround(flagPos, route);
        status = AIJH::JOB_FINISHED;
		
        return;
    }
}


void AIJH::SearchJob::ExecuteJob()
{
    status = JOB_FAILED;
    PositionSearchState state = aijh->FindGoodPosition(search, true);

    if (state == SEARCH_IN_PROGRESS)
    {
        status = JOB_WAITING;
    }
    else if (state == SEARCH_FAILED)
    {
        status = JOB_FAILED;
    }
    else
    {
        status = JOB_FINISHED;
        aijh->AddBuildJob(new BuildJob(aijh, search->bld, search->result, SEARCHMODE_NONE), true);
    }
}

AIJH::SearchJob::~SearchJob()
{
    delete search;
}
