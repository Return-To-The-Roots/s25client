// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
#include "s25util/warningSuppression.h"
#include <boost/range/adaptor/reversed.hpp>
#include <algorithm>
#include <array>
#include <cstdlib>
#include <limits>
#include <list>

namespace AIJH {

AIConstruction::AIConstruction(AIPlayerJH& aijh)
    : aijh(aijh), aii(aijh.GetInterface()), bldPlanner(aijh.GetBldPlanner())
{
    std::fill(constructionorders.begin(), constructionorders.end(), 0u);
}

AIConstruction::~AIConstruction() = default;

void AIConstruction::AddBuildJob(std::unique_ptr<BuildJob> job, bool front)
{
    if(job->GetType() == BuildingType::Shipyard && aijh.IsInvalidShipyardPosition(job->GetAround()))
        return;
    if(BuildingProperties::IsMilitary(
         job->GetType())) // non military buildings can only be added once to the contruction que for every location
    {
        if(front)
            buildJobs.push_front(std::move(job));
        else
            buildJobs.push_back(std::move(job));
    } else // check if the buildjob is already in list and if so dont add it again
    {
        bool alreadyinlist = false;
        for(auto& buildJob : buildJobs)
        {
            if(buildJob->GetType() == job->GetType() && buildJob->GetAround() == job->GetAround())
            {
                alreadyinlist = true;
                break;
            }
        }
        if(!alreadyinlist)
        {
            if(front)
                buildJobs.push_front(std::move(job));
            else
                buildJobs.push_back(std::move(job));
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
    unsigned initconjobs = std::min<unsigned>(connectJobs.size(), 5);
    unsigned initbuildjobs = std::min<unsigned>(buildJobs.size(), 5);
    for(; i < limit && !connectJobs.empty() && i < initconjobs;
        i++) // go through list, until limit is reached or list empty or when every entry has been checked
    {
        auto job = std::move(connectJobs.front());
        connectJobs.pop_front();
        job->ExecuteJob();
        if(job->GetState() != JobState::Finished
           && job->GetState() != JobState::Failed) // couldnt do job? -> move to back of list
        {
            connectJobs.push_back(std::move(job));
        }
    }
    for(; i < limit && !buildJobs.empty() && i < (initconjobs + initbuildjobs); i++)
    {
        auto job = GetBuildJob();
        job->ExecuteJob();
        if(job->GetState() != JobState::Finished
           && job->GetState() != JobState::Failed) // couldnt do job? -> move to back of list
        {
            buildJobs.push_back(std::move(job));
        }
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

std::unique_ptr<BuildJob> AIConstruction::GetBuildJob()
{
    if(buildJobs.empty())
        return nullptr;

    std::unique_ptr<BuildJob> job = std::move(buildJobs.front());
    buildJobs.pop_front();
    return job;
}

void AIConstruction::AddConnectFlagJob(const noFlag* flag)
{
    // already in list?
    for(auto& connectJob : connectJobs)
    {
        if(connectJob->getFlag() == flag->GetPos())
            return;
    }
    // add to list
    connectJobs.push_back(std::make_unique<ConnectJob>(aijh, flag->GetPos()));
}

bool AIConstruction::CanStillConstructHere(const MapPoint pt) const
{
    for(const auto& constructionlocation : constructionlocations)
    {
        if(aii.gwb.CalcDistance(pt, constructionlocation) < 12)
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
        const GameWorldBase& world_;
        Point2FlagAI(const GameWorldBase& world) : world_(world) {}
        const noFlag* operator()(const MapPoint pt, unsigned /*r*/) const { return world_.GetSpecObj<noFlag>(pt); }
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
    std::vector<const noFlag*> flags =
      aii.gwb.GetPointsInRadius<30>(pt, radius, Point2FlagAI(aii.gwb), IsValidFlag(aii.GetPlayerId()));
    // When the radius is at least half the size of the map then we may have duplicates that need to be removed
    if(radius >= std::min(aii.gwb.GetSize().x, aii.gwb.GetSize().y))
    {
        helpers::makeUniqueStable(flags);
        // If at this pos is a flag, then it might be included due to wrapping.
        // This is wrong and needs to be removed
        const auto* flag = aii.gwb.GetSpecObj<noFlag>(pt);
        if(flag)
        {
            auto it = std::find(flags.begin(), flags.end(), flag);
            if(it != flags.end())
                flags.erase(it);
        }
    }

    // TODO Performance Killer!
    /*
    if (radius > 10)
    {
        list<nobBaseMilitary*> military;
        gwb->LookForMilitaryBuildings(military, pt, 2);
        for(const nobBaseMilitary* milBld: military)
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
    if(milbld.GetFrontierDistance() != FrontierDistance::Far) // close to front or harbor? connect!
        return true;
    if(!aijh.UpgradeBldPos.isValid()) // no upgrade bld on last update -> connect all that want to connect
        return true;
    if(aijh.UpgradeBldPos
       == milbld.GetPos()) // upgrade bld should have road already but just in case it doesnt -> get a road asap
        return true;
    // TODO: This probably does not do what is wanted...
    const auto bldIdx = helpers::indexOf(aii.GetMilitaryBuildings(), &milbld);
    return bldIdx
           > static_cast<int>(aii.GetMilitaryBuildings().size() - aijh.GetNumPlannedConnectedInlandMilitaryBlds());
}

bool AIConstruction::ConnectFlagToRoadSytem(const noFlag* flag, std::vector<Direction>& route,
                                            unsigned maxSearchRadius /*= 14*/)
{
    // TODO: die methode kann  ganz schön böse Laufzeiten bekommen... Optimieren?

    // Radius in dem nach würdigen Fahnen gesucht wird
    // const unsigned short maxSearchRadius = 10;

    // flag of a military building? -> check if we really want to connect this right now
    const MapPoint bldPos = aii.gwb.GetNeighbour(flag->GetPos(), Direction::NorthWest);
    if(const auto* milBld = aii.gwb.GetSpecObj<const nobMilitary>(bldPos))
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

    const noFlag* shortest = nullptr;
    unsigned shortestLength = 99999;
    std::vector<Direction> tmpRoute;

    // Jede Flagge testen...
    for(const noFlag* curFlag : flags)
    {
        tmpRoute.clear();
        unsigned length;
        // the flag should not be at a military building!
        if(aii.gwb.IsMilitaryBuildingOnNode(aii.gwb.GetNeighbour(curFlag->GetPos(), Direction::NorthWest), true))
            continue;
        // Gibts überhaupt einen Pfad zu dieser Flagge
        if(!aii.FindFreePathForNewRoad(flag->GetPos(), curFlag->GetPos(), &tmpRoute, &length))
            continue;

        // Wenn ja, dann gucken ob dieser Pfad möglichst kurz zum "höheren" Ziel (allgemeines Lager im Moment) ist
        unsigned maxNonFlagPts = 0;
        // check for non-flag points on planned route: more than 2 nonflaggable spaces on the route -> not really valid
        // path
        unsigned curNonFlagPts = 0;
        MapPoint tmpPos = flag->GetPos();
        for(auto j : tmpRoute)
        {
            tmpPos = aii.gwb.GetNeighbour(tmpPos, j);
            RTTR_Assert(aii.GetBuildingQuality(tmpPos) == aijh.GetAINode(tmpPos).bq);
            if(aii.GetBuildingQuality(tmpPos) == BuildingQuality::Nothing)
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

        // Find path from current flag to target. If the current flag IS the target then we have already a path with
        // distance=0
        unsigned distance = 0;
        bool pathFound = curFlag == targetFlag || aii.FindPathOnRoads(*curFlag, *targetFlag, &distance);

        // Gewählte Fahne hat leider auch kein Anschluß an ein Lager, zu schade!
        if(!pathFound)
            continue;

        // Sind wir mit der Fahne schon verbunden? Einmal reicht!
        if(aii.FindPathOnRoads(*curFlag, *flag))
            continue;

        // Kürzer als der letzte? Nehmen! Existierende Strecke höher gewichten (2), damit möglichst kurze Baustrecken
        // bevorzugt werden bei ähnlich langen Wegmöglichkeiten
        if(2 * length + distance + 10 * maxNonFlagPts < shortestLength)
        {
            shortest = curFlag;
            shortestLength = 2 * length + distance + 10 * maxNonFlagPts;
            route = tmpRoute;
        }
    }

    if(shortest)
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

bool AIConstruction::MinorRoadImprovements(const noRoadNode* start, const noRoadNode* target,
                                           std::vector<Direction>& route)
{
    return BuildRoad(start, target, route);
    // TODO: Enable later after checking for performance and correctness
    RTTR_IGNORE_UNREACHABLE_CODE
    MapPoint pStart = start->GetPos(); //-V779
    for(unsigned i = 0; i + 1 < route.size(); i++)
    {
        // switching current and next route element will result in the same position after building both
        if((route[i] + 1u == route[i + 1]) || (route[i] - 1u == route[i + 1]))
        {
            MapPoint t(pStart);
            t = aii.gwb.GetNeighbour(t, route[i + 1]);
            pStart = aii.gwb.GetNeighbour(pStart, route[i]);
            // can the alternative road be build?
            if(aii.gwb.IsRoadAvailable(false, t) && aii.IsOwnTerritory(t))
            {
                // does the alternative road block a lower buildingquality point than the normal planned route?
                if(aii.CalcBQSumDifference(pStart, t))
                {
                    // LOG.write(("AIConstruction::road improvements p%i from %i,%i moved node %i,%i to %i,%i i:%i,
                    // i+1:%i\n",playerID, start->GetX(), start->GetY(), ptx, pt.y, t.x, t.y,route[i],route[i+1]);
                    pStart = t; // we move the alternative path so move x&y and switch the route entries
                    std::swap(route[i], route[i + 1]);
                }
            }
        } else
            pStart = aii.gwb.GetNeighbour(pStart, route[i]);
    }
    return BuildRoad(start, target, route);
    RTTR_POP_DIAGNOSTIC
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
        aii.BuildRoad(start->GetPos(), false, route);
        // set flags along the road just after contruction - todo: handle failed road construction by removing the
        // useless flags!
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

helpers::OptionalEnum<BuildingType> AIConstruction::GetSmallestAllowedMilBuilding() const
{
    for(BuildingType bld : BuildingProperties::militaryBldTypes)
    {
        if(aii.CanBuildBuildingtype(bld))
            return bld;
    }
    return boost::none;
}

helpers::OptionalEnum<BuildingType> AIConstruction::GetBiggestAllowedMilBuilding() const
{
    for(BuildingType bld : boost::adaptors::reverse(BuildingProperties::militaryBldTypes))
    {
        if(aii.CanBuildBuildingtype(bld))
            return bld;
    }
    return boost::none;
}

helpers::OptionalEnum<BuildingType> AIConstruction::ChooseMilitaryBuilding(const MapPoint pt)
{
    // default : 2 barracks for each guardhouse
    // stones & low soldiers -> only guardhouse (no stones -> only barracks)
    // harbor nearby that could be used to attack/get attacked -> tower
    // enemy nearby? -> tower or fortress
    // to do: important location or an area with a very low amount of buildspace? -> try large buildings
    // buildings with requirement > small have a chance to be replaced with small buildings to avoid getting stuck if
    // there are no places for medium/large buildings
    auto bld = GetSmallestAllowedMilBuilding();
    // If we are not allowed to build a military building, return early
    if(!bld)
        return boost::none;

    const BuildingType biggestBld = GetBiggestAllowedMilBuilding().value();

    const Inventory& inventory = aii.GetInventory();
    if(((rand() % 3) == 0 || inventory.people[Job::Private] < 15)
       && (inventory.goods[GoodType::Stones] > 6 || bldPlanner.GetNumBuildings(BuildingType::Quarry) > 0))
        bld = BuildingType::Guardhouse;
    if(aijh.getAIInterface().isHarborPosClose(pt, 19) && rand() % 10 != 0 && aijh.ggs.isEnabled(AddonId::SEA_ATTACK))
    {
        if(aii.CanBuildBuildingtype(BuildingType::Watchtower))
            return BuildingType::Watchtower;
        return GetBiggestAllowedMilBuilding();
    }
    if(biggestBld == BuildingType::Watchtower || biggestBld == BuildingType::Fortress)
    {
        if(aijh.UpdateUpgradeBuilding() < 0 && bldPlanner.GetNumBuildingSites(biggestBld) < 1
           && (inventory.goods[GoodType::Stones] > 20 || bldPlanner.GetNumBuildings(BuildingType::Quarry) > 0)
           && rand() % 10 != 0)
        {
            return biggestBld;
        }
    }

    uint8_t playerId = aii.GetPlayerId();
    sortedMilitaryBlds military = aii.gwb.LookForMilitaryBuildings(pt, 3);
    for(const nobBaseMilitary* milBld : military)
    {
        unsigned distance = aii.gwb.CalcDistance(milBld->GetPos(), pt);

        // Prüfen ob Feind in der Nähe
        if(milBld->GetPlayer() != playerId && distance < 35)
        {
            int randmil = rand();
            bool buildCatapult = randmil % 8 == 0 && aii.CanBuildCatapult()
                                 && bldPlanner.GetNumAdditionalBuildingsWanted(BuildingType::Catapult) > 0;
            // another catapult within "min" radius? ->dont build here!
            const unsigned min = 16;
            if(buildCatapult && aii.gwb.CalcDistance(pt, aii.GetStorehouses().front()->GetPos()) < min)
                buildCatapult = false;
            if(buildCatapult)
            {
                for(const nobUsual* catapult : aii.GetBuildings(BuildingType::Catapult))
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
                for(const noBuildingSite* bldSite : aii.GetBuildingSites())
                {
                    if(bldSite->GetBuildingType() == bld && aii.gwb.CalcDistance(pt, bldSite->GetPos()) < min)
                    {
                        buildCatapult = false;
                        break;
                    }
                }
            }
            if(buildCatapult)
                bld = BuildingType::Catapult;
            else
            {
                if(randmil % 2 == 0 && aii.CanBuildBuildingtype(BuildingType::Watchtower))
                    bld = BuildingType::Watchtower;
                else
                    bld = biggestBld;
            }
            // slim chance for a guardhouse instead of tower or fortress so we can expand towards an enemy even if there
            // are no big building spots in that direction
            if(randmil % 10 == 0)
            {
                if(aii.CanBuildBuildingtype(BuildingType::Guardhouse))
                    bld = BuildingType::Guardhouse;
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
    if(type == BuildingType::Catapult && !aii.CanBuildCatapult())
        return false;
    if(BuildingProperties::IsMilitary(type) || type == BuildingType::Storehouse)
        return bldPlanner.WantMoreMilitaryBlds(aijh);
    if(type == BuildingType::Sawmill && bldPlanner.GetNumBuildings(BuildingType::Sawmill) > 1)
    {
        if(aijh.AmountInStorage(GoodType::Wood) < 15 * (bldPlanner.GetNumBuildingSites(BuildingType::Sawmill) + 1))
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
    for(auto i : mainroad)
    {
        t = aii.gwb.GetNeighbour(t, i);
    }
    const auto* mainflag = aii.gwb.GetSpecObj<noFlag>(t);

    // Jede Flagge testen...
    for(const noFlag* i : flags)
    {
        const noFlag& curFlag = *i;
        // When the current flag is the end of the main route, we skip it as crossing the main route is dissallowed by
        // crossmainpath check a bit below
        if(mainflag && &curFlag == mainflag)
            continue;

        route.clear();
        unsigned newLength;
        // the flag should not be at a military building!
        if(aii.gwb.IsMilitaryBuildingOnNode(aii.gwb.GetNeighbour(curFlag.GetPos(), Direction::NorthWest), true))
            continue;

        if(!IsConnectedToRoadSystem(&curFlag))
            continue;

        // Gibts überhaupt einen Pfad zu dieser Flagge
        if(!aii.FindFreePathForNewRoad(flag->GetPos(), curFlag.GetPos(), &route, &newLength))
            continue;

        // Wenn ja, dann gucken ob unser momentaner Weg zu dieser Flagge vielleicht voll weit ist und sich eine Straße
        // lohnt
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
        for(auto j : route)
        {
            t = aii.gwb.GetNeighbour(t, j);
            MapPoint t2 = flag->GetPos();
            // check if we cross the planned main road
            for(auto k : mainroad)
            {
                t2 = aii.gwb.GetNeighbour(t2, k);
                if(t2 == t)
                {
                    crossmainpath = true;
                    break;
                }
            }
            RTTR_Assert(aii.GetBuildingQuality(t) == aijh.GetAINode(t).bq);
            if(aii.GetBuildingQuality(t) == BuildingQuality::Nothing)
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
    for(const nobUsual* bld : aii.GetBuildings(bt))
    {
        if(aii.gwb.CalcDistance(bld->GetPos(), pt) < radius)
            return true;
    }
    for(const noBuildingSite* bldSite : aii.GetBuildingSites())
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
    for(const nobBaseWarehouse* wh : aii.GetStorehouses())
    {
        if(aii.gwb.CalcDistance(wh->GetPos(), pt) < radius)
            return true;
    }
    for(const noBuildingSite* bldSite : aii.GetBuildingSites())
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
    const nobBaseWarehouse* minTarget = nullptr;
    for(const nobBaseWarehouse* wh : aii.GetStorehouses())
    {
        unsigned dist = aii.gwb.CalcDistance(pt, wh->GetPos());
        if(dist < minDistance)
        {
            minDistance = dist;
            minTarget = wh;
        }
    }
    if(!minTarget)
        return nullptr;
    else
        return minTarget->GetFlag();
}

} // namespace AIJH
