// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "defines.h" // IWYU pragma: keep
#include "AIConstruction.h"
#include "GlobalGameSettings.h"
#include "Jobs.h"
#include "Point.h"
#include "addons/const_addons.h"
#include "ai/AIInterface.h"
#include "ai/aijh/AIPlayerJH.h"
#include "buildings/noBuildingSite.h"
#include "buildings/nobBaseMilitary.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobUsual.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noRoadNode.h"
#include "gameTypes/BuildingQuality.h"
#include "gameTypes/Direction.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/Inventory.h"
#include "gameTypes/JobTypes.h"
#include "gameData/BuildingProperties.h"
#include <boost/array.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/container/vector.hpp>
#include <boost/foreach.hpp>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <list>

namespace AIJH {

AIConstruction::AIConstruction(AIInterface& aii, AIPlayerJH& aijh)
    : aii(aii), aijh(aijh), buildingsWanted(BUILDING_TYPES_COUNT), constructionorders(BUILDING_TYPES_COUNT)
{
    InitMilitaryBldTypes();
    RefreshBuildingCount();
    InitBuildingsWanted();
}

AIConstruction::~AIConstruction()
{
}

void AIConstruction::AddBuildJob(BuildJob* job, bool front)
{
    if(job->GetType() == BLD_SHIPYARD && aijh.IsInvalidShipyardPosition(job->GetAround()))
    {
        delete job;
        return;
    }
    if(BuildingProperties::IsMilitary(
         job->GetType())) // non military buildings can only be added once to the contruction que for every location
    {
        if(front)
            buildJobs.push_front(job);
        else
            buildJobs.push_back(job);
    } else // check if the buildjob is already in list and if so dont add it again
    {
        bool alreadyinlist = false;
        for(unsigned i = 0; i < buildJobs.size(); i++)
        {
            if(buildJobs[i]->GetType() == job->GetType() && buildJobs[i]->GetAround() == job->GetAround())
            {
                alreadyinlist = true;
                break;
            }
        }
        if(!alreadyinlist)
        {
            if(front)
                buildJobs.push_front(job);
            else
                buildJobs.push_back(job);
        } else
        {
            // LOG.write(("duplicate buildorders type %i at %i,%i \n",job->GetType(),job->GetTargetX(),job->GetTargetY());
            delete job;
        }
    }
}

/*void AIConstruction::AddJob(BuildJob* job, bool front)
{
    if (front)
        buildJobs.push_front(job);
    else
        buildJobs.push_back(job);

}*/

void AIConstruction::ExecuteJobs(unsigned limit)
{
    unsigned i = 0; // count up to limit
    unsigned initconjobs = connectJobs.size() < 5 ? connectJobs.size() : 5;
    unsigned initbuildjobs = buildJobs.size() < 5 ? buildJobs.size() : 5;
    for(; i < limit && !connectJobs.empty() && i < initconjobs;
        i++) // go through list, until limit is reached or list empty or when every entry has been checked
    {
        ConnectJob* job = connectJobs.front();
        job->ExecuteJob();
        if(job->GetStatus() != JOB_FINISHED && job->GetStatus() != JOB_FAILED) // couldnt do job? -> move to back of list
        {
            connectJobs.push_back(job);
            connectJobs.pop_front();
        } else // job done of failed -> delete job and remove from list
        {
            connectJobs.pop_front();
            delete job;
        }
    }
    for(; i < limit && !buildJobs.empty() && i < (initconjobs + initbuildjobs); i++)
    {
        BuildJob* job = GetBuildJob();
        job->ExecuteJob();
        if(job->GetStatus() != JOB_FINISHED && job->GetStatus() != JOB_FAILED) // couldnt do job? -> move to back of list
        {
            buildJobs.push_back(job);
        } else // job done of failed -> delete job
            delete job;
    }
}

void AIConstruction::SetFlagsAlongRoad(const noRoadNode& roadNode, Direction dir)
{
    // does the roadsegment still exist?
    const RoadSegment& roadSeg = *roadNode.GetRoute(dir);
    bool isBwdDir = roadSeg.GetNodeID(roadNode);
    MapPoint curPos = roadNode.GetPos();
    // Skip first
    curPos = aii.GetNeighbour(curPos, roadSeg.GetDir(isBwdDir, 0));
    // Start after getting first neighbor (min distance == 2)
    // and skip last 2 points (min distance and last is flag)
    for(unsigned i = 1; i + 2 < roadSeg.GetLength(); ++i)
    {
        curPos = aii.GetNeighbour(curPos, roadSeg.GetDir(isBwdDir, i));
        aii.SetFlag(curPos);
        constructionlocations.push_back(curPos);
    }
}

void AIConstruction::InitMilitaryBldTypes()
{
    militaryBldTypes.clear();
    for(unsigned i = 0; i < BUILDING_TYPES_COUNT; i++)
    {
        BuildingType bld = BuildingType(i);
        if(BuildingProperties::IsMilitary(bld))
            militaryBldTypes.push_back(bld);
    }
}

unsigned AIConstruction::GetMilitaryBldCount() const
{
    unsigned result = 0;
    BOOST_FOREACH(BuildingType bld, militaryBldTypes)
        result += buildingCounts.buildings[bld];
    return result;
}

unsigned AIConstruction::GetMilitaryBldSiteCount() const
{
    unsigned result = 0;
    BOOST_FOREACH(BuildingType bld, militaryBldTypes)
        result += buildingCounts.buildingSites[bld];
    return result;
}

BuildJob* AIConstruction::GetBuildJob()
{
    if(buildJobs.empty())
        return NULL;

    BuildJob* job = buildJobs.front();
    buildJobs.pop_front();
    return job;
}

void AIConstruction::AddConnectFlagJob(const noFlag* flag)
{
    // already in list?
    for(unsigned i = 0; i < connectJobs.size(); i++)
    {
        if(connectJobs[i]->getFlag() == flag->GetPos())
            return;
    }
    // add to list
    connectJobs.push_back(new ConnectJob(aijh, flag->GetPos()));
}

bool AIConstruction::CanStillConstructHere(const MapPoint pt) const
{
    for(unsigned i = 0; i < constructionlocations.size(); i++)
    {
        if(aii.CalcDistance(pt, constructionlocations[i]) < 12)
            return false;
    }
    return true;
}

void AIConstruction::ConstructionOrdered(const BuildJob& job)
{
    RTTR_Assert(job.GetTarget().isValid());
    // add new construction area to the list of active orders in the current nwf
    constructionlocations.push_back(job.GetTarget());
    constructionorders[job.GetType()]++;
}

void AIConstruction::ConstructionsExecuted()
{
    constructionlocations.clear();
    std::fill(constructionorders.begin(), constructionorders.end(), 0u);
}

namespace {
    struct Point2FlagAI
    {
        typedef const noFlag* result_type;
        const AIInterface& aii_;

        Point2FlagAI(const AIInterface& aii) : aii_(aii) {}

        result_type operator()(const MapPoint pt, unsigned /*r*/) const { return aii_.GetSpecObj<noFlag>(pt); }
    };

    struct IsValidFlag
    {
        const unsigned playerId_;

        IsValidFlag(const unsigned playerId) : playerId_(playerId) {}

        bool operator()(const noFlag* const flag) const { return flag && flag->GetPlayer() == playerId_; }
    };
} // namespace

std::vector<const noFlag*> AIConstruction::FindFlags(const MapPoint pt, unsigned short radius)
{
    std::vector<const noFlag*> flags = aii.GetPointsInRadius<30>(pt, radius, Point2FlagAI(aii), IsValidFlag(aii.GetPlayerId()));

    // TODO Performance Killer!
    /*
    if (radius > 10)
    {
        list<nobBaseMilitary*> military;
        gwb->LookForMilitaryBuildings(military, pt, 2);
        for(list<nobBaseMilitary*>::iterator it = military.begin();it != military.end();++it)
        {
            unsigned distance = gwb->CalcDistance((*it)->GetPos(), pt);
            if (distance < radius && (*it)->GetPlayer() == player->getPlayerId())
            {
                FindFlags(flags, (*it)->GetPos(), 10, pt, radius, false);
            }
        }
    }
    */
    return flags;
}

bool AIConstruction::MilitaryBuildingWantsRoad(nobMilitary* milbld, unsigned listpos)
{
    if(milbld->GetFrontierDistance() > 0) // close to front or harbor? connect!
        return true;
    if(aijh.UpgradeBldListNumber < 0) // no upgrade bld on last update -> connect all that want to connect
        return true;
    if(static_cast<unsigned>(aijh.UpgradeBldListNumber)
       == listpos) // upgrade bld should have road already but just in case it doesnt -> get a road asap
        return true;
    if(listpos > (aii.GetMilitaryBuildings().size() - aijh.PlannedConnectedInlandMilitary()))
        return true;
    return false;
}

bool AIConstruction::ConnectFlagToRoadSytem(const noFlag* flag, std::vector<Direction>& route, unsigned maxSearchRadius /*= 14*/)
{
    // TODO: die methode kann  ganz schön böse Laufzeiten bekommen... Optimieren?

    // Radius in dem nach würdigen Fahnen gesucht wird
    // const unsigned short maxSearchRadius = 10;

    // flag of a military building? -> check if we really want to connect this right now
    const MapPoint bldPos = aii.GetNeighbour(flag->GetPos(), Direction::NORTHWEST);
    if(aii.IsMilitaryBuildingOnNode(bldPos))
    {
        unsigned listpos = 0;
        const std::list<nobMilitary*>& militaryBuildings = aii.GetMilitaryBuildings();
        for(std::list<nobMilitary*>::const_iterator it = militaryBuildings.begin(); it != militaryBuildings.end(); ++it, ++listpos)
        {
            if((*it)->GetPos() == bldPos)
            {
                if(!MilitaryBuildingWantsRoad(*it, listpos))
                    return false;
                break;
            }
        }
    }
    // Ziel, das möglichst schnell erreichbar sein soll
    // noFlag *targetFlag = gwb->GetSpecObj<nobHQ>(player->hqPos)->GetFlag();
    noFlag* targetFlag = FindTargetStoreHouseFlag(flag->GetPos());

    // Falls kein Lager mehr vorhanden ist, brauchen wir auch keinen Weg suchen
    if(!targetFlag)
        return false;

    // Flaggen in der Umgebung holen
    std::vector<const noFlag*> flags = FindFlags(flag->GetPos(), maxSearchRadius);

#ifdef DEBUG_AI
    std::cout << "FindFlagsNum: " << flags.size() << std::endl;
#endif

    const noFlag* shortest = NULL;
    unsigned shortestLength = 99999;
    std::vector<Direction> tmpRoute;
    bool found = false;

    // Jede Flagge testen...
    for(std::vector<const noFlag*>::iterator flagIt = flags.begin(); flagIt != flags.end(); ++flagIt)
    {
        const noFlag& curFlag = **flagIt;
        tmpRoute.clear();
        unsigned length;
        // the flag should not be at a military building!
        if(aii.IsMilitaryBuildingOnNode(aii.GetNeighbour(curFlag.GetPos(), Direction::NORTHWEST)))
            continue;
        // Gibts überhaupt einen Pfad zu dieser Flagge
        if(!aii.FindFreePathForNewRoad(flag->GetPos(), curFlag.GetPos(), &tmpRoute, &length))
            continue;

        // Wenn ja, dann gucken ob dieser Pfad möglichst kurz zum "höheren" Ziel (allgemeines Lager im Moment) ist
        unsigned maxNonFlagPts = 0;
        // check for non-flag points on planned route: more than 2 nonflaggable spaces on the route -> not really valid path
        unsigned curNonFlagPts = 0;
        MapPoint tmpPos = flag->GetPos();
        for(unsigned j = 0; j < tmpRoute.size(); ++j)
        {
            tmpPos = aii.GetNeighbour(tmpPos, tmpRoute[j]);
            if(aii.GetBuildingQuality(tmpPos) == BQ_NOTHING)
                curNonFlagPts++;
            else
            {
                if(maxNonFlagPts < curNonFlagPts)
                    maxNonFlagPts = curNonFlagPts;
                curNonFlagPts = 0;
            }
        }
        if(maxNonFlagPts > 2)
            continue;

        // Find path from current flag to target. If the current flag IS the target then we have already a path with distance=0
        unsigned distance = 0;
        bool pathFound = curFlag.GetObjId() == targetFlag->GetObjId() || aii.FindPathOnRoads(curFlag, *targetFlag, &distance);

        // Gewählte Fahne hat leider auch kein Anschluß an ein Lager, zu schade!
        if(!pathFound)
            continue;

        // Sind wir mit der Fahne schon verbunden? Einmal reicht!
        if(aii.FindPathOnRoads(curFlag, *flag))
            continue;

        // Ansonsten haben wir einen Pfad!
        found = true;

        // Kürzer als der letzte? Nehmen! Existierende Strecke höher gewichten (2), damit möglichst kurze Baustrecken
        // bevorzugt werden bei ähnlich langen Wegmöglichkeiten
        if(2 * length + distance + 10 * maxNonFlagPts < shortestLength)
        {
            shortest = &curFlag;
            shortestLength = 2 * length + distance + 10 * maxNonFlagPts;
            route = tmpRoute;
        }
    }

    if(found)
    {
        // LOG.write(("ai build main road player %i at %i %i\n", flag->GetPlayer(), flag->GetPos());
        if(!MinorRoadImprovements(flag, shortest, route))
            return false;
        // add new construction area to the list of active orders in the current nwf
        // to wait till path is constructed
        constructionlocations.push_back(flag->GetPos());
        constructionlocations.push_back(shortest->GetPos());
        return true;
    }
    return false;
}

bool AIConstruction::MinorRoadImprovements(const noRoadNode* start, const noRoadNode* target, std::vector<Direction>& route)
{
    return BuildRoad(start, target, route);
    // bool done=false;
    MapPoint pStart = start->GetPos();
    /*for(unsigned i=0;i<route.size();i++)
    {
        LOG.write((" %i",route[i]);
    }
    LOG.write(("\n");*/
    for(unsigned i = 0; i + 1 < route.size(); i++)
    {
        if((route[i] + 1u == route[i + 1])
           || (route[i] - 1u
               == route[i + 1])) // switching current and next route element will result in the same position after building both
        {
            MapPoint t(pStart);
            t = aii.GetNeighbour(t, route[i + 1]);
            pStart = aii.GetNeighbour(pStart, route[i]);
            if(aii.RoadAvailable(t) && aii.IsOwnTerritory(t)) // can the alternative road be build?
            {
                if(aii.CalcBQSumDifference(
                     pStart, t)) // does the alternative road block a lower buildingquality point than the normal planned route?
                {
                    // LOG.write(("AIConstruction::road improvements p%i from %i,%i moved node %i,%i to %i,%i i:%i, i+1:%i\n",playerID,
                    // start->GetX(), start->GetY(), ptx, pt.y, t.x, t.y,route[i],route[i+1]);
                    pStart = t; // we move the alternative path so move x&y and switch the route entries
                    if(route[i] + 1u == route[i + 1])
                    {
                        ++route[i];
                        --route[i + 1];
                    } else
                    {
                        --route[i];
                        ++route[i + 1];
                    }
                    // done=true;
                }
            }
        } else
            pStart = aii.GetNeighbour(pStart, route[i]);
    }
    /*if(done)
    {
        LOG.write(("final road\n");
        for(unsigned i=0;i<route.size();i++)
        {
            LOG.write((" %i",route[i]);
        }
        LOG.write(("\n");
    }*/
    return BuildRoad(start, target, route);
}

bool AIConstruction::BuildRoad(const noRoadNode* start, const noRoadNode* target, std::vector<Direction>& route)
{
    bool foundPath;

    // Gucken obs einen Weg gibt
    if(route.empty())
    {
        foundPath = aii.FindFreePathForNewRoad(start->GetPos(), target->GetPos(), &route);
    } else
    {
        // Wenn Route übergeben wurde, davon ausgehen dass diese auch existiert
        foundPath = true;
    }

    // Wenn Pfad gefunden, Befehl zum Straße bauen und Flagen setzen geben
    if(foundPath)
    {
        aii.SetFlag(target->GetPos());
        aii.BuildRoad(start->GetPos(), false, route);
        // set flags along the road just after contruction - todo: handle failed road construction by removing the useless flags!
        /*MapCoord tx=x,ty=y;
        for(unsigned i=0;i<route.size()-2;i++)
        {
            x=aii.GetXA(tx,ty,route[i]);
            y=aii.GetXA(tx,ty,route[i]);
            tx=x;
            ty=y;
            if(i>0 && i%2==0)
                aii.SetFlag(tx,ty);
        }*/
        return true;
    }
    return false;
}

bool AIConstruction::IsConnectedToRoadSystem(const noFlag* flag) const
{
    noFlag* targetFlag = FindTargetStoreHouseFlag(flag->GetPos());
    if(targetFlag)
        return (targetFlag == flag) || aii.FindPathOnRoads(*flag, *targetFlag);
    else
        return false;
}

BuildingType AIConstruction::GetSmallestAllowedMilBuilding() const
{
    BOOST_FOREACH(BuildingType bld, militaryBldTypes)
    {
        if(aii.CanBuildBuildingtype(bld))
            return bld;
    }
    return BLD_NOTHING;
}

BuildingType AIConstruction::GetBiggestAllowedMilBuilding() const
{
    BOOST_REVERSE_FOREACH(BuildingType bld, militaryBldTypes)
    {
        if(aii.CanBuildBuildingtype(bld))
            return bld;
    }
    return BLD_NOTHING;
}

BuildingType AIConstruction::ChooseMilitaryBuilding(const MapPoint pt)
{
    // default : 2 barracks for each guardhouse
    // stones & low soldiers -> only guardhouse (no stones -> only barracks)
    // harbor nearby that could be used to attack/get attacked -> tower
    // enemy nearby? -> tower or fortress
    // to do: important location or an area with a very low amount of buildspace? -> try large buildings
    // buildings with requirement > small have a chance to be replaced with small buildings to avoid getting stuck if there are no places
    // for medium/large buildings
    BuildingType bld = GetSmallestAllowedMilBuilding();
    // If we are not allowed to build a military building, return early
    if(bld == BLD_NOTHING)
        return BLD_NOTHING;

    const BuildingType biggestBld = GetBiggestAllowedMilBuilding();

    const Inventory& inventory = aii.GetInventory();
    if(((rand() % 3) == 0 || inventory.people[JOB_PRIVATE] < 15) && (inventory.goods[GD_STONES] > 6 || GetBuildingCount(BLD_QUARRY) > 0))
        bld = BLD_GUARDHOUSE;
    if(aijh.HarborPosClose(pt, 20) && rand() % 10 != 0 && aijh.ggs.getSelection(AddonId::SEA_ATTACK) != 2)
    {
        if(aii.CanBuildBuildingtype(BLD_WATCHTOWER))
            return BLD_WATCHTOWER;
        return GetBiggestAllowedMilBuilding();
    }
    if(biggestBld == BLD_WATCHTOWER || biggestBld == BLD_FORTRESS)
    {
        if(aijh.UpdateUpgradeBuilding() < 0 && buildingCounts.buildingSites[biggestBld] < 1
           && (inventory.goods[GD_STONES] > 20 || GetBuildingCount(BLD_QUARRY) > 0) && rand() % 10 != 0)
        {
            return biggestBld;
        }
    }

    // avoid to build catapults in the beginning (no expansion)
    const unsigned militaryBuildingCount = GetMilitaryBldCount() + GetMilitaryBldSiteCount();

    uint8_t playerId = aii.GetPlayerId();
    sortedMilitaryBlds military = aii.GetMilitaryBuildings(pt, 3);
    for(sortedMilitaryBlds::iterator it = military.begin(); it != military.end(); ++it)
    {
        unsigned distance = aii.GetDistance((*it)->GetPos(), pt);

        // Prüfen ob Feind in der Nähe
        if((*it)->GetPlayer() != playerId && distance < 35)
        {
            int randmil = rand();

            // another catapult within "min" radius? ->dont build here!
            unsigned min = 16;
            nobBaseWarehouse* wh = (*aii.GetStorehouses().begin());
            if(aii.CalcDistance(pt, wh->GetPos()) < min)
                min = 0;
            for(std::list<nobUsual*>::const_iterator it = aii.GetBuildings(BLD_CATAPULT).begin();
                min > 0 && it != aii.GetBuildings(BLD_CATAPULT).end(); ++it)
            {
                if(aii.CalcDistance(pt, (*it)->GetPos()) < min)
                    min = 0;
            }
            for(std::list<noBuildingSite*>::const_iterator it = aii.GetBuildingSites().begin();
                min > 0 && it != aii.GetBuildingSites().end(); ++it)
            {
                if((*it)->GetBuildingType() == bld)
                {
                    if(aii.CalcDistance(pt, (*it)->GetPos()) < min)
                        min = 0;
                }
            }
            if(min > 0 && randmil % 8 == 0 && aii.CanBuildCatapult() && militaryBuildingCount > 5
               && inventory.goods[GD_STONES] > 50 + (4 * GetBuildingCount(BLD_CATAPULT)))
            {
                bld = BLD_CATAPULT;
            } else
            {
                if(randmil % 2 == 0)
                    bld = biggestBld;
                else if(aii.CanBuildBuildingtype(BLD_WATCHTOWER))
                    bld = BLD_WATCHTOWER;
                else
                    bld = biggestBld;
            }
            // slim chance for a guardhouse instead of tower or fortress so we can expand towards an enemy even if there are no big building
            // spots in that direction
            if(randmil % 10 == 0)
            {
                if(aii.CanBuildBuildingtype(BLD_GUARDHOUSE))
                    bld = BLD_GUARDHOUSE;
                else
                    bld = GetSmallestAllowedMilBuilding();
            }
            break;
        }
    }

    return bld;
}

unsigned AIConstruction::GetBuildingCount(BuildingType type) const
{
    return buildingCounts.buildings[type] + buildingCounts.buildingSites[type];
}

unsigned AIConstruction::GetBuildingSitesCount(BuildingType type) const
{
    return buildingCounts.buildingSites[type];
}

bool AIConstruction::Wanted(BuildingType type) const
{
    if(!aii.CanBuildBuildingtype(type))
        return false;
    if(type == BLD_CATAPULT)
        return aii.CanBuildCatapult() && (aii.GetInventory().goods[GD_STONES] > 50 + (4 * GetBuildingCount(BLD_CATAPULT)));
    if(BuildingProperties::IsMilitary(type) || type == BLD_STOREHOUSE)
        // todo: find a better way to determine that there is no risk in expanding than sawmill up and complete
        return ((GetMilitaryBldCount() + GetMilitaryBldSiteCount() > 0 || buildingCounts.buildings[BLD_SAWMILL] > 0
                 || (aii.GetInventory().goods[GD_BOARDS] > 30 && GetBuildingCount(BLD_SAWMILL) > 0))
                && WantMoreMilitaryBlds());
    if(type == BLD_SAWMILL && GetBuildingCount(BLD_SAWMILL) > 1)
    {
        if(aijh.AmountInStorage(GD_WOOD, 0) < 15 * (buildingCounts.buildingSites[BLD_SAWMILL] + 1))
            return false;
    }
    return GetBuildingCount(type) + constructionorders[type] < buildingsWanted[type];
}

bool AIConstruction::WantMoreMilitaryBlds() const
{
    unsigned complete = GetMilitaryBldCount();
    unsigned inconstruction = GetMilitaryBldSiteCount();
    return complete + 3 > inconstruction;
}

void AIConstruction::RefreshBuildingCount()
{
    // max processing
    unsigned foodusers = GetBuildingCount(BLD_CHARBURNER) + GetBuildingCount(BLD_MILL) + GetBuildingCount(BLD_BREWERY)
                         + GetBuildingCount(BLD_PIGFARM) + GetBuildingCount(BLD_DONKEYBREEDER);

    buildingCounts = aii.GetBuildingCount();
    // no military buildings -> usually start only
    const std::list<nobMilitary*>& militaryBuildings = aii.GetMilitaryBuildings();

    if(militaryBuildings.empty() && aii.GetStorehouses().size() < 2)
    {
        buildingsWanted[BLD_FORESTER] = 1;
        buildingsWanted[BLD_SAWMILL] =
          2; // probably only has 1 saw+carpenter but if that is the case the ai will try to produce 1 additional saw very quickly
        buildingsWanted[BLD_WOODCUTTER] = 2;
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
    } else // at least some expansion happened -> more buildings wanted
           // building wanted usually limited by profession workers+tool for profession with some arbitrary limit. Some buildings which are
           // linked to others in a chain / profession-tool-rivalry have additional limits.
    {
        const Inventory& inventory = aii.GetInventory();

        // foresters
        unsigned max_available_forester = inventory.people[JOB_FORESTER] + inventory.goods[GD_SHOVEL];
        unsigned additional_forester = GetBuildingCount(BLD_CHARBURNER);

        // 1 mil -> 1 forester, 2 mil -> 2 forester, 4 mil -> 3 forester, 8 mil -> 4 forester, 16 mil -> 5 forester, ... wanted
        if(!militaryBuildings.empty())
            buildingsWanted[BLD_FORESTER] = (unsigned)(1.45 * log(militaryBuildings.size()) + 1);

        buildingsWanted[BLD_FORESTER] += additional_forester;
        buildingsWanted[BLD_FORESTER] = min<int>(max_available_forester, buildingsWanted[BLD_FORESTER]);

        // earlygame: limit board use so limited to militarybuildingcount
        // woodcutters
        unsigned max_available_woodcutter = inventory.goods[GD_AXE] + inventory.people[JOB_WOODCUTTER];
        buildingsWanted[BLD_WOODCUTTER] = buildingsWanted[BLD_FORESTER] * 3; // two per forester + 1 for 'natural' forest
        buildingsWanted[BLD_WOODCUTTER] = min<int>(max_available_woodcutter, buildingsWanted[BLD_WOODCUTTER]);

        ////on maps with many trees the ai will build woodcutters all over the place which means the foresters are not really required
        // TODO: get number of trees in own territory. use it relatively to total size of own territory to adapt number of foresters (less)
        // and woodcutters (more)

        // fishery & hunter
        buildingsWanted[BLD_FISHERY] = (inventory.goods[GD_RODANDLINE] + inventory.people[JOB_FISHER]) > (militaryBuildings.size() + 1) ?
                                         (militaryBuildings.size() + 1) :
                                         (inventory.goods[GD_RODANDLINE] + inventory.people[JOB_FISHER]);
        buildingsWanted[BLD_HUNTER] =
          (inventory.goods[GD_BOW] + inventory.people[JOB_HUNTER] < 4) ? inventory.goods[GD_BOW] + inventory.people[JOB_HUNTER] : 4;

        // quarry: low ware games start at 2 otherwise build as many as we have stonemasons, higher ware games up to 6 quarries
        if(inventory.goods[GD_PICKAXE] + inventory.people[JOB_MINER] < 7 && inventory.people[JOB_STONEMASON] > 0
           && inventory.people[JOB_MINER] < 3)
        {
            buildingsWanted[BLD_QUARRY] = inventory.people[JOB_STONEMASON] > 2 ?
                                            inventory.people[JOB_STONEMASON] > (militaryBuildings.size()) ?
                                            (militaryBuildings.size()) :
                                            inventory.people[JOB_STONEMASON] :
                                            2;
        } else
        {
            //>6miners = build up to 6 depending on resources, else max out at miners/2
            buildingsWanted[BLD_QUARRY] =
              (inventory.goods[GD_PICKAXE] + inventory.people[JOB_STONEMASON] < 6) ?
                ((inventory.people[JOB_MINER] > 6) ? inventory.goods[GD_PICKAXE] + inventory.people[JOB_STONEMASON] :
                                                     inventory.people[JOB_MINER] / 2) :
                6;
            if(buildingsWanted[BLD_QUARRY] > militaryBuildings.size())
                buildingsWanted[BLD_QUARRY] = militaryBuildings.size();
        }
        // sawmills limited by woodcutters and carpenter+saws reduced by charburners minimum of 2
        unsigned resourcelimit = inventory.people[JOB_CARPENTER] + inventory.goods[GD_SAW];
        buildingsWanted[BLD_SAWMILL] =
          max<int>(min<int>((GetBuildingCount(BLD_WOODCUTTER) - (GetBuildingCount(BLD_CHARBURNER) * 2)) / 2, resourcelimit), 3); // min 2

        // ironsmelters limited by ironmines or crucibles
        buildingsWanted[BLD_IRONSMELTER] =
          (inventory.goods[GD_CRUCIBLE] + inventory.people[JOB_IRONFOUNDER] >= GetBuildingCount(BLD_IRONMINE)) ?
            GetBuildingCount(BLD_IRONMINE) :
            inventory.goods[GD_CRUCIBLE] + inventory.people[JOB_IRONFOUNDER];

        buildingsWanted[BLD_MINT] = GetBuildingCount(BLD_GOLDMINE);
        // armory count = smelter -metalworks if there is more than 1 smelter or 1 if there is just 1.
        buildingsWanted[BLD_ARMORY] = (GetBuildingCount(BLD_IRONSMELTER) > 1) ?
                                        GetBuildingCount(BLD_IRONSMELTER) - GetBuildingCount(BLD_METALWORKS) :
                                        GetBuildingCount(BLD_IRONSMELTER);
        if(aijh.ggs.isEnabled(AddonId::HALF_COST_MIL_EQUIP))
            buildingsWanted[BLD_ARMORY] *= 2;
        // brewery count = 1+(armory/5) if there is at least 1 armory or armory /6 for exhaustible mines
        if(aijh.ggs.isEnabled(AddonId::INEXHAUSTIBLE_MINES))
            buildingsWanted[BLD_BREWERY] =
              (GetBuildingCount(BLD_ARMORY) > 0 && GetBuildingCount(BLD_FARM) > 0) ? 1 + (GetBuildingCount(BLD_ARMORY) / 5) : 0;
        else
            buildingsWanted[BLD_BREWERY] =
              (GetBuildingCount(BLD_ARMORY) > 0 && GetBuildingCount(BLD_FARM) > 0) ? 1 + (GetBuildingCount(BLD_ARMORY) / 6) : 0;
        // metalworks is 1 if there is at least 1 smelter, 2 if mines are inexhaustible and we have at least 4 ironsmelters
        buildingsWanted[BLD_METALWORKS] = (GetBuildingCount(BLD_IRONSMELTER) > 0) ? 1 : 0;

        if(buildingCounts.buildings[BLD_FARM]
           >= buildingCounts.buildings[BLD_PIGFARM] + buildingCounts.buildings[BLD_DONKEYBREEDER] + buildingCounts.buildings[BLD_BREWERY])
            buildingsWanted[BLD_MILL] = std::min(buildingCounts.buildings[BLD_FARM]
                                                   - (buildingCounts.buildings[BLD_PIGFARM] + buildingCounts.buildings[BLD_DONKEYBREEDER]
                                                      + buildingCounts.buildings[BLD_BREWERY]),
                                                 GetBuildingCount(BLD_BAKERY) + 1);
        else
            buildingsWanted[BLD_MILL] = buildingCounts.buildings[BLD_MILL];

        resourcelimit = inventory.people[JOB_BAKER] + inventory.goods[GD_ROLLINGPIN] + 1;
        buildingsWanted[BLD_BAKERY] = min<unsigned>(GetBuildingCount(BLD_MILL), resourcelimit);

        buildingsWanted[BLD_PIGFARM] =
          (buildingCounts.buildings[BLD_FARM] < 8) ? buildingCounts.buildings[BLD_FARM] / 4 : (buildingCounts.buildings[BLD_FARM] - 2) / 4;
        if(buildingsWanted[BLD_PIGFARM] > GetBuildingCount(BLD_SLAUGHTERHOUSE) + 1)
            buildingsWanted[BLD_PIGFARM] = GetBuildingCount(BLD_SLAUGHTERHOUSE) + 1;
        buildingsWanted[BLD_SLAUGHTERHOUSE] =
          (GetBuildingCount(BLD_PIGFARM) > inventory.goods[GD_CLEAVER] + inventory.people[JOB_BUTCHER]) ?
            inventory.goods[GD_CLEAVER] + inventory.people[JOB_BUTCHER] :
            (GetBuildingCount(BLD_PIGFARM));

        buildingsWanted[BLD_WELL] =
          buildingsWanted[BLD_BAKERY] + buildingsWanted[BLD_PIGFARM] + buildingsWanted[BLD_DONKEYBREEDER] + buildingsWanted[BLD_BREWERY];

        buildingsWanted[BLD_FARM] = min<unsigned>(inventory.goods[GD_SCYTHE] + inventory.people[JOB_FARMER], foodusers + 3);

        if(inventory.goods[GD_PICKAXE] + inventory.people[JOB_MINER] < 3)
        {
            // almost out of new pickaxes and miners - emergency program: get coal,iron,smelter&metalworks
            buildingsWanted[BLD_COALMINE] = 1;
            buildingsWanted[BLD_IRONMINE] = 1;
            buildingsWanted[BLD_GOLDMINE] = 0;
            buildingsWanted[BLD_IRONSMELTER] = 1;
            buildingsWanted[BLD_METALWORKS] = 1;
            buildingsWanted[BLD_ARMORY] = 0;
            buildingsWanted[BLD_GRANITEMINE] = 0;
            buildingsWanted[BLD_MINT] = 0;
        } else // more than 2 miners
        {
            // coalmine count now depends on iron & gold not linked to food or material supply - might have to add a material check if this
            // makes problems
            buildingsWanted[BLD_COALMINE] = (GetBuildingCount(BLD_IRONMINE) > 0) ?
                                              (GetBuildingCount(BLD_IRONMINE) * 2) - 1 + GetBuildingCount(BLD_GOLDMINE) :
                                              (GetBuildingCount(BLD_GOLDMINE) > 0) ? GetBuildingCount(BLD_GOLDMINE) : 1;
            // more mines planned than food available? -> limit mines
            if(buildingsWanted[BLD_COALMINE] > 2
               && buildingsWanted[BLD_COALMINE] * 2 > aii.GetBuildings(BLD_FARM).size() + aii.GetBuildings(BLD_FISHERY).size() + 1)
                buildingsWanted[BLD_COALMINE] = (aii.GetBuildings(BLD_FARM).size() + aii.GetBuildings(BLD_FISHERY).size()) / 2 + 2;
            if(GetBuildingCount(BLD_FARM) > 7) // quite the empire just scale mines with farms
            {
                if(aijh.ggs.isEnabled(AddonId::INEXHAUSTIBLE_MINES)) // inexhaustible mines? -> more farms required for each mine
                    buildingsWanted[BLD_IRONMINE] = (GetBuildingCount(BLD_FARM) * 2 / 5 > GetBuildingCount(BLD_IRONSMELTER) + 1) ?
                                                      GetBuildingCount(BLD_IRONSMELTER) + 1 :
                                                      GetBuildingCount(BLD_FARM) * 2 / 5;
                else
                    buildingsWanted[BLD_IRONMINE] = (GetBuildingCount(BLD_FARM) / 2 > GetBuildingCount(BLD_IRONSMELTER) + 1) ?
                                                      GetBuildingCount(BLD_IRONSMELTER) + 1 :
                                                      GetBuildingCount(BLD_FARM) / 2;
                buildingsWanted[BLD_GOLDMINE] = (GetBuildingCount(BLD_MINT) > 0) ?
                                                  GetBuildingCount(BLD_IRONSMELTER) > 6 && GetBuildingCount(BLD_MINT) > 1 ?
                                                  GetBuildingCount(BLD_IRONSMELTER) > 10 ? 4 : 3 :
                                                  2 :
                                                  1;
                buildingsWanted[BLD_DONKEYBREEDER] = 1;
                resourcelimit = inventory.people[JOB_CHARBURNER] + inventory.goods[GD_SHOVEL] + 1;
                if(aijh.ggs.isEnabled(AddonId::CHARBURNER) && (buildingsWanted[BLD_COALMINE] > GetBuildingCount(BLD_COALMINE) + 4))
                    buildingsWanted[BLD_CHARBURNER] =
                      min<int>(min<int>(buildingsWanted[BLD_COALMINE] - (GetBuildingCount(BLD_COALMINE) + 1), 3), resourcelimit);
            } else
            {
                // probably still limited in food supply go up to 4 coal 1 gold 2 iron (gold+coal->coin, iron+coal->tool,
                // iron+coal+coal->weapon)
                buildingsWanted[BLD_IRONMINE] = (inventory.people[JOB_MINER] + inventory.goods[GD_PICKAXE]
                                                     - (GetBuildingCount(BLD_COALMINE) + GetBuildingCount(BLD_GOLDMINE))
                                                   > 1
                                                 && GetBuildingCount(BLD_BAKERY) + GetBuildingCount(BLD_SLAUGHTERHOUSE)
                                                        + GetBuildingCount(BLD_HUNTER) + GetBuildingCount(BLD_FISHERY)
                                                      > 4) ?
                                                  2 :
                                                  1;
                buildingsWanted[BLD_GOLDMINE] = (inventory.people[JOB_MINER] > 2) ? 1 : 0;
                resourcelimit = inventory.people[JOB_CHARBURNER] + inventory.goods[GD_SHOVEL];
                if(aijh.ggs.isEnabled(AddonId::CHARBURNER)
                   && (GetBuildingCount(BLD_COALMINE) < 1 && (GetBuildingCount(BLD_IRONMINE) + GetBuildingCount(BLD_GOLDMINE) > 0)))
                    buildingsWanted[BLD_CHARBURNER] = min<int>(1, resourcelimit);
            }
            if(GetBuildingCount(BLD_QUARRY) + 1 < buildingsWanted[BLD_QUARRY]
               && aijh.AmountInStorage(GD_STONES, 0) < 100) // no quarry and low stones -> try granitemines.
            {
                buildingsWanted[BLD_GRANITEMINE] =
                  (inventory.people[JOB_MINER] > 6 && buildingsWanted[BLD_QUARRY] > GetBuildingCount(BLD_QUARRY)) ?
                    buildingsWanted[BLD_QUARRY] - GetBuildingCount(BLD_QUARRY) :
                    1;
                if(buildingsWanted[BLD_GRANITEMINE] > (militaryBuildings.size() / 15) + 1) // limit granitemines to military / 15
                    buildingsWanted[BLD_GRANITEMINE] = (militaryBuildings.size() / 15) + 1;
            } else
                buildingsWanted[BLD_GRANITEMINE] = 0;
        }
    }
    if(aijh.ggs.GetMaxMilitaryRank() == 0)
    {
        buildingsWanted[BLD_GOLDMINE] = 0; // max rank is 0 = private / recruit ==> gold is useless!
    }
}

void AIConstruction::InitBuildingsWanted()
{
    buildingsWanted[BLD_FORESTER] = 1;
    buildingsWanted[BLD_SAWMILL] = 1;
    buildingsWanted[BLD_WOODCUTTER] = 12;
    buildingsWanted[BLD_GRANITEMINE] = 0;
    buildingsWanted[BLD_COALMINE] = 4;
    buildingsWanted[BLD_IRONMINE] = 2;
    buildingsWanted[BLD_GOLDMINE] = 1;
    buildingsWanted[BLD_CATAPULT] = 5;
    buildingsWanted[BLD_FISHERY] = 6;
    buildingsWanted[BLD_QUARRY] = 6;
    buildingsWanted[BLD_HUNTER] = 2;
    buildingsWanted[BLD_FARM] = aii.GetInventory().goods[GD_SCYTHE] + aii.GetInventory().people[JOB_FARMER];
    buildingsWanted[BLD_HARBORBUILDING] = 99;
    buildingsWanted[BLD_SHIPYARD] = aijh.GetCountofAIRelevantSeaIds() == 1 ? 1 : 99;
}

bool AIConstruction::BuildAlternativeRoad(const noFlag* flag, std::vector<Direction>& route)
{
    // LOG.write(("ai build alt road player %i at %i %i\n", flag->GetPlayer(), flag->GetPos());
    // Radius in dem nach würdigen Fahnen gesucht wird
    const unsigned short maxRoadLength = 10;
    // Faktor um den der Weg kürzer sein muss als ein vorhander Pfad, um gebaut zu werden
    const unsigned short lengthFactor = 5;

    // Flaggen in der Umgebung holen
    std::vector<const noFlag*> flags = FindFlags(flag->GetPos(), maxRoadLength);
    std::vector<Direction> mainroad = route;
    // targetflag for mainroad
    MapPoint t = flag->GetPos();
    for(unsigned i = 0; i < mainroad.size(); i++)
    {
        t = aii.GetNeighbour(t, mainroad[i]);
    }
    const noFlag* mainflag = aii.GetSpecObj<noFlag>(t);

    // Jede Flagge testen...
    for(unsigned i = 0; i < flags.size(); ++i)
    {
        const noFlag& curFlag = *flags[i];
        // When the current flag is the end of the main route, we skip it as crossing the main route is dissallowed by crossmainpath check a
        // bit below
        if(mainflag && curFlag.GetObjId() == mainflag->GetObjId())
            continue;

        route.clear();
        unsigned newLength;
        // the flag should not be at a military building!
        if(aii.IsMilitaryBuildingOnNode(aii.GetNeighbour(curFlag.GetPos(), Direction::NORTHWEST)))
            continue;

        if(!IsConnectedToRoadSystem(&curFlag))
            continue;

        // Gibts überhaupt einen Pfad zu dieser Flagge
        if(!aii.FindFreePathForNewRoad(flag->GetPos(), curFlag.GetPos(), &route, &newLength))
            continue;

        // Wenn ja, dann gucken ob unser momentaner Weg zu dieser Flagge vielleicht voll weit ist und sich eine Straße lohnt
        unsigned oldLength = 0;

        // Aktuelle Strecke zu der Flagge
        bool pathAvailable = aii.FindPathOnRoads(curFlag, *flag, &oldLength);
        if(!pathAvailable && mainflag)
        {
            pathAvailable = aii.FindPathOnRoads(curFlag, *mainflag, &oldLength);
            if(pathAvailable)
                oldLength += mainroad.size();
        }
        bool crossmainpath = false;
        unsigned size = 0;
        // more than 5 nonflaggable spaces on the route -> not really valid path
        unsigned temp = 0;
        t = flag->GetPos();
        for(unsigned j = 0; j < route.size(); ++j)
        {
            t = aii.GetNeighbour(t, route[j]);
            MapPoint t2 = flag->GetPos();
            // check if we cross the planned main road
            for(unsigned k = 0; k < mainroad.size(); ++k)
            {
                t2 = aii.GetNeighbour(t2, mainroad[k]);
                if(t2 == t)
                {
                    crossmainpath = true;
                    break;
                }
            }
            if(aii.GetBuildingQuality(t) < 1)
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
        if(!pathAvailable || newLength * lengthFactor < oldLength)
        {
            if(BuildRoad(flag, &curFlag, route))
            {
                constructionlocations.push_back(flag->GetPos());
                return true;
            }
        }
    }

    return false;
}

bool AIConstruction::OtherUsualBuildingInRadius(MapPoint pt, unsigned radius, BuildingType bt)
{
    for(std::list<nobUsual*>::const_iterator it = aii.GetBuildings(bt).begin(); it != aii.GetBuildings(bt).end(); ++it)
    {
        if(aii.CalcDistance((*it)->GetPos(), pt) < radius)
            return true;
    }
    for(std::list<noBuildingSite*>::const_iterator it = aii.GetBuildingSites().begin(); it != aii.GetBuildingSites().end(); ++it)
    {
        if((*it)->GetBuildingType() == bt)
        {
            if(aii.CalcDistance((*it)->GetPos(), pt) < radius)
                return true;
        }
    }
    return false;
}

bool AIConstruction::OtherStoreInRadius(MapPoint pt, unsigned radius)
{
    for(std::list<nobBaseWarehouse*>::const_iterator it = aii.GetStorehouses().begin(); it != aii.GetStorehouses().end(); ++it)
    {
        if(aii.CalcDistance((*it)->GetPos(), pt) < radius)
            return true;
    }
    for(std::list<noBuildingSite*>::const_iterator it = aii.GetBuildingSites().begin(); it != aii.GetBuildingSites().end(); ++it)
    {
        if(BuildingProperties::IsWareHouse((*it)->GetBuildingType()))
        {
            if(aii.CalcDistance((*it)->GetPos(), pt) < radius)
                return true;
        }
    }
    return false;
}

noFlag* AIConstruction::FindTargetStoreHouseFlag(const MapPoint pt) const
{
    unsigned minDistance = std::numeric_limits<unsigned>::max();
    nobBaseWarehouse* minTarget = NULL;
    bool found = false;
    for(std::list<nobBaseWarehouse*>::const_iterator it = aii.GetStorehouses().begin(); it != aii.GetStorehouses().end(); ++it)
    {
        unsigned dist = aii.GetDistance(pt, (*it)->GetPos());
        if(dist < minDistance)
        {
            minDistance = dist;
            minTarget = *it;
            found = true;
        }
    }
    if(!found)
        return NULL;
    else
    {
        return minTarget->GetFlag();
    }
}

} // namespace AIJH
