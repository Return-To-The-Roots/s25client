// $Id: AIConstruction.cpp 9590 2015-02-01 09:38:32Z marcus $
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
#include "AIConstruction.h"

#include "GameCommands.h"
#include "nobBaseMilitary.h"
#include "MapGeometry.h"
#include "nobHQ.h"
#include "AIPlayerJH.h"
#include "noBuildingSite.h"

// from Pathfinding.cpp
bool IsPointOK_RoadPath(const GameWorldBase& gwb, const MapCoord x, const MapCoord y, const unsigned char dir, const void* param);
bool IsPointOK_RoadPathEvenStep(const GameWorldBase& gwb, const MapCoord x, const MapCoord y, const unsigned char dir, const void* param);

AIConstruction::AIConstruction(AIInterface* aii, AIPlayerJH* aijh)
    : aii(aii), aijh(aijh)
{
    playerID = aii->GetPlayerID();
    buildingsWanted.resize(BUILDING_TYPES_COUNT);
    RefreshBuildingCount();
    InitBuildingsWanted();
}

AIConstruction::~AIConstruction(void)
{
}

void AIConstruction::AddBuildJob(AIJH::BuildJob* job, bool front)
{
	if(job->GetType()==BLD_SHIPYARD && aijh->IsInvalidShipyardPosition(job->GetAroundX(),job->GetAroundY()))
	{
		delete job;
		return;
	}
	if (job->GetType()<BLD_FORTRESS) //non military buildings can only be added once to the contruction que for every location
	{
		if (front)
			buildJobs.push_front(job);
		else
			buildJobs.push_back(job);
	}
	else //check if the buildjob is already in list and if so dont add it again
	{
		bool alreadyinlist=false;
		for(unsigned i=0;i<buildJobs.size();i++)
		{
			if(buildJobs[i]->GetType()==job->GetType() && buildJobs[i]->GetAroundX()==job->GetAroundX() && buildJobs[i]->GetAroundY()==job->GetAroundY())
			{
				alreadyinlist=true;
				break;
			}
		}
		if(!alreadyinlist)
		{
			if (front)
				buildJobs.push_front(job);
			else
				buildJobs.push_back(job);
		}
		else
		{
			//LOG.lprintf("duplicate buildorders type %i at %i,%i \n",job->GetType(),job->GetTargetX(),job->GetTargetY());
			delete job;
		}
	}
}

/*void AIConstruction::AddJob(AIJH::BuildJob* job, bool front)
{
    if (front)
        buildJobs.push_front(job);
    else
        buildJobs.push_back(job);

}*/

void AIConstruction::ExecuteJobs(unsigned limit)
{
	unsigned i=0; //count up to limit
	unsigned initconjobs = connectJobs.size()<5?connectJobs.size():5;
	unsigned initbuildjobs = buildJobs.size()<5?buildJobs.size():5;
	for(i;i<limit && connectJobs.size() && i < initconjobs ;i++) //go through list, until limit is reached or list empty or when every entry has been checked
	{
		connectJobs.front()->ExecuteJob();
		if(connectJobs.front()->GetStatus() != AIJH::JOB_FINISHED && connectJobs.front()->GetStatus() != AIJH::JOB_FAILED) //couldnt do job? -> move to back of list
		{
			connectJobs.push_back(connectJobs.front());
			connectJobs.pop_front();
		}
		else //job done of failed -> delete job and remove from list
		{
			AIJH::Job* job = connectJobs.front();
			connectJobs.pop_front();
			delete job;
		}
	}	
	for(i;i<limit && buildJobs.size() && i < (initconjobs+initbuildjobs) ;i++)
	{
		buildJobs.front()->ExecuteJob();
		if(buildJobs.front()->GetStatus() != AIJH::JOB_FINISHED && buildJobs.front()->GetStatus() != AIJH::JOB_FAILED) //couldnt do job? -> move to back of list
		{
			buildJobs.push_back(buildJobs.front());
			buildJobs.pop_front();
		}
		else //job done of failed -> delete job and remove from list
		{
			AIJH::Job* job = buildJobs.front();
			buildJobs.pop_front();
			delete job;
		}
	}
}

AIJH::Job* AIConstruction::GetBuildJob()
{
    if (buildJobs.empty())
        return NULL;

    AIJH::Job* job = buildJobs.front();
    buildJobs.pop_front();
    return job;
}

void AIConstruction::AddConnectFlagJob(const noFlag* flag)
{
	//already in list?
	for(unsigned i=0;i<connectJobs.size();i++)
	{		
		if(connectJobs[i]->getflagx()==flag->GetX() && connectJobs[i]->getflagy()==flag->GetY())
			return;
	}
	//add to list
    connectJobs.push_back(new AIJH::ConnectJob(aijh, flag->GetX(), flag->GetY()));
}

bool AIConstruction::CanStillConstructHere(MapCoord x, MapCoord y)
{
	for(unsigned i=0;i<constructionlocations.size();i+=2)
	{
		if(aii->CalcDistance(x,y,constructionlocations[i],constructionlocations[i+1])<12)
			return false;
	}
	return true;
}

void AIConstruction::FindFlags(std::vector<const noFlag*>& flags, unsigned short x, unsigned short y, unsigned short radius,
                               unsigned short real_x, unsigned short real_y, unsigned short real_radius, bool clear)
{
    if (clear)
        flags.clear();

    for(MapCoord tx = aii->GetXA(x, y, 0), r = 1; r <= radius; tx = aii->GetXA(tx, y, 0), ++r)
    {
        MapCoord tx2 = tx, ty2 = y;
        for(unsigned i = 2; i < 8; ++i)
        {
            for(MapCoord r2 = 0; r2 < r; aii->GetPointA(tx2, ty2, i % 6), ++r2)
            {
                if(aii->GetDistance(tx2, ty2, real_x, real_y) <= real_radius && aii->GetSpecObj<noFlag>(tx2, ty2))
                {
                    flags.push_back(aii->GetSpecObj<noFlag>(tx2, ty2));
                }
            }
        }
    }
}

void AIConstruction::FindFlags(std::vector<const noFlag*>& flags, unsigned short x, unsigned short y, unsigned short radius, bool clear)
{
    if (clear)
        flags.clear();

    // TODO Performance Killer!
    /*
    if (radius > 10)
    {
        list<nobBaseMilitary*> military;
        gwb->LookForMilitaryBuildings(military, x, y, 2);
        for(list<nobBaseMilitary*>::iterator it = military.begin();it.valid();++it)
        {
            unsigned distance = gwb->CalcDistance((*it)->GetX(), (*it)->GetY(), x, y);
            if (distance < radius && (*it)->GetPlayer() == player->getPlayerID())
            {
                FindFlags(flags, (*it)->GetX(), (*it)->GetY(), 10, x, y, radius, false);
            }
        }
    }
    */

    for(MapCoord tx = aii->GetXA(x, y, 0), r = 1; r <= radius; tx = aii->GetXA(tx, y, 0), ++r)
    {
        MapCoord tx2 = tx, ty2 = y;
        for(unsigned i = 2; i < 8; ++i)
        {
            for(MapCoord r2 = 0; r2 < r; aii->GetPointA(tx2, ty2, i % 6), ++r2)
            {
                const noFlag* flag = aii->GetSpecObj<noFlag>(tx2, ty2);
                if(flag && flag->GetPlayer() == playerID)
                {
                    flags.push_back(flag);
                    if (flags.size() > 30)
                    {
                        return;
                    }
                }
            }
        }
    }
}

bool AIConstruction::MilitaryBuildingWantsRoad(nobMilitary* milbld, unsigned listpos)
{
	if(milbld->GetFrontierDistance()>0) //close to front or harbor? connect!
		return true;
	if(aijh->UpgradeBldListNumber==listpos) // upgrade bld should have road already but just in case it doesnt -> get a road asap
		return true;
	if(aijh->UpgradeBldListNumber<0) //no upgrade bld on last update -> connect all that want to connect
		return true;
	if(listpos>(aii->GetMilitaryBuildings().size()-aijh->PlannedConnectedInlandMilitary()))
		return true;
	return false;
}

bool AIConstruction::ConnectFlagToRoadSytem(const noFlag* flag, std::vector<unsigned char>& route, unsigned int maxSearchRadius)
{
    // TODO: die methode kann  ganz schön böse Laufzeiten bekommen... Optimieren?

    // Radius in dem nach würdigen Fahnen gesucht wird
    //const unsigned short maxSearchRadius = 10;

	//flag of a military building? -> check if we really want to connect this right now
	if (aii->IsMilitaryBuildingOnNode(aii->GetXA(flag->GetX(),flag->GetY(),1),aii->GetYA(flag->GetX(),flag->GetY(),1)))
	{
		MapCoord mx=aii->GetXA(flag->GetX(),flag->GetY(),1);
		MapCoord my=aii->GetYA(flag->GetX(),flag->GetY(),1);
		unsigned listpos=0;
		for (std::list<nobMilitary*>::const_iterator it=aii->GetMilitaryBuildings().begin();it!=aii->GetMilitaryBuildings().end();it++)
		{
			if((*it)->GetX()==mx && (*it)->GetY()==my) 
			{
				if(!MilitaryBuildingWantsRoad((*it),listpos))
				{
					return false;
				}
				break;
			}
			listpos++;
		}
	}
    // Ziel, das möglichst schnell erreichbar sein soll
    //noFlag *targetFlag = gwb->GetSpecObj<nobHQ>(player->hqx, player->hqy)->GetFlag();
    noFlag* targetFlag = FindTargetStoreHouseFlag(flag->GetX(), flag->GetY());

    // Falls kein Lager mehr vorhanden ist, brauchen wir auch keinen Weg suchen
    if (!targetFlag)
        return false;

    // Flaggen in der Umgebung holen
    std::vector<const noFlag*> flags;
    FindFlags(flags, flag->GetX(), flag->GetY(), maxSearchRadius);

#ifdef DEBUG_AI
    std::cout << "FindFlagsNum: " << flags.size() << std::endl;
#endif

    unsigned shortest = 0;
    unsigned int shortestLength = 99999;
    std::vector<unsigned char> tmpRoute;
    bool found = false;

    // Jede Flagge testen...
    for(unsigned i = 0; i < flags.size(); ++i)
    {
        tmpRoute.clear();
        unsigned int length;
		// the flag should not be at a military building!		
		if (aii->IsMilitaryBuildingOnNode(aii->GetXA(flags[i]->GetX(),flags[i]->GetY(),1),aii->GetYA(flags[i]->GetX(),flags[i]->GetY(),1)))
			continue;
        // Gibts überhaupt einen Pfad zu dieser Flagge
        bool pathFound = aii->FindFreePathForNewRoad(flag->GetX(), flag->GetY(), flags[i]->GetX(), flags[i]->GetY(), &tmpRoute, &length);

        // Wenn ja, dann gucken ob dieser Pfad möglichst kurz zum "höheren" Ziel (allgemeines Lager im Moment) ist
        if (pathFound)
        {
            unsigned int distance = 0;
            unsigned size = 0;
            //check for non-flag points on planned route: more than 2 nonflaggable spaces on the route -> not really valid path
            unsigned temp = 0;
            MapCoord tx = flag->GetX();
            MapCoord ty = flag->GetY();
            for(unsigned j = 0; j < tmpRoute.size(); ++j)
            {
                aii->GetPointA(tx, ty, tmpRoute[j]);
                if(aii->GetBuildingQuality(tx, ty) < 1)
                    temp++;
                else
                {
                    if(size < temp)
                        size = temp;
                    temp = 0;
                }
            }
            if(size > 2)
                continue;

            // Strecke von der potenziellen Zielfahne bis zum Lager
            bool pathFound = aii->FindPathOnRoads(flags[i], targetFlag, &distance);

            // Gewählte Fahne hat leider auch kein Anschluß an ein Lager, zu schade!
            if (!pathFound)
                // Und ist auch nicht zufällig die Lager-Flagge selber...
                if (flags[i]->GetX() != targetFlag->GetX() || flags[i]->GetY() != targetFlag->GetY())
                    continue;

            // Sind wir mit der Fahne schon verbunden? Einmal reicht!
            if (aii->FindPathOnRoads(flags[i], flag))
                continue;

            // Ansonsten haben wir einen Pfad!
            found = true;

            // Kürzer als der letzte? Nehmen! Existierende Strecke höher gewichten (2), damit möglichst kurze Baustrecken
            // bevorzugt werden bei ähnlich langen Wegmöglichkeiten
            if (2 * length + distance + 10 * size < shortestLength)
            {
                shortest = i;
                shortestLength = 2 * length + distance + 10 * size;
                route = tmpRoute;
            }
        }
    }

    if (found)
    {
        //LOG.lprintf("ai build main road player %i at %i %i\n", flag->GetPlayer(), flag->GetX(),flag->GetY());
        return MinorRoadImprovements(flag, flags[shortest], route);
    }
    return false;
}

bool AIConstruction::MinorRoadImprovements(const noRoadNode* start, const noRoadNode* target, std::vector<unsigned char>&route)
{
	 return BuildRoad(start, target, route);
    //bool done=false;
    MapCoord x = start->GetX(), y = start->GetY();
    /*for(unsigned i=0;i<route.size();i++)
    {
        LOG.lprintf(" %i",route[i]);
    }
    LOG.lprintf("\n");*/
    for(unsigned i = 0; i < (route.size() - 1); i++)
    {
        if(((route[i] + 1) % 6 == route[i + 1]) || ((route[i] + 5) % 6 == route[i + 1])) //switching current and next route element will result in the same position after building both
        {
            MapCoord tx = x, ty = y;
            aii->GetPointA(tx, ty, route[i + 1]);
            aii->GetPointA(x, y, route[i]);
            if(aii->RoadAvailable(tx, ty, route[i + 1]) && aii->IsOwnTerritory(tx, ty)) //can the alternative road be build?
            {
                if(aii->CalcBQSumDifference(x, y, tx, ty)) //does the alternative road block a lower buildingquality point than the normal planned route?
                {
                    //LOG.lprintf("AIConstruction::road improvements p%i from %i,%i moved node %i,%i to %i,%i i:%i, i+1:%i\n",playerID, start->GetX(),start->GetY(),x,y,tx,ty,route[i],route[i+1]);
                    x = tx; //we move the alternative path so move x&y and switch the route entries
                    y = ty;
                    if((route[i] + 1) % 6 == route[i + 1])
                    {
                        route[i] = (route[i] + 1) % 6;
                        route[i + 1] = (route[i + 1] + 5) % 6;
                    }
                    else
                    {
                        route[i] = (route[i] + 5) % 6;
                        route[i + 1] = (route[i + 1] + 1) % 6;
                    }
                    //done=true;
                }
            }
        }
        else
            aii->GetPointA(x, y, route[i]);
    }
    /*if(done)
    {
        LOG.lprintf("final road\n");
        for(unsigned i=0;i<route.size();i++)
        {
            LOG.lprintf(" %i",route[i]);
        }
        LOG.lprintf("\n");
    }*/
    return BuildRoad(start, target, route);
}

bool AIConstruction::BuildRoad(const noRoadNode* start, const noRoadNode* target, std::vector<unsigned char> &route)
{
    bool foundPath;

    // Gucken obs einen Weg gibt
    if (route.empty())
    {
        foundPath = aii->FindFreePathForNewRoad(start->GetX(), start->GetY(), target->GetX(), target->GetY(), &route);
    }
    else
    {
        // Wenn Route übergeben wurde, davon ausgehen dass diese auch existiert
        foundPath = true;
    }

    // Wenn Pfad gefunden, Befehl zum Straße bauen und Flagen setzen geben
    if (foundPath)
    {
        MapCoord x = start->GetX();
        MapCoord y = start->GetY();
        aii->SetFlag(target->GetX(), target->GetY());
        aii->BuildRoad(x, y, route);
		//set flags along the road just after contruction - todo: handle failed road construction by removing the useless flags!
		/*MapCoord tx=x,ty=y;
		for(unsigned i=0;i<route.size()-2;i++)
		{
			x=aii->GetXA(tx,ty,route[i]);
			y=aii->GetXA(tx,ty,route[i]);
			tx=x;
			ty=y;
			if(i>0 && i%2==0)
				aii->SetFlag(tx,ty);
		}*/
        return true;
    }
    return false;
}

bool AIConstruction::IsConnectedToRoadSystem(const noFlag* flag)
{
    noFlag* targetFlag = this->FindTargetStoreHouseFlag(flag->GetX(), flag->GetY());
    if (targetFlag)
        return aii->FindPathOnRoads(flag, targetFlag);
    else
        return false;
}

BuildingType AIConstruction::ChooseMilitaryBuilding(MapCoord x, MapCoord y)
{
	//default : 2 barracks for each guardhouse
	//stones & low soldiers -> only guardhouse (no stones -> only barracks)
	//harbor nearby that could be used to attack/get attacked -> tower
	//enemy nearby? -> tower or fortress 
	//to do: important location or an area with a very low amount of buildspace? -> try large buildings
	//buildings with requirement > small have a chance to be replaced with small buildings to avoid getting stuck if there are no places for medium/large buildings
    BuildingType bld = BLD_BARRACKS;

    if (((rand() % 3) == 0 || aii->GetInventory()->people[JOB_PRIVATE] < 15) && (aii->GetInventory()->goods[GD_STONES] > 6 || GetBuildingCount(BLD_QUARRY) > 0))
        bld = BLD_GUARDHOUSE;
	if (aijh->HarborPosClose(x,y,20) && rand()%10!=0 && aijh->ggs->getSelection(ADDON_SEA_ATTACK) != 2)
	{
		bld = BLD_WATCHTOWER;
		return bld;
	}
	if(aijh->UpdateUpgradeBuilding()<0 && buildingCounts.building_site_counts[BLD_FORTRESS]<1 && (aii->GetInventory()->goods[GD_STONES] > 20 || GetBuildingCount(BLD_QUARRY) > 0) && rand()%10!=0)
	{
		bld = BLD_FORTRESS;
		return bld;
	}
    std::list<nobBaseMilitary*> military;
    aii->GetMilitaryBuildings(x, y, 3, military);
    for(std::list<nobBaseMilitary*>::iterator it = military.begin(); it != military.end(); ++it)
    {
        unsigned distance = aii->GetDistance((*it)->GetX(), (*it)->GetY(), x, y);

        // Prüfen ob Feind in der Nähe
        if ((*it)->GetPlayer() != playerID && distance < 35)
        {
            int randmil = rand();

            // avoid to build catapults in the beginning (no expansion)
            unsigned  militaryBuildingCount = GetBuildingCount(BLD_BARRACKS) + GetBuildingCount(BLD_GUARDHOUSE)
                                              + GetBuildingCount(BLD_WATCHTOWER) + GetBuildingCount(BLD_FORTRESS);
            //another catapult within "min" radius? ->dont build here!
            unsigned min = 16;
            nobBaseWarehouse* wh = (*aii->GetStorehouses().begin());
            if (aii->CalcDistance(x, y, wh->GetX(), wh->GetY()) < min)
                min = 0;
            for(std::list<nobUsual*>::const_iterator it = aii->GetBuildings(BLD_CATAPULT).begin(); min > 0 && it != aii->GetBuildings(BLD_CATAPULT).end(); it++)
            {
                if(aii->CalcDistance(x, y, (*it)->GetX(), (*it)->GetY()) < min)
                    min = 0;
            }
            for(std::list<noBuildingSite*>::const_iterator it = aii->GetBuildingSites().begin(); min > 0 && it != aii->GetBuildingSites().end(); it++)
            {
                if((*it)->GetBuildingType() == bld)
                {
                    if(aii->CalcDistance(x, y, (*it)->GetX(), (*it)->GetY()) < min)
                        min = 0;
                }
            }
            if (min > 0 && randmil % 8 == 0 && aii->CanBuildCatapult() && militaryBuildingCount > 5 && aii->GetInventory()->goods[GD_STONES] > 50 + (4 * GetBuildingCount(BLD_CATAPULT)))
            {
                bld = BLD_CATAPULT;
            }
            else
            {
                if (randmil % 2 == 0)
                    bld = BLD_FORTRESS;
                else
                    bld = BLD_WATCHTOWER;
            }
            //slim chance for a guardhouse instead of tower or fortress so we can expand towards an enemy even if there are no big building spots in that direction
            if(randmil % 10 == 0)
                bld = BLD_GUARDHOUSE;
            break;
        }
    }

    return bld;
}

unsigned AIConstruction::GetBuildingCount(BuildingType type)
{
    return buildingCounts.building_counts[type] + buildingCounts.building_site_counts[type];
}

unsigned AIConstruction::GetBuildingSitesCount(BuildingType type)
{
    return buildingCounts.building_site_counts[type];
}

bool AIConstruction::Wanted(BuildingType type)
{
	if (!aii->CanBuildBuildingtype(type))
		return false;
    if (type == BLD_CATAPULT)
        return aii->CanBuildCatapult() && (aii->GetInventory()->goods[GD_STONES] > 50 + (4 * GetBuildingCount(BLD_CATAPULT)));
    if ((type >= BLD_BARRACKS && type <= BLD_FORTRESS) || type == BLD_STOREHOUSE)
        //todo: find a better way to determine that there is no risk in expanding than sawmill up and complete
        return ((GetBuildingCount(BLD_BARRACKS) + GetBuildingCount(BLD_GUARDHOUSE) + GetBuildingCount(BLD_FORTRESS) + GetBuildingCount(BLD_WATCHTOWER) > 0 || buildingCounts.building_counts[BLD_SAWMILL] > 0 || (aii->GetInventory()->goods[GD_BOARDS] > 30 && GetBuildingCount(BLD_SAWMILL) > 0)) && MilitaryBuildingSitesLimit());
    if(type==BLD_SAWMILL && GetBuildingCount(BLD_SAWMILL)>1)
	{
		if (aijh->AmountInStorage(GD_WOOD,0) < 15*(buildingCounts.building_site_counts[BLD_SAWMILL]+1))
			return false;
	}
	return GetBuildingCount(type)+constructionorders[type] < buildingsWanted[type];
}

bool AIConstruction::MilitaryBuildingSitesLimit()
{
	unsigned complete = buildingCounts.building_counts[BLD_WATCHTOWER] + buildingCounts.building_counts[BLD_FORTRESS] + buildingCounts.building_counts[BLD_GUARDHOUSE] + buildingCounts.building_counts[BLD_BARRACKS];
	unsigned inconstruction = buildingCounts.building_site_counts[BLD_WATCHTOWER] + buildingCounts.building_site_counts[BLD_FORTRESS] + buildingCounts.building_site_counts[BLD_GUARDHOUSE] + buildingCounts.building_site_counts[BLD_BARRACKS];
	return complete+3 > inconstruction;
}

void AIConstruction::RefreshBuildingCount()
{
    unsigned resourcelimit = 0; //variables to make this more readable for humans
    unsigned bonuswant = 0;
	//max processing
    unsigned foodusers=GetBuildingCount(BLD_CHARBURNER)+GetBuildingCount(BLD_MILL)+GetBuildingCount(BLD_BREWERY)+GetBuildingCount(BLD_PIGFARM)+GetBuildingCount(BLD_DONKEYBREEDER);
	

    aii->GetBuildingCount(buildingCounts);
    //no military buildings -> usually start only
    if(aii->GetMilitaryBuildings().size() < 1 && aii->GetStorehouses().size() < 2)
    {
        buildingsWanted[BLD_FORESTER] = 1;
        buildingsWanted[BLD_SAWMILL] = 2; //probably only has 1 saw+carpenter but if that is the case the ai will try to produce 1 additional saw very quickly
        buildingsWanted[BLD_WOODCUTTER] = 1;
        buildingsWanted[BLD_QUARRY] = 2;
        buildingsWanted[BLD_GRANITEMINE] = 0;
        buildingsWanted[BLD_COALMINE] = 0;
        buildingsWanted[BLD_IRONMINE] = 0;
        buildingsWanted[BLD_GOLDMINE] = 0;
        buildingsWanted[BLD_CATAPULT] = 0;
        buildingsWanted[BLD_FISHERY] = 0;
        buildingsWanted[BLD_HUNTER] = 0;
        buildingsWanted[BLD_FARM] = 0;
        buildingsWanted[BLD_CHARBURNER] = 0;
    }
    else //at least some expansion happened -> more buildings wanted
        //building wanted usually limited by profession workers+tool for profession with some arbitrary limit. Some buildings which are linked to others in a chain / profession-tool-rivalry have additional limits.
    {
        //foresters
        resourcelimit = aii->GetInventory()->people[JOB_FORESTER] + aii->GetInventory()->goods[GD_SHOVEL] + 1; //bonuswant for foresters depends on addon settings for mines,wells,charburner
        bonuswant = GetBuildingCount(BLD_CHARBURNER) + ((!aijh->ggs->isEnabled(ADDON_INEXHAUSTIBLE_MINES) && ((GetBuildingCount(BLD_IRONMINE) + GetBuildingCount(BLD_COALMINE) + GetBuildingCount(BLD_GOLDMINE)) > 6)) ? 1 : 0) + ((aijh->ggs->isEnabled(ADDON_EXHAUSTIBLE_WELLS) && GetBuildingCount(BLD_WELL) > 3) ? 1 : 0);
        buildingsWanted[BLD_FORESTER] = max<int>((min<int>((aii->GetMilitaryBuildings().size() > 29 ? 5 : (aii->GetMilitaryBuildings().size() / 6) + 1) + bonuswant, resourcelimit)), 1);
		

		//earlygame: limit board use so limited to militarybuildingcount
        //woodcutters
		buildingsWanted[BLD_WOODCUTTER] = (aii->GetInventory()->goods[GD_AXE] + aii->GetInventory()->people[JOB_WOODCUTTER] + 1)>(aii->GetMilitaryBuildings().size()+1)?(aii->GetMilitaryBuildings().size()+1):(aii->GetInventory()->goods[GD_AXE] + aii->GetInventory()->people[JOB_WOODCUTTER] + 1);
		
		//on maps with many trees the ai will build woodcutters all over the place which means the foresters are not really required
		if((buildingsWanted[BLD_FORESTER]>1 && aii->GetMilitaryBuildings().size()<10 && buildingsWanted[BLD_WOODCUTTER]<GetBuildingCount(BLD_WOODCUTTER)+3) || (buildingsWanted[BLD_FORESTER]>1 && buildingsWanted[BLD_WOODCUTTER]<GetBuildingCount(BLD_WOODCUTTER)+2) )
		{
			buildingsWanted[BLD_FORESTER]=1;
		}


        //fishery & hunter
        buildingsWanted[BLD_FISHERY] = (aii->GetInventory()->goods[GD_RODANDLINE] + aii->GetInventory()->people[JOB_FISHER])>(aii->GetMilitaryBuildings().size()+1)?(aii->GetMilitaryBuildings().size()+1):(aii->GetInventory()->goods[GD_RODANDLINE] + aii->GetInventory()->people[JOB_FISHER]);
        buildingsWanted[BLD_HUNTER] = (aii->GetInventory()->goods[GD_BOW] + aii->GetInventory()->people[JOB_HUNTER] < 4) ? aii->GetInventory()->goods[GD_BOW] + aii->GetInventory()->people[JOB_HUNTER] : 4;

        //quarry: low ware games start at 2 otherwise build as many as we have stonemasons, higher ware games up to 6 quarries
        if(aii->GetInventory()->goods[GD_PICKAXE] + aii->GetInventory()->people[JOB_MINER] < 7 && aii->GetInventory()->people[JOB_STONEMASON] > 0 && aii->GetInventory()->people[JOB_MINER] < 3)
        {
            buildingsWanted[BLD_QUARRY] = aii->GetInventory()->people[JOB_STONEMASON] > 2 ? aii->GetInventory()->people[JOB_STONEMASON]>(aii->GetMilitaryBuildings().size())?(aii->GetMilitaryBuildings().size()):aii->GetInventory()->people[JOB_STONEMASON] : 2;
        }
        else
        {
            //>6miners = build up to 6 depending on resources, else max out at miners/2
            buildingsWanted[BLD_QUARRY] = (aii->GetInventory()->goods[GD_PICKAXE] + aii->GetInventory()->people[JOB_STONEMASON] < 6) ? ((aii->GetInventory()->people[JOB_MINER] > 6) ? aii->GetInventory()->goods[GD_PICKAXE] + aii->GetInventory()->people[JOB_STONEMASON] : aii->GetInventory()->people[JOB_MINER] / 2) : 6;
			if(buildingsWanted[BLD_QUARRY]>(aii->GetMilitaryBuildings().size()))
				buildingsWanted[BLD_QUARRY]=(aii->GetMilitaryBuildings().size());
		}
        //sawmills limited by woodcutters and carpenter+saws reduced by charburners minimum of 2
        resourcelimit = aii->GetInventory()->people[JOB_CARPENTER] + aii->GetInventory()->goods[GD_SAW];
        buildingsWanted[BLD_SAWMILL] = max<int>(min<int>((GetBuildingCount(BLD_WOODCUTTER) - (GetBuildingCount(BLD_CHARBURNER) * 2)) / 2, resourcelimit), 3); //min 2

        //ironsmelters limited by ironmines or crucibles
        buildingsWanted[BLD_IRONSMELTER] = (aii->GetInventory()->goods[GD_CRUCIBLE] + aii->GetInventory()->people[JOB_IRONFOUNDER] >= GetBuildingCount(BLD_IRONMINE)) ? GetBuildingCount(BLD_IRONMINE) : aii->GetInventory()->goods[GD_CRUCIBLE] + aii->GetInventory()->people[JOB_IRONFOUNDER];

        buildingsWanted[BLD_MINT] = GetBuildingCount(BLD_GOLDMINE);
        //armory count = smelter -metalworks if there is more than 1 smelter or 1 if there is just 1.
        buildingsWanted[BLD_ARMORY] = (GetBuildingCount(BLD_IRONSMELTER) > 1) ? GetBuildingCount(BLD_IRONSMELTER) - GetBuildingCount(BLD_METALWORKS) : GetBuildingCount(BLD_IRONSMELTER);
		if(aijh->ggs->isEnabled(ADDON_HALF_COST_MIL_EQUIP))
			buildingsWanted[BLD_ARMORY]*=2;
        //brewery count = 1+(armory/5) if there is at least 1 armory or armory /6 for exhaustible mines
        if(aijh->ggs->isEnabled(ADDON_INEXHAUSTIBLE_MINES))
            buildingsWanted[BLD_BREWERY] = (GetBuildingCount(BLD_ARMORY) > 0 && GetBuildingCount(BLD_FARM) > 0) ? 1 + (GetBuildingCount(BLD_ARMORY) / 5) : 0;
        else
            buildingsWanted[BLD_BREWERY] = (GetBuildingCount(BLD_ARMORY) > 0 && GetBuildingCount(BLD_FARM) > 0) ? 1 + (GetBuildingCount(BLD_ARMORY) / 6) : 0;
        //metalworks is 1 if there is at least 1 smelter, 2 if mines are inexhaustible and we have at least 4 ironsmelters
        buildingsWanted[BLD_METALWORKS] = (GetBuildingCount(BLD_IRONSMELTER) > 0) ? 1 : 0 ;

        if(buildingCounts.building_counts[BLD_FARM] >= buildingCounts.building_counts[BLD_PIGFARM] + buildingCounts.building_counts[BLD_DONKEYBREEDER] + buildingCounts.building_counts[BLD_BREWERY])
            buildingsWanted[BLD_MILL] = min(buildingCounts.building_counts[BLD_FARM] - (buildingCounts.building_counts[BLD_PIGFARM] + buildingCounts.building_counts[BLD_DONKEYBREEDER] + buildingCounts.building_counts[BLD_BREWERY]), GetBuildingCount(BLD_BAKERY) + 1);
        else
            buildingsWanted[BLD_MILL] = buildingCounts.building_counts[BLD_MILL];

        resourcelimit = aii->GetInventory()->people[JOB_BAKER] + aii->GetInventory()->goods[GD_ROLLINGPIN] + 1;
        buildingsWanted[BLD_BAKERY] = min<unsigned>(GetBuildingCount(BLD_MILL), resourcelimit);

        buildingsWanted[BLD_PIGFARM] = (buildingCounts.building_counts[BLD_FARM] < 8) ? buildingCounts.building_counts[BLD_FARM] / 4 : (buildingCounts.building_counts[BLD_FARM] - 2) / 4;
        if (buildingsWanted[BLD_PIGFARM] > GetBuildingCount(BLD_SLAUGHTERHOUSE) + 1)
            buildingsWanted[BLD_PIGFARM] = GetBuildingCount(BLD_SLAUGHTERHOUSE) + 1;
        buildingsWanted[BLD_SLAUGHTERHOUSE] = (GetBuildingCount(BLD_PIGFARM) > aii->GetInventory()->goods[GD_CLEAVER] + aii->GetInventory()->people[JOB_BUTCHER]) ? aii->GetInventory()->goods[GD_CLEAVER] + aii->GetInventory()->people[JOB_BUTCHER] : (GetBuildingCount(BLD_PIGFARM));

        buildingsWanted[BLD_WELL] = buildingsWanted[BLD_BAKERY] + buildingsWanted[BLD_PIGFARM] + buildingsWanted[BLD_DONKEYBREEDER] + buildingsWanted[BLD_BREWERY];

        buildingsWanted[BLD_FARM] = min<unsigned>(aii->GetInventory()->goods[GD_SCYTHE] + aii->GetInventory()->people[JOB_FARMER],foodusers+3);

        if(aii->GetInventory()->goods[GD_PICKAXE] + aii->GetInventory()->people[JOB_MINER] < 3)
        {
            //almost out of new pickaxes and miners - emergency program: get coal,iron,smelter&metalworks
            buildingsWanted[BLD_COALMINE] = 1;
            buildingsWanted[BLD_IRONMINE] = 1;
            buildingsWanted[BLD_GOLDMINE] = 0;
            buildingsWanted[BLD_IRONSMELTER] = 1;
            buildingsWanted[BLD_METALWORKS] = 1;
            buildingsWanted[BLD_ARMORY] = 0;
            buildingsWanted[BLD_GRANITEMINE] = 0;
            buildingsWanted[BLD_MINT] = 0;
        }
        else  //more than 2 miners
        {
            //coalmine count now depends on iron & gold not linked to food or material supply - might have to add a material check if this makes problems
            buildingsWanted[BLD_COALMINE] = (GetBuildingCount(BLD_IRONMINE) > 0) ? (GetBuildingCount(BLD_IRONMINE) * 2) - 1 + GetBuildingCount(BLD_GOLDMINE) : (GetBuildingCount(BLD_GOLDMINE) > 0) ? GetBuildingCount(BLD_GOLDMINE) : 1;
			//more mines planned than food available? -> limit mines
			if(buildingsWanted[BLD_COALMINE]>2 && buildingsWanted[BLD_COALMINE]*2 > aii->GetBuildings(BLD_FARM).size()+aii->GetBuildings(BLD_FISHERY).size()+1)
				buildingsWanted[BLD_COALMINE]=(aii->GetBuildings(BLD_FARM).size()+aii->GetBuildings(BLD_FISHERY).size())/2+2;
            if (GetBuildingCount(BLD_FARM) > 7) //quite the empire just scale mines with farms
            {
                if(aijh->ggs->isEnabled(ADDON_INEXHAUSTIBLE_MINES)) //inexhaustible mines? -> more farms required for each mine
                    buildingsWanted[BLD_IRONMINE] = (GetBuildingCount(BLD_FARM) * 2 / 5 > GetBuildingCount(BLD_IRONSMELTER) + 1) ? GetBuildingCount(BLD_IRONSMELTER) + 1 : GetBuildingCount(BLD_FARM) * 2 / 5;
                else
                    buildingsWanted[BLD_IRONMINE] = (GetBuildingCount(BLD_FARM) / 2 > GetBuildingCount(BLD_IRONSMELTER) + 1) ? GetBuildingCount(BLD_IRONSMELTER) + 1 : GetBuildingCount(BLD_FARM) / 2;
                buildingsWanted[BLD_GOLDMINE] = (GetBuildingCount(BLD_MINT) > 0) ? GetBuildingCount(BLD_IRONSMELTER) > 6 && GetBuildingCount(BLD_MINT) > 1 ? GetBuildingCount(BLD_IRONSMELTER) > 10 ? 4 : 3 : 2 : 1;
                buildingsWanted[BLD_DONKEYBREEDER] = 1;
                resourcelimit = aii->GetInventory()->people[JOB_CHARBURNER] + aii->GetInventory()->goods[GD_SHOVEL] + 1;
                if(aijh->ggs->isEnabled(ADDON_CHARBURNER) && (buildingsWanted[BLD_COALMINE] > GetBuildingCount(BLD_COALMINE) + 4))
                    buildingsWanted[BLD_CHARBURNER] = min<int>(min<int>(buildingsWanted[BLD_COALMINE] - (GetBuildingCount(BLD_COALMINE) + 1), 3), resourcelimit);
            }
            else
            {
                //probably still limited in food supply go up to 4 coal 1 gold 2 iron (gold+coal->coin, iron+coal->tool, iron+coal+coal->weapon)
                buildingsWanted[BLD_IRONMINE] = (aii->GetInventory()->people[JOB_MINER] + aii->GetInventory()->goods[GD_PICKAXE] - (GetBuildingCount(BLD_COALMINE) + GetBuildingCount(BLD_GOLDMINE)) > 1 && GetBuildingCount(BLD_BAKERY) + GetBuildingCount(BLD_SLAUGHTERHOUSE) + GetBuildingCount(BLD_HUNTER) + GetBuildingCount(BLD_FISHERY) > 4) ? 2 : 1;
                buildingsWanted[BLD_GOLDMINE] = (aii->GetInventory()->people[JOB_MINER] > 2) ? 1 : 0;				
                resourcelimit = aii->GetInventory()->people[JOB_CHARBURNER] + aii->GetInventory()->goods[GD_SHOVEL];
                if(aijh->ggs->isEnabled(ADDON_CHARBURNER) && (GetBuildingCount(BLD_COALMINE) < 1 && (GetBuildingCount(BLD_IRONMINE) + GetBuildingCount(BLD_GOLDMINE) > 0)))
                    buildingsWanted[BLD_CHARBURNER] = min<int>(1, resourcelimit);
            }
			if(GetBuildingCount(BLD_QUARRY)+1 < buildingsWanted[BLD_QUARRY] && aijh->AmountInStorage(GD_STONES,0)<100) //no quarry and low stones -> try granitemines.
            {
                buildingsWanted[BLD_GRANITEMINE] = (aii->GetInventory()->people[JOB_MINER] > 6 && buildingsWanted[BLD_QUARRY] > GetBuildingCount(BLD_QUARRY)) ? buildingsWanted[BLD_QUARRY] - GetBuildingCount(BLD_QUARRY) : 1;
				if(buildingsWanted[BLD_GRANITEMINE] >  (aii->GetMilitaryBuildings().size() / 15) + 1) //limit granitemines to military / 15
					buildingsWanted[BLD_GRANITEMINE] = (aii->GetMilitaryBuildings().size() / 15) + 1;
            }
            else
                buildingsWanted[BLD_GRANITEMINE] = 0;
        }
    }
    if(MAX_MILITARY_RANK - aijh->ggs->getSelection(ADDON_MAX_RANK) < 1)
    {
        buildingsWanted[BLD_GOLDMINE] = 0; // max rank is 0 = private / recruit ==> gold is useless!
    }
}

void AIConstruction::InitBuildingsWanted()
{
    buildingsWanted[BLD_FORESTER] = 1;
    buildingsWanted[BLD_SAWMILL] = 1;
    buildingsWanted[BLD_WOODCUTTER] = 12;
    buildingsWanted[BLD_QUARRY] = 6;
    buildingsWanted[BLD_GRANITEMINE] = 0;
    buildingsWanted[BLD_COALMINE] = 4;
    buildingsWanted[BLD_IRONMINE] = 2;
    buildingsWanted[BLD_GOLDMINE] = 1;
    buildingsWanted[BLD_CATAPULT] = 5;
    buildingsWanted[BLD_FISHERY] = 6;
    buildingsWanted[BLD_QUARRY] = 6;
    buildingsWanted[BLD_HUNTER] = 2;
    buildingsWanted[BLD_FARM] = aii->GetInventory()->goods[GD_SCYTHE] + aii->GetInventory()->people[JOB_FARMER];
    buildingsWanted[BLD_HARBORBUILDING] = 99;
    buildingsWanted[BLD_SHIPYARD] = aijh->GetCountofAIRelevantSeaIds() == 1 ? 1 : 99;
}

bool AIConstruction::BuildAlternativeRoad(const noFlag* flag, std::vector<unsigned char> &route)
{
    //LOG.lprintf("ai build alt road player %i at %i %i\n", flag->GetPlayer(), flag->GetX(),flag->GetY());
    // Radius in dem nach würdigen Fahnen gesucht wird
    const unsigned short maxRoadLength = 10;
    // Faktor um den der Weg kürzer sein muss als ein vorhander Pfad, um gebaut zu werden
    const unsigned short lengthFactor = 5;


    // Flaggen in der Umgebung holen
    std::vector<const noFlag*> flags;
    FindFlags(flags, flag->GetX(), flag->GetY(), maxRoadLength);
    std::vector<unsigned char>mainroad = route;
    //targetflag for mainroad
    MapCoord tx = flag->GetX(), tx2;
    MapCoord ty = flag->GetY(), ty2;
    for(unsigned i = 0; i < mainroad.size(); i++)
    {
        aii->GetPointA(tx, ty, mainroad[i]);
    }
    const noFlag* mainflag = aii->GetSpecObj<noFlag>(tx, ty);

    // Jede Flagge testen...
    for(unsigned i = 0; i < flags.size(); ++i)
    {
        //std::vector<unsigned char> new_route;
        route.clear();
        unsigned int newLength;
		// the flag should not be at a military building!		
		if (aii->IsMilitaryBuildingOnNode(aii->GetXA(flags[i]->GetX(),flags[i]->GetY(),1),aii->GetYA(flags[i]->GetX(),flags[i]->GetY(),1)))
			continue;
        // Gibts überhaupt einen Pfad zu dieser Flagge
        bool pathFound = aii->FindFreePathForNewRoad(flag->GetX(), flag->GetY(), flags[i]->GetX(), flags[i]->GetY(), &route, &newLength);

        // Wenn ja, dann gucken ob unser momentaner Weg zu dieser Flagge vielleicht voll weit ist und sich eine Straße lohnt
        if (pathFound)
        {
            unsigned int oldLength = 0;

            // Aktuelle Strecke zu der Flagge
            bool pathAvailable = aii->FindPathOnRoads(flags[i], flag, &oldLength);
            if(!pathAvailable && mainflag)
            {
                pathAvailable = aii->FindPathOnRoads(flags[i], mainflag, &oldLength);
                if(pathAvailable)
                    oldLength += mainroad.size();
            }
            bool crossmainpath = false;
            unsigned size = 0;
            //more than 5 nonflaggable spaces on the route -> not really valid path
            unsigned temp = 0;
            tx = flag->GetX();
            ty = flag->GetY();
            for(unsigned j = 0; j < route.size(); ++j)
            {
                aii->GetPointA(tx, ty, route[j]);
                tx2 = flag->GetX();
                ty2 = flag->GetY();
                //check if we cross the planned main road
                for(unsigned k = 0; k < mainroad.size(); ++k)
                {
                    aii->GetPointA(tx2, ty2, mainroad[k]);
                    if(tx2 == tx && ty2 == ty)
                    {
                        crossmainpath = true;
                        break;
                    }
                }
                if(aii->GetBuildingQuality(tx, ty) < 1)
                    temp++;
                else
                {
                    if(size < temp)
                        size = temp;
                    temp = 0;
                }
            }
            if(size > 2 || crossmainpath)
                continue;

            // Lohnt sich die Straße?
            if (!pathAvailable || newLength * lengthFactor < oldLength)
            {
                return BuildRoad(flag, flags[i], route);
            }
        }
    }

    return false;
}

bool AIConstruction::OtherUsualBuildingInRadius(MapCoord& x, MapCoord& y, unsigned radius, BuildingType bt)
{
	for (std::list<nobUsual*>::const_iterator it = aii->GetBuildings(bt).begin(); it != aii->GetBuildings(bt).end(); it++)
    {
        if(aii->CalcDistance((*it)->GetX(), (*it)->GetY(), x, y) < radius)
            return true;
    }
	for(std::list<noBuildingSite*>::const_iterator it = aii->GetBuildingSites().begin(); it != aii->GetBuildingSites().end(); it++)
    {
        if((*it)->GetBuildingType() == bt)
        {
            if(aii->CalcDistance((*it)->GetX(), (*it)->GetY(), x, y) < radius)
                return true;
        }
    }
	return false;
}

bool AIConstruction::OtherStoreInRadius(MapCoord& x, MapCoord& y, unsigned radius)
{
    for (std::list<nobBaseWarehouse*>::const_iterator it = aii->GetStorehouses().begin(); it != aii->GetStorehouses().end(); it++)
    {
        if(aii->CalcDistance((*it)->GetX(), (*it)->GetY(), x, y) < radius)
            return true;
    }
    for(std::list<noBuildingSite*>::const_iterator it = aii->GetBuildingSites().begin(); it != aii->GetBuildingSites().end(); it++)
    {
        if((*it)->GetBuildingType() == BLD_STOREHOUSE || (*it)->GetBuildingType() == BLD_HARBORBUILDING)
        {
            if(aii->CalcDistance((*it)->GetX(), (*it)->GetY(), x, y) < radius)
                return true;
        }
    }
    return false;
}

noFlag* AIConstruction::FindTargetStoreHouseFlag(MapCoord x, MapCoord y)
{
    unsigned minDistance = std::numeric_limits<unsigned>::max();
    nobBaseWarehouse* minTarget = NULL;
    bool found = false;
    for (std::list<nobBaseWarehouse*>::const_iterator it = aii->GetStorehouses().begin(); it != aii->GetStorehouses().end(); it++)
    {
        unsigned dist = aii->GetDistance(x, y, (*it)->GetX(), (*it)->GetY());
        if (dist < minDistance)
        {
            minDistance = dist;
            minTarget = *it;
            found = true;
        }
    }
    if (!found)
        return NULL;
    else
    {
        return minTarget->GetFlag();
    }
}
