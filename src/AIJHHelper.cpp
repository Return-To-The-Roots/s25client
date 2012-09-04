// $Id: AIJHHelper.cpp 8144 2012-09-04 10:22:54Z marcus $
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
#include "main.h"
#include "AIPlayerJH.h"
#include "AIConstruction.h"

#include "GameClientPlayer.h"
#include "GameWorld.h"
#include "GameCommands.h"
#include "GamePlayerList.h"
#include "nobMilitary.h"
#include "nobHQ.h"
#include "noBuildingSite.h"
#include "MapGeometry.h"
#include "AIInterface.h"

#include <iostream>


AIJH::Job::Job(AIPlayerJH *aijh) 
: aijh(aijh), status(AIJH::JOB_WAITING)
{
 aii = aijh->GetInterface();
}

void AIJH::BuildJob::ExecuteJob()
{
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
			// evtl noch prüfen ob auch dieser Straßenbau erfolgreich war?
			aijh->RecalcGround(target_x, target_y, route);
			status = AIJH::JOB_FINISHED;
		}
		break;

	default:
		assert(false);
		break;


	}

	// Evil harbour-hack
	if (type == BLD_HARBORBUILDING && status == AIJH::JOB_FINISHED && target_x != 0xFFFF)
	{
		aijh->AddBuildJob(BLD_SHIPYARD, target_x, target_y, true);
	}

	// Fertig?
	if (status == AIJH::JOB_FAILED || status == AIJH::JOB_FINISHED)
		return;

	if ((target_x != 0xFFFF) && aii->IsMilitaryBuildingNearNode(target_x, target_y) && type >= BLD_BARRACKS && type <= BLD_FORTRESS)
	{
		status = AIJH::JOB_FAILED;
#ifdef DEBUG_AI
		std::cout << "Player " << (unsigned)aijh->GetPlayerID() << ", Job failed: Military building too near for " << BUILDING_NAMES[type] << " at " << target_x << "/" << target_y << "." << std::endl;
#endif
		//aijh->nodes[target_x + target_y * aijh->GetGWB()->GetWidth()].
		return;
	}
}

void AIJH::BuildJob::TryToBuild()
{
	MapCoord bx = around_x;
	MapCoord by = around_y;

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
		//hier machen für mehre gebäude
		if (type != BLD_WOODCUTTER)
		{
			searchMode = SEARCHMODE_RADIUS;
		}
		else
		{
			PositionSearch *search = aijh->CreatePositionSearch(bx, by, AIJH::WOOD, BQ_HUT, 20, BLD_WOODCUTTER, true);
			SearchJob *job = new SearchJob(aijh, search);
			aijh->AddJob(job, true);
			status = AIJH::JOB_FINISHED;
			return;
		}
		
	}
	

	if (searchMode == SEARCHMODE_RADIUS)
	{
		switch(type)
		{
		case BLD_WOODCUTTER:
			{
			unsigned numWoodcutter = aijh->GetConstruction()->GetBuildingCount(BLD_WOODCUTTER);
			foundPos = aijh->FindBestPosition(bx, by, AIJH::WOOD, BQ_HUT, (numWoodcutter > 2) ? 20 : 1 + aijh->GetConstruction()->GetBuildingCount(BLD_WOODCUTTER) * 10, 15);
			if(foundPos&&!aijh->ValidTreeinRange(bx,by))
				foundPos=false;
			break;
			}
		case BLD_FORESTER:
 			if (aijh->GetDensity(bx, by, AIJH::PLANTSPACE, 7) > 0.2)
				foundPos = aijh->FindBestPosition(bx, by, AIJH::WOOD, BQ_HUT, 0, 15);
			break;
		case BLD_HUNTER:
			{	//check if there are any animals in range
				if(aijh->HuntablesinRange(bx,by,(2<<aijh->GetConstruction()->GetBuildingCount(BLD_HUNTER))))
					foundPos=aijh->SimpleFindPosition(bx, by, BUILDING_SIZE[type], 15);
				break;
			}
		case BLD_QUARRY:
			{
			unsigned numQuarries = aijh->GetConstruction()->GetBuildingCount(BLD_QUARRY);
			foundPos = aijh->FindBestPosition(bx, by, AIJH::STONES, BQ_HUT, (numQuarries > 4) ? 40 : 1 + aijh->GetConstruction()->GetBuildingCount(BLD_QUARRY) * 10, 15);
			if(foundPos&&!aijh->ValidStoneinRange(bx,by))
				foundPos=false;
			break;
			}
		case BLD_BARRACKS:
		case BLD_GUARDHOUSE:
		case BLD_WATCHTOWER:
		case BLD_FORTRESS:
			foundPos = aijh->FindBestPosition(bx, by, AIJH::BORDERLAND, BUILDING_SIZE[type],1, 15, true);
			break;
		case BLD_GOLDMINE:
			foundPos = aijh->FindBestPosition(bx, by, AIJH::GOLD, BQ_MINE, 15, true);
			break;
		case BLD_COALMINE:
			foundPos = aijh->FindBestPosition(bx, by, AIJH::COAL, BQ_MINE, 15, true);
			break;
		case BLD_IRONMINE:
			foundPos = aijh->FindBestPosition(bx, by, AIJH::IRONORE, BQ_MINE, 15, true);
			break;
		case BLD_GRANITEMINE:
			foundPos = aijh->FindBestPosition(bx, by, AIJH::GRANITE, BQ_MINE, 15, true);
			break;

		case BLD_FISHERY:
			foundPos = aijh->FindBestPosition(bx, by, AIJH::FISH, BQ_HUT, 15, true);
			break;
		case BLD_STOREHOUSE:
			if(aijh->GetConstruction()->FindStoreHousePosition(bx, by, 15))
				foundPos = aijh->SimpleFindPosition(bx, by, BUILDING_SIZE[BLD_STOREHOUSE], 15);
			break;

		case BLD_FARM:
			foundPos = aijh->FindBestPosition(bx, by, AIJH::PLANTSPACE, BQ_CASTLE, 25, 12, true);
			break;

		default:
			foundPos = aijh->SimpleFindPosition(bx, by, BUILDING_SIZE[type], 15);
			break;
		}
	}

	if (searchMode == SEARCHMODE_NONE)
	{
		foundPos = true;
		bx = around_x;
		by = around_y;
	}

	
	if (!foundPos)
	{
		status = JOB_FAILED;
#ifdef DEBUG_AI
		std::cout << "Player " << (unsigned)aijh->GetPlayerID() << ", Job failed: No Position found for " << BUILDING_NAMES[type] << " around " << bx << "/" << by << "." << std::endl;
#endif
		return;
	}

#ifdef DEBUG_AI
	if (type == BLD_FARM)
		std::cout << " Player " << (unsigned)aijh->GetPlayerID() << " built farm at " << bx << "/" << by << " on value of " << aijh->resourceMaps[AIJH::PLANTSPACE][bx+by*aijh->GetGWB()->GetWidth()] << std::endl;
#endif

	aii->SetBuildingSite(bx, by, type);
	target_x = bx;
	target_y = by;
	status = AIJH::JOB_EXECUTING_ROAD1;
	return;
}

void AIJH::BuildJob::BuildMainRoad()
{
	const noBuildingSite *bld;
	if (!(bld = aii->GetSpecObj<noBuildingSite>(target_x, target_y)))
	{
		// Prüfen ob sich vielleicht die BQ geändert hat und damit Bau unmöglich ist
		BuildingQuality bq = aii->GetBuildingQuality(target_x, target_y);
		if (!(bq >= BUILDING_SIZE[type] && bq < BQ_MINE) // normales Gebäude
							&& !(bq == BUILDING_SIZE[type]))	// auch Bergwerke
		{
			status = AIJH::JOB_FAILED;
#ifdef DEBUG_AI
			std::cout << "Player " << (unsigned)aijh->GetPlayerID() << ", Job failed: BQ changed for " << BUILDING_NAMES[type] << " at " << target_x << "/" << target_y << ". Retrying..." << std::endl;
#endif
			aijh->nodes[target_x + target_y * aii->GetMapWidth()].bq = bq;
			aijh->AddBuildJob(new AIJH::BuildJob(aijh, type, around_x, around_y));
			return;
		}
		return;
	}

	if (bld->GetBuildingType() != type)
	{
#ifdef DEBUG_AI
		std::cout << "Player " << (unsigned)aijh->GetPlayerID() << ", Job failed: Wrong Builingsite found for " << BUILDING_NAMES[type] << " at " << target_x << "/" << target_y << "." << std::endl;
#endif
		status = AIJH::JOB_FAILED;
		return;
	}
	const noFlag *houseFlag = aii->GetSpecObj<noFlag>(aii->GetXA(target_x, target_y, 4), 
		aii->GetYA(target_x, target_y, 4));
	// Gucken noch nicht ans Wegnetz angeschlossen
	if (!aijh->GetConstruction()->IsConnectedToRoadSystem(houseFlag))
	{
		// Bau unmöglich?
		if (!aijh->GetConstruction()->ConnectFlagToRoadSytem(houseFlag, route))
		{
			status = AIJH::JOB_FAILED;
#ifdef DEBUG_AI
			std::cout << "Player " << (unsigned)aijh->GetPlayerID() << ", Job failed: Cannot connect " << BUILDING_NAMES[type] << " at " << target_x << "/" << target_y << ". Retrying..." << std::endl;
#endif
			aijh->nodes[target_x + target_y * aii->GetMapWidth()].reachable = false;
			aii->DestroyBuilding(target_x, target_y);
			aii->DestroyFlag(houseFlag->GetX(), houseFlag->GetY());
			aijh->AddBuildJob(new AIJH::BuildJob(aijh, type, around_x, around_y));
			return;
		}
		else
		{
			// Warten bis Weg da ist...
			return;
		}
	}
	else
	{
		// Wir sind angeschlossen, BQ für den eben gebauten Weg aktualisieren
		//aijh->RecalcBQAround(target_x, target_y);
		aijh->RecalcGround(target_x, target_y, route);

		switch(type)
		{
		case BLD_WOODCUTTER:
			//aijh->ChangeResourceMap(target_x, target_y, 7, aijh->resourceMaps[AIJH::WOOD], -15);
			break;
		case BLD_FORESTER:
			//aijh->ChangeResourceMap(target_x, target_y, 7, aijh->resourceMaps[AIJH::WOOD], 15);
			break;
		case BLD_QUARRY:
			//aijh->ChangeResourceMap(target_x, target_y, 7, aijh->resourceMaps[AIJH::STONES], -15);
			break;
		case BLD_BARRACKS:
		case BLD_GUARDHOUSE:
		case BLD_WATCHTOWER:
		case BLD_FORTRESS:
			//aijh->ChangeResourceMap(target_x, target_y, 8, aijh->resourceMaps[AIJH::BORDERLAND], -8);
			aijh->milBuildingSites.push_back(AIPlayerJH::Coords(target_x, target_y));
		case BLD_GOLDMINE:
			//if(!(aijh->ggs->isEnabled(ADDON_INEXHAUSTIBLE_MINES))) //ask the ai to recalculate the resourcemap instead of the old way to just change it this way its accurate
			//	aijh->ChangeResourceMap(target_x, target_y, 4, aijh->resourceMaps[AIJH::GOLD], -30);
			break;
		case BLD_COALMINE:
			//if(!(aijh->ggs->isEnabled(ADDON_INEXHAUSTIBLE_MINES)))
			//	aijh->ChangeResourceMap(target_x, target_y, 4, aijh->resourceMaps[AIJH::COAL], -30);
			break;
		case BLD_IRONMINE:
			//if(!(aijh->ggs->isEnabled(ADDON_INEXHAUSTIBLE_MINES)))
			//	aijh->ChangeResourceMap(target_x, target_y, 4, aijh->resourceMaps[AIJH::IRONORE], -30);
			break;
		case BLD_GRANITEMINE:
			//if(!(aijh->ggs->isEnabled(ADDON_INEXHAUSTIBLE_MINES)))
			//	aijh->ChangeResourceMap(target_x, target_y, 4, aijh->resourceMaps[AIJH::GRANITE], -30);
			break;

		case BLD_FISHERY:
			aijh->ChangeResourceMap(target_x, target_y, 10, aijh->resourceMaps[AIJH::FISH], -30);
			break;
		case BLD_STOREHOUSE:
			aijh->GetConstruction()->AddStoreHouse(target_x, target_y);
			break;
		case BLD_HARBORBUILDING:
			aijh->GetConstruction()->AddStoreHouse(target_x, target_y);
			break;
		case BLD_FARM:
			aijh->ChangeResourceMap(target_x, target_y, 5, aijh->resourceMaps[AIJH::PLANTSPACE], -30);
			aijh->SetFarmedNodes(target_x, target_y);
			break;

		case BLD_MILL:
			aijh->AddBuildJob(new AIJH::BuildJob(aijh, BLD_BAKERY, target_x, target_y));
			break;

		case BLD_PIGFARM:
			aijh->AddBuildJob(new AIJH::BuildJob(aijh, BLD_SLAUGHTERHOUSE, target_x, target_y));
			break;

		case BLD_BAKERY:
		case BLD_SLAUGHTERHOUSE:
		case BLD_BREWERY:
			aijh->AddBuildJob(new AIJH::BuildJob(aijh, BLD_WELL, target_x, target_y));
			break;

		default:
			break;
		}

		// Just 4 Fun Gelehrten rufen
		if (BUILDING_SIZE[type] == BQ_MINE)
		{
			aii->CallGeologist(houseFlag->GetX(), houseFlag->GetY());
		}

		status = AIJH::JOB_EXECUTING_ROAD2;
		return;
	}
}
	
void AIJH::BuildJob::TryToBuildSecondaryRoad()
{
	const noFlag *houseFlag = aii->GetSpecObj<noFlag>(aii->GetXA(target_x, target_y, 4), 
		aii->GetYA(target_x, target_y, 4));

	if (!houseFlag)
	{
		// Baustelle wurde wohl zerstört, oh schreck!
		status = AIJH::JOB_FAILED;
#ifdef DEBUG_AI
			std::cout << "Player " << (unsigned)aijh->GetPlayerID() << ", Job failed: House flag is gone, " << BUILDING_NAMES[type] << " at " << target_x << "/" << target_y << ". Retrying..." << std::endl;
#endif
		aijh->AddBuildJob(new AIJH::BuildJob(aijh, type, around_x, around_y));
		return;
	}

	if (aijh->GetConstruction()->BuildAlternativeRoad(houseFlag, route))
		status = AIJH::JOB_EXECUTING_ROAD2_2;
	else
		status = AIJH::JOB_FINISHED;
}

void AIJH::ExpandJob::ExecuteJob()
{

}


void AIJH::EventJob::ExecuteJob()
{
	switch(ev->GetType())
	{
	case AIEvent::BuildingConquered:
		{
			AIEvent::Building *evb = dynamic_cast<AIEvent::Building *>(ev);
			aijh->HandleNewMilitaryBuilingOccupied(AIPlayerJH::Coords(evb->GetX(), evb->GetY()));
			status = AIJH::JOB_FINISHED;
		}
		break;
	case AIEvent::BuildingLost:
		{
			AIEvent::Building *evb = dynamic_cast<AIEvent::Building *>(ev);
			aijh->HandleMilitaryBuilingLost(AIPlayerJH::Coords(evb->GetX(), evb->GetY()));
			status = AIJH::JOB_FINISHED;
		}
		break;
	case AIEvent::BuildingDestroyed:
		{//todo maybe do sth about it?
			//AIEvent::Building *evb = dynamic_cast<AIEvent::Building *>(ev);
			//aijh->HandleBuilingDestroyed(AIPlayerJH::Coords(evb->GetX(), evb->GetY()),evb->GetBuildingType());
			status = AIJH::JOB_FINISHED;
		}
		break;
	case AIEvent::NoMoreResourcesReachable:
		{
			AIEvent::Building *evb = dynamic_cast<AIEvent::Building *>(ev);
			aijh->HandleNoMoreResourcesReachable(AIPlayerJH::Coords(evb->GetX(), evb->GetY()), evb->GetBuildingType());
			status = AIJH::JOB_FINISHED;
		}
		break;
	case AIEvent::BorderChanged:
		{
			AIEvent::Building *evb = dynamic_cast<AIEvent::Building *>(ev);
			aijh->HandleBorderChanged(AIPlayerJH::Coords(evb->GetX(), evb->GetY()));
			status = AIJH::JOB_FINISHED;
		}
		break;
	case AIEvent::BuildingFinished:
		{
			AIEvent::Building *evb = dynamic_cast<AIEvent::Building *>(ev);
			aijh->HandleBuildingFinished(AIPlayerJH::Coords(evb->GetX(), evb->GetY()), evb->GetBuildingType());
			status = AIJH::JOB_FINISHED;
		}
		break;
	case AIEvent::ExpeditionWaiting:
		{
			AIEvent::Location *lvb = dynamic_cast<AIEvent::Location *>(ev);
			aijh->HandleExpedition(AIPlayerJH::Coords(lvb->GetX(), lvb->GetY()));
			status = AIJH::JOB_FINISHED;
		}
		break;
	case AIEvent::TreeChopped:
		{
			AIEvent::Location *lvb = dynamic_cast<AIEvent::Location *>(ev);
			aijh->HandleTreeChopped(AIPlayerJH::Coords(lvb->GetX(), lvb->GetY()));
			status = AIJH::JOB_FINISHED;
		}
		break;
	case AIEvent::ShipBuilt:
		{
			AIEvent::Location *lvb = dynamic_cast<AIEvent::Location *>(ev);
			aijh->HandleShipBuilt(AIPlayerJH::Coords(lvb->GetX(), lvb->GetY()));
			status = AIJH::JOB_FINISHED;
		}
		break;
	case AIEvent::RoadConstructionComplete:
		{
			AIEvent::Direction *dvb = dynamic_cast<AIEvent::Direction *>(ev);
			aijh->HandleRoadConstructionComplete(AIPlayerJH::Coords(dvb->GetX(), dvb->GetY()),dvb->GetDirection());
			status = AIJH::JOB_FINISHED;
		}
		break;
	default:
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
	const noFlag *flag = aii->GetSpecObj<noFlag>(flag_x, flag_y);

	if (!flag)
	{
#ifdef DEBUG_AI
			std::cout << "Flag is gone." << std::endl;
#endif
		status = AIJH::JOB_FAILED;
		return;
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
			// wait...
			return;
		}
	}
	else
	{
#ifdef DEBUG_AI
			std::cout << "Flag is connected." << std::endl;
#endif
		aijh->RecalcGround(flag_x, flag_y, route);
		status = AIJH::JOB_FINISHED;
		return;
	}
}


void AIJH::SearchJob::ExecuteJob()
{
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
		aijh->AddBuildJob(new BuildJob(aijh, search->bld, search->resultX, search->resultY, SEARCHMODE_NONE), true);
	}
}

AIJH::SearchJob::~SearchJob()
{ 
	delete search; 
}
