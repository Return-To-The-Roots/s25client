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
#include "BuildingPlanner.h"
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
#include "helpers/containerUtils.h"
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

AIConstruction::AIConstruction(AIPlayerJH& aijh)
    : aijh(aijh), aii(aijh.GetInterface()), bldPlanner(aijh.GetBldPlanner()), constructionorders(BUILDING_TYPES_COUNT)
{
}

AIConstruction::~AIConstruction()
{
    BOOST_FOREACH(AIJH::BuildJob* job, buildJobs)
        delete job;
    BOOST_FOREACH(AIJH::ConnectJob* job, connectJobs)
        delete job;
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
    curPos = aii.gwb.GetNeighbour(curPos, roadSeg.GetDir(isBwdDir, 0));
    // Start after getting first neighbor (min distance == 2)
    // and skip last 2 points (min distance and last is flag)
    for(unsigned i = 1; i + 2 < roadSeg.GetLength(); ++i)
    {
        curPos = aii.gwb.GetNeighbour(curPos, roadSeg.GetDir(isBwdDir, i));
        aii.SetFlag(curPos);
        constructionlocations.push_back(curPos);
    }
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
        if(aii.gwb.CalcDistance(pt, constructionlocations[i]) < 12)
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
        const GameWorldBase& world_;

        Point2FlagAI(const GameWorldBase& world) : world_(world) {}

        result_type operator()(const MapPoint pt, unsigned /*r*/) const { return world_.GetSpecObj<noFlag>(pt); }
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
    std::vector<const noFlag*> flags = aii.gwb.GetPointsInRadius<30>(pt, radius, Point2FlagAI(aii.gwb), IsValidFlag(aii.GetPlayerId()));
    // When the radius is at least half the size of the map then we may have duplicates that need to be removed
    if(radius >= std::min(aii.gwb.GetSize().x, aii.gwb.GetSize().y))
    {
        helpers::makeUnique(flags);
        // If at this pos is a flag, then it might be included due to wrapping.
        // This is wrong and needs to be removed
        const noFlag* flag = aii.gwb.GetSpecObj<noFlag>(pt);
        if(flag)
        {
            std::vector<const noFlag*>::iterator it = std::find(flags.begin(), flags.end(), flag);
            flags.erase(it);
        }
    }

    // TODO Performance Killer!
    /*
    if (radius > 10)
    {
        list<nobBaseMilitary*> military;
        gwb->LookForMilitaryBuildings(military, pt, 2);
        BOOST_FOREACH(const nobBaseMilitary* milBld, military)
        {
            unsigned distance = gwb->CalcDistance(milBld->GetPos(), pt);
            if (distance < radius && milBld->GetPlayer() == player->getPlayerId())
            {
                FindFlags(flags, milBld->GetPos(), 10, pt, radius, false);
            }
        }
    }
    */
    return flags;
}

bool AIConstruction::MilitaryBuildingWantsRoad(const nobMilitary& milbld)
{
    if(milbld.GetFrontierDistance() > 0) // close to front or harbor? connect!
        return true;
    if(!aijh.UpgradeBldPos.isValid()) // no upgrade bld on last update -> connect all that want to connect
        return true;
    if(aijh.UpgradeBldPos == milbld.GetPos()) // upgrade bld should have road already but just in case it doesnt -> get a road asap
        return true;
    // TODO: This probably does not do what is wanted...
    int bldIdx = helpers::indexOf(aii.GetMilitaryBuildings(), &milbld);
    if(bldIdx > static_cast<int>(aii.GetMilitaryBuildings().size() - aijh.GetNumPlannedConnectedInlandMilitaryBlds()))
        return true;
    return false;
}

bool AIConstruction::ConnectFlagToRoadSytem(const noFlag* flag, std::vector<Direction>& route, unsigned maxSearchRadius /*= 14*/)
{
    // TODO: die methode kann  ganz schön böse Laufzeiten bekommen... Optimieren?

    // Radius in dem nach würdigen Fahnen gesucht wird
    // const unsigned short maxSearchRadius = 10;

    // flag of a military building? -> check if we really want to connect this right now
    const MapPoint bldPos = aii.gwb.GetNeighbour(flag->GetPos(), Direction::NORTHWEST);
    if(const nobMilitary* milBld = aii.gwb.GetSpecObj<const nobMilitary>(bldPos))
    {
        if(!MilitaryBuildingWantsRoad(*milBld))
            return false;
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
    BOOST_FOREACH(const noFlag* curFlag, flags)
    {
        tmpRoute.clear();
        unsigned length;
        // the flag should not be at a military building!
        if(aii.gwb.IsMilitaryBuildingOnNode(aii.gwb.GetNeighbour(curFlag->GetPos(), Direction::NORTHWEST), true))
            continue;
        // Gibts überhaupt einen Pfad zu dieser Flagge
        if(!aii.FindFreePathForNewRoad(flag->GetPos(), curFlag->GetPos(), &tmpRoute, &length))
            continue;

        // Wenn ja, dann gucken ob dieser Pfad möglichst kurz zum "höheren" Ziel (allgemeines Lager im Moment) ist
        unsigned maxNonFlagPts = 0;
        // check for non-flag points on planned route: more than 2 nonflaggable spaces on the route -> not really valid path
        unsigned curNonFlagPts = 0;
        MapPoint tmpPos = flag->GetPos();
        for(unsigned j = 0; j < tmpRoute.size(); ++j)
        {
            tmpPos = aii.gwb.GetNeighbour(tmpPos, tmpRoute[j]);
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
        bool pathFound = curFlag == targetFlag || aii.FindPathOnRoads(*curFlag, *targetFlag, &distance);

        // Gewählte Fahne hat leider auch kein Anschluß an ein Lager, zu schade!
        if(!pathFound)
            continue;

        // Sind wir mit der Fahne schon verbunden? Einmal reicht!
        if(aii.FindPathOnRoads(*curFlag, *flag))
            continue;

        // Ansonsten haben wir einen Pfad!
        found = true;

        // Kürzer als der letzte? Nehmen! Existierende Strecke höher gewichten (2), damit möglichst kurze Baustrecken
        // bevorzugt werden bei ähnlich langen Wegmöglichkeiten
        if(2 * length + distance + 10 * maxNonFlagPts < shortestLength)
        {
            shortest = curFlag;
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
            t = aii.gwb.GetNeighbour(t, route[i + 1]);
            pStart = aii.gwb.GetNeighbour(pStart, route[i]);
            if(aii.gwb.IsRoadAvailable(false, t) && aii.IsOwnTerritory(t)) // can the alternative road be build?
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
            pStart = aii.gwb.GetNeighbour(pStart, route[i]);
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
    BOOST_FOREACH(BuildingType bld, BuildingProperties::militaryBldTypes)
    {
        if(aii.CanBuildBuildingtype(bld))
            return bld;
    }
    return BLD_NOTHING;
}

BuildingType AIConstruction::GetBiggestAllowedMilBuilding() const
{
    BOOST_REVERSE_FOREACH(BuildingType bld, BuildingProperties::militaryBldTypes)
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
    if(((rand() % 3) == 0 || inventory.people[JOB_PRIVATE] < 15)
       && (inventory.goods[GD_STONES] > 6 || bldPlanner.GetBuildingCount(BLD_QUARRY) > 0))
        bld = BLD_GUARDHOUSE;
    if(aijh.HarborPosClose(pt, 20) && rand() % 10 != 0 && aijh.ggs.getSelection(AddonId::SEA_ATTACK) != 2)
    {
        if(aii.CanBuildBuildingtype(BLD_WATCHTOWER))
            return BLD_WATCHTOWER;
        return GetBiggestAllowedMilBuilding();
    }
    if(biggestBld == BLD_WATCHTOWER || biggestBld == BLD_FORTRESS)
    {
        if(aijh.UpdateUpgradeBuilding() < 0 && bldPlanner.GetBuildingSitesCount(biggestBld) < 1
           && (inventory.goods[GD_STONES] > 20 || bldPlanner.GetBuildingCount(BLD_QUARRY) > 0) && rand() % 10 != 0)
        {
            return biggestBld;
        }
    }

    // avoid to build catapults in the beginning (no expansion)
    const unsigned militaryBuildingCount = bldPlanner.GetMilitaryBldCount() + bldPlanner.GetMilitaryBldSiteCount();

    uint8_t playerId = aii.GetPlayerId();
    sortedMilitaryBlds military = aii.gwb.LookForMilitaryBuildings(pt, 3);
    BOOST_FOREACH(const nobBaseMilitary* milBld, military)
    {
        unsigned distance = aii.gwb.CalcDistance(milBld->GetPos(), pt);

        // Prüfen ob Feind in der Nähe
        if(milBld->GetPlayer() != playerId && distance < 35)
        {
            int randmil = rand();
            bool buildCatapult = randmil % 8 == 0 && aii.CanBuildCatapult() && militaryBuildingCount > 5
                                 && inventory.goods[GD_STONES] > 50 + (4 * bldPlanner.GetBuildingCount(BLD_CATAPULT));
            // another catapult within "min" radius? ->dont build here!
            const unsigned min = 16;
            if(buildCatapult && aii.gwb.CalcDistance(pt, aii.GetStorehouses().front()->GetPos()) < min)
                buildCatapult = false;
            if(buildCatapult)
            {
                BOOST_FOREACH(const nobUsual* catapult, aii.GetBuildings(BLD_CATAPULT))
                {
                    if(aii.gwb.CalcDistance(pt, catapult->GetPos()) < min)
                    {
                        buildCatapult = false;
                        break;
                    }
                }
            }
            if(buildCatapult)
            {
                BOOST_FOREACH(const noBuildingSite* bldSite, aii.GetBuildingSites())
                {
                    if(bldSite->GetBuildingType() == bld && aii.gwb.CalcDistance(pt, bldSite->GetPos()) < min)
                    {
                        buildCatapult = false;
                        break;
                    }
                }
            }
            if(buildCatapult)
                bld = BLD_CATAPULT;
            else
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

bool AIConstruction::Wanted(BuildingType type) const
{
    if(!aii.CanBuildBuildingtype(type))
        return false;
    if(type == BLD_CATAPULT)
        return aii.CanBuildCatapult() && (aii.GetInventory().goods[GD_STONES] > 50 + (4 * bldPlanner.GetBuildingCount(BLD_CATAPULT)));
    if(BuildingProperties::IsMilitary(type) || type == BLD_STOREHOUSE)
    {
        if(!bldPlanner.WantMoreMilitaryBlds())
            return false;
        // todo: find a better way to determine that there is no risk in expanding than sawmill up and complete
        if(bldPlanner.GetBuildingCount(BLD_SAWMILL) > 0)
            return true;
        if(aii.GetInventory().goods[GD_BOARDS] > 30 && bldPlanner.GetBuildingSitesCount(BLD_SAWMILL) > 0)
            return true;
        return (bldPlanner.GetMilitaryBldCount() + bldPlanner.GetMilitaryBldSiteCount() > 0);
    }
    if(type == BLD_SAWMILL && bldPlanner.GetBuildingCount(BLD_SAWMILL) > 1)
    {
        if(aijh.AmountInStorage(GD_WOOD) < 15 * (bldPlanner.GetBuildingSitesCount(BLD_SAWMILL) + 1))
            return false;
    }
    return constructionorders[type] < bldPlanner.GetNumAdditionalBuildingsWanted(type);
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
        t = aii.gwb.GetNeighbour(t, mainroad[i]);
    }
    const noFlag* mainflag = aii.gwb.GetSpecObj<noFlag>(t);

    // Jede Flagge testen...
    for(unsigned i = 0; i < flags.size(); ++i)
    {
        const noFlag& curFlag = *flags[i];
        // When the current flag is the end of the main route, we skip it as crossing the main route is dissallowed by crossmainpath check a
        // bit below
        if(mainflag && &curFlag == mainflag)
            continue;

        route.clear();
        unsigned newLength;
        // the flag should not be at a military building!
        if(aii.gwb.IsMilitaryBuildingOnNode(aii.gwb.GetNeighbour(curFlag.GetPos(), Direction::NORTHWEST), true))
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
            t = aii.gwb.GetNeighbour(t, route[j]);
            MapPoint t2 = flag->GetPos();
            // check if we cross the planned main road
            for(unsigned k = 0; k < mainroad.size(); ++k)
            {
                t2 = aii.gwb.GetNeighbour(t2, mainroad[k]);
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
    BOOST_FOREACH(const nobUsual* bld, aii.GetBuildings(bt))
    {
        if(aii.gwb.CalcDistance(bld->GetPos(), pt) < radius)
            return true;
    }
    BOOST_FOREACH(const noBuildingSite* bldSite, aii.GetBuildingSites())
    {
        if(bldSite->GetBuildingType() == bt)
        {
            if(aii.gwb.CalcDistance(bldSite->GetPos(), pt) < radius)
                return true;
        }
    }
    return false;
}

bool AIConstruction::OtherStoreInRadius(MapPoint pt, unsigned radius)
{
    BOOST_FOREACH(const nobBaseWarehouse* wh, aii.GetStorehouses())
    {
        if(aii.gwb.CalcDistance(wh->GetPos(), pt) < radius)
            return true;
    }
    BOOST_FOREACH(const noBuildingSite* bldSite, aii.GetBuildingSites())
    {
        if(BuildingProperties::IsWareHouse(bldSite->GetBuildingType()))
        {
            if(aii.gwb.CalcDistance(bldSite->GetPos(), pt) < radius)
                return true;
        }
    }
    return false;
}

noFlag* AIConstruction::FindTargetStoreHouseFlag(const MapPoint pt) const
{
    unsigned minDistance = std::numeric_limits<unsigned>::max();
    const nobBaseWarehouse* minTarget = NULL;
    BOOST_FOREACH(const nobBaseWarehouse* wh, aii.GetStorehouses())
    {
        unsigned dist = aii.gwb.CalcDistance(pt, wh->GetPos());
        if(dist < minDistance)
        {
            minDistance = dist;
            minTarget = wh;
        }
    }
    if(!minTarget)
        return NULL;
    else
        return minTarget->GetFlag();
}

} // namespace AIJH
