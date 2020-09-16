// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "Jobs.h"
#include "ai/AIEvents.h"
#include "ai/AIInterface.h"
#include "ai/aijh/AIConstruction.h"
#include "ai/aijh/AIPlayerJH.h"
#include "ai/aijh/BuildingPlanner.h"
#include "ai/aijh/PositionSearch.h"
#include "buildings/noBuildingSite.h"
#include "world/GameWorldBase.h"
#include "nodeObjs/noFlag.h"
#include "gameData/BuildingConsts.h"
#include "gameData/BuildingProperties.h"
#include <boost/range/adaptor/reversed.hpp>

namespace AIJH {

Job::Job(AIPlayerJH& aijh) : aijh(aijh), state(JOB_WAITING) {}

void BuildJob::ExecuteJob()
{
    // are we allowed to plan construction work in the area in this nwf?
    if(!aijh.GetConstruction().CanStillConstructHere(around))
        return;

    if(state == JOB_WAITING)
        state = JOB_EXECUTING_START;

    switch(state)
    {
        case JOB_EXECUTING_START: TryToBuild(); break;

        case JOB_EXECUTING_ROAD1: BuildMainRoad(); break;

        case JOB_EXECUTING_ROAD2: TryToBuildSecondaryRoad(); break;
        case JOB_EXECUTING_ROAD2_2:
            // evtl noch prüfen ob auch dieser Straßenbau erfolgreich war?
            aijh.RecalcGround(target, route);
            state = JOB_FINISHED;
            break;

        default: RTTR_Assert(false); break;
    }

    // Fertig?
    if(state == JOB_FAILED || state == JOB_FINISHED)
        return;

    if(BuildingProperties::IsMilitary(type) && target.isValid()
       && aijh.GetWorld().IsMilitaryBuildingNearNode(target, aijh.GetPlayerId()))
    {
        state = JOB_FAILED;
#ifdef DEBUG_AI
        std::cout << "Player " << (unsigned)aijh.GetPlayerId() << ", Job failed: Military building too near for "
                  << BUILDING_NAMES[type] << " at " << target.x << "/" << target.y << "." << std::endl;
#endif
        return;
    }
}

void BuildJob::TryToBuild()
{
    AIConstruction& aiConstruction = aijh.GetConstruction();

    if(aijh.GetInterface().GetBuildingSites().size() > 40)
    {
        return;
    }

    if(!aiConstruction.Wanted(type))
    {
        state = JOB_FINISHED;
        return;
    }

    if(searchMode == SEARCHMODE_GLOBAL)
    {
        // TODO: tmp solution for testing: only woodcutter
        // hier machen für mehre gebäude
        /*erstmal wieder rausgenommen weil kaputt - todo: fix positionsearch
        if (type == BLD_WOODCUTTER)
        {
            PositionSearch *search = new PositionSearch(around, WOOD, 20, BLD_WOODCUTTER, true);
            SearchJob *job = new SearchJob(aijh, search);
            aijh.AddJob(job, true);
            status = JOB_FINISHED;
            return;
        }*/
        searchMode = SEARCHMODE_RADIUS;
    }

    MapPoint foundPos = MapPoint::Invalid();
    if(searchMode == SEARCHMODE_RADIUS)
    {
        foundPos = aijh.FindPositionForBuildingAround(type, around);
        if(BuildingProperties::IsMilitary(type))
        {
            if(foundPos.isValid())
            {
                // could we build a bigger military building? check if the location is surrounded by terrain that does
                // not allow normal buildings (probably important map part)
                AIInterface& aiInterface = aijh.GetInterface();
                RTTR_Assert(aiInterface.GetBuildingQuality(foundPos) == aijh.GetAINode(foundPos).bq);
                if(type != BLD_FORTRESS && aiInterface.GetBuildingQuality(foundPos) != BQ_MINE
                   && aiInterface.GetBuildingQuality(foundPos) > BUILDING_SIZE[type]
                   && aijh.BQsurroundcheck(foundPos, 6, true, 10) < 10)
                {
                    // more than 80% is unbuildable in range 7 -> upgrade
                    for(BuildingType bld : BuildingProperties::militaryBldTypes)
                    {
                        if(BUILDING_SIZE[bld] > BUILDING_SIZE[type] && aiInterface.CanBuildBuildingtype(bld))
                        {
                            type = bld;
                            break;
                        }
                    }
                }
            } else if(aijh.GetBldPlanner().IsExpansionRequired() && BUILDING_SIZE[type] != BQ_HUT)
            {
                // Downgrade to the next smaller building
                for(BuildingType bld : BuildingProperties::militaryBldTypes | boost::adaptors::reversed)
                {
                    if(BUILDING_SIZE[bld] < BUILDING_SIZE[type] && aijh.GetInterface().CanBuildBuildingtype(bld))
                    {
                        aijh.AddBuildJob(bld, around);
                        break;
                    }
                }
            }
        }
    } else if(searchMode == SEARCHMODE_NONE)
        foundPos = around;

    if(!foundPos.isValid())
    {
        state = JOB_FAILED;
#ifdef DEBUG_AI
        std::cout << "Player " << (unsigned)aijh.GetPlayerId() << ", Job failed: No Position found for "
                  << BUILDING_NAMES[type] << " around " << foundPos << "." << std::endl;
#endif
        return;
    }

#ifdef DEBUG_AI
    if(type == BLD_FARM)
        std::cout << " Player " << (unsigned)aijh.GetPlayerId() << " built farm at " << foundPos << " on value of "
                  << aijh.resourceMaps[PLANTSPACE][foundPos] << std::endl;
#endif

    if(!aijh.GetInterface().SetBuildingSite(foundPos, type))
    {
        state = JOB_FAILED;
        return;
    }
    target = foundPos;
    state = JOB_EXECUTING_ROAD1;
    aiConstruction.ConstructionOrdered(*this);
}

void BuildJob::BuildMainRoad()
{
    AIInterface& aiInterface = aijh.GetInterface();
    const auto* bld = aiInterface.gwb.GetSpecObj<noBuildingSite>(target);
    if(!bld)
    {
        // We don't have a building site where it should be. Maybe the BQ has changed due to another object next to it
        // If so, we update the BQ (TODO: Check if required, shouldn't be) and readd the build job
        // Note: If the BQ still allows construction it might as well be that the command was not executed yet
        BuildingQuality bq = aiInterface.GetBuildingQuality(target);
        if(!canUseBq(bq, BUILDING_SIZE[type]))
        {
            state = JOB_FAILED;
#ifdef DEBUG_AI
            std::cout << "Player " << (unsigned)aijh.GetPlayerId() << ", Job failed: BQ changed for "
                      << BUILDING_NAMES[type] << " at " << target.x << "/" << target.y << ". Retrying..." << std::endl;
#endif
            aijh.GetAINode(target).bq = bq;
            aijh.AddBuildJob(type, around);
        }
        return;
    }

    if(bld->GetBuildingType() != type)
    {
#ifdef DEBUG_AI
        std::cout << "Player " << (unsigned)aijh.GetPlayerId() << ", Job failed: Wrong Builingsite found for "
                  << BUILDING_NAMES[type] << " at " << target.x << "/" << target.y << "." << std::endl;
#endif
        state = JOB_FAILED;
        return;
    }
    const noFlag* houseFlag = bld->GetFlag();
    // Gucken noch nicht ans Wegnetz angeschlossen
    AIConstruction& aiConstruction = aijh.GetConstruction();
    if(!aiConstruction.IsConnectedToRoadSystem(houseFlag))
    {
        // Bau unmöglich?
        if(!aiConstruction.ConnectFlagToRoadSytem(houseFlag, route))
        {
            state = JOB_FAILED;
#ifdef DEBUG_AI
            std::cout << "Player " << (unsigned)aijh.GetPlayerId() << ", Job failed: Cannot connect "
                      << BUILDING_NAMES[type] << " at " << target.x << "/" << target.y << ". Retrying..." << std::endl;
#endif
            aijh.GetAINode(target).reachable = false;
            // We thought this had be reachable, but it is not (might be blocked by building site itself):
            // It has to be reachable in a check for 20x times, to avoid retrying it too often.
            aijh.GetAINode(target).failed_penalty = 20;
            aiInterface.DestroyBuilding(target);
            aiInterface.DestroyFlag(houseFlag->GetPos());
            aijh.AddBuildJob(type, around);
            return;
        }
    }

    // Wir sind angeschlossen, BQ für den eben gebauten Weg aktualisieren
    // aijh.RecalcBQAround(target);
    // aijh.RecalcGround(target, route);

    switch(type)
    {
        case BLD_FORESTER: aijh.AddBuildJob(BLD_WOODCUTTER, target); break;
        case BLD_CHARBURNER:
        case BLD_FARM: aijh.SetFarmedNodes(target, true); break;
        case BLD_MILL: aijh.AddBuildJob(BLD_BAKERY, target); break;
        case BLD_PIGFARM: aijh.AddBuildJob(BLD_SLAUGHTERHOUSE, target); break;
        case BLD_BAKERY:
        case BLD_SLAUGHTERHOUSE:
        case BLD_BREWERY: aijh.AddBuildJob(BLD_WELL, target); break;
        default: break;
    }

    // Just 4 Fun Gelehrten rufen
    if(BUILDING_SIZE[type] == BQ_MINE)
    {
        aiInterface.CallSpecialist(houseFlag->GetPos(), JOB_GEOLOGIST);
    }
    if(!BuildingProperties::IsMilitary(type)) // not a military building? -> build secondary road now
    {
        state = JOB_EXECUTING_ROAD2;
        return TryToBuildSecondaryRoad();
    } else // military buildings only get 1 road
    {
        state = JOB_FINISHED;
    }
}

void BuildJob::TryToBuildSecondaryRoad()
{
    const auto* houseFlag =
      aijh.GetWorld().GetSpecObj<noFlag>(aijh.GetWorld().GetNeighbour(target, Direction::SOUTHEAST));

    if(!houseFlag)
    {
        // Baustelle wurde wohl zerstört, oh schreck!
        state = JOB_FAILED;
#ifdef DEBUG_AI
        std::cout << "Player " << (unsigned)aijh.GetPlayerId() << ", Job failed: House flag is gone, "
                  << BUILDING_NAMES[type] << " at " << target.x << "/" << target.y << ". Retrying..." << std::endl;
#endif
        aijh.AddBuildJob(type, around);
        return;
    }

    if(aijh.GetConstruction().BuildAlternativeRoad(houseFlag, route))
    {
        state = JOB_EXECUTING_ROAD2_2;
    } else
        state = JOB_FINISHED;
}

EventJob::EventJob(AIPlayerJH& aijh, std::unique_ptr<AIEvent::Base> ev) : Job(aijh), ev(std::move(ev)) {}

EventJob::~EventJob() = default;

void EventJob::ExecuteJob() // for now it is assumed that all these will be finished or failed after execution (no wait
                            // or progress)
{
    switch(ev->GetType())
    {
        case AIEvent::BuildingConquered:
        {
            const auto& evb = *checkedCast<AIEvent::Building*>(ev.get());
            aijh.HandleNewMilitaryBuildingOccupied(evb.GetPos());
            state = JOB_FINISHED;
        }
        break;
        case AIEvent::BuildingLost:
        {
            const auto& evb = *checkedCast<AIEvent::Building*>(ev.get());
            aijh.HandleMilitaryBuilingLost(evb.GetPos());
            state = JOB_FINISHED;
        }
        break;
        case AIEvent::LostLand:
        {
            const auto& evb = *checkedCast<AIEvent::Building*>(ev.get());
            aijh.HandleLostLand(evb.GetPos());
            state = JOB_FINISHED;
        }
        break;
        case AIEvent::BuildingDestroyed:
        {
            // todo maybe do sth about it?
            const auto& evb = *checkedCast<AIEvent::Building*>(ev.get());
            aijh.HandleBuilingDestroyed(evb.GetPos(), evb.GetBuildingType());
            state = JOB_FINISHED;
        }
        break;
        case AIEvent::NoMoreResourcesReachable:
        {
            const auto& evb = *checkedCast<AIEvent::Building*>(ev.get());
            aijh.HandleNoMoreResourcesReachable(evb.GetPos(), evb.GetBuildingType());
            state = JOB_FINISHED;
        }
        break;
        case AIEvent::BorderChanged:
        {
            const auto& evb = *checkedCast<AIEvent::Building*>(ev.get());
            aijh.HandleBorderChanged(evb.GetPos());
            state = JOB_FINISHED;
        }
        break;
        case AIEvent::BuildingFinished:
        {
            const auto& evb = *checkedCast<AIEvent::Building*>(ev.get());
            aijh.HandleBuildingFinished(evb.GetPos(), evb.GetBuildingType());
            state = JOB_FINISHED;
        }
        break;
        case AIEvent::ExpeditionWaiting:
        {
            const auto& lvb = *checkedCast<AIEvent::Location*>(ev.get());
            aijh.HandleExpedition(lvb.GetPos());
            state = JOB_FINISHED;
        }
        break;
        case AIEvent::TreeChopped:
        {
            const auto& lvb = *checkedCast<AIEvent::Location*>(ev.get());
            aijh.HandleTreeChopped(lvb.GetPos());
            state = JOB_FINISHED;
        }
        break;
        case AIEvent::NewColonyFounded:
        {
            const auto& lvb = *checkedCast<AIEvent::Location*>(ev.get());
            aijh.HandleNewColonyFounded(lvb.GetPos());
            state = JOB_FINISHED;
        }
        break;
        case AIEvent::ShipBuilt:
        {
            const auto& lvb = *checkedCast<AIEvent::Location*>(ev.get());
            aijh.HandleShipBuilt(lvb.GetPos());
            state = JOB_FINISHED;
        }
        break;
        case AIEvent::RoadConstructionComplete:
        {
            const auto& dvb = *checkedCast<AIEvent::Direction*>(ev.get());
            aijh.HandleRoadConstructionComplete(dvb.GetPos(), dvb.GetDirection());
            state = JOB_FINISHED;
        }
        break;
        case AIEvent::RoadConstructionFailed:
        {
            const auto& dvb = *checkedCast<AIEvent::Direction*>(ev.get());
            aijh.HandleRoadConstructionFailed(dvb.GetPos(), dvb.GetDirection());
            state = JOB_FINISHED;
        }
        break;
        case AIEvent::LuaConstructionOrder:
        {
            const auto& evb = *checkedCast<AIEvent::Building*>(ev.get());
            aijh.ExecuteLuaConstructionOrder(evb.GetPos(), evb.GetBuildingType(), true);
            state = JOB_FINISHED;
        }
        break;
        default:
            // status = JOB_FAILED;
            break;
    }

    // temp only:
    state = JOB_FINISHED;
}

void ConnectJob::ExecuteJob()
{
#ifdef DEBUG_AI
    std::cout << "Player " << (unsigned)aijh.GetPlayerId() << ", ConnectJob executed..." << std::endl;
#endif

    // can the ai still construct here? else return and try again later
    AIConstruction& construction = aijh.GetConstruction();
    if(!construction.CanStillConstructHere(flagPos))
        return;

    const GameWorldBase& world = aijh.GetWorld();
    const auto* flag = world.GetSpecObj<noFlag>(flagPos);

    if(!flag)
    {
#ifdef DEBUG_AI
        std::cout << "Flag is gone." << std::endl;
#endif
        state = JOB_FAILED;
        return;
    }

    // is flag of a military building and has some road connection alraedy (not necessarily to a warehouse so this is
    // required to avoid multiple connections on mil buildings)
    if(world.IsMilitaryBuildingOnNode(world.GetNeighbour(flag->GetPos(), Direction::NORTHWEST), true))
    {
        for(unsigned dir = 2; dir < 7; dir++)
        {
            if(flag->GetRoute(Direction(dir)))
            {
                state = JOB_FINISHED;
                return;
            }
        }
    }

    // already connected?
    if(!construction.IsConnectedToRoadSystem(flag))
    {
#ifdef DEBUG_AI
        std::cout << "Flag is not connected..." << std::endl;
#endif
        // building road possible?
        if(!construction.ConnectFlagToRoadSytem(flag, route, 24))
        {
#ifdef DEBUG_AI
            std::cout << "Flag is not connectable." << std::endl;
#endif
            state = JOB_FAILED;
        } else
        {
#ifdef DEBUG_AI
            std::cout << "Connecting flag..." << std::endl;
#endif
        }
    } else
    {
#ifdef DEBUG_AI
        std::cout << "Flag is connected." << std::endl;
#endif
        aijh.RecalcGround(flagPos, route);
        state = JOB_FINISHED;
    }
}

void SearchJob::ExecuteJob()
{
    state = JOB_FAILED;
    PositionSearchState searchState = search->execute(aijh);

    if(searchState == SEARCH_IN_PROGRESS)
        state = JOB_WAITING;
    else if(searchState == SEARCH_FAILED)
        state = JOB_FAILED;
    else
    {
        state = JOB_FINISHED;
        aijh.AddBuildJob(search->GetBld(), search->GetResultPt(), true, false);
    }
}

SearchJob::~SearchJob()
{
    delete search;
}

} // namespace AIJH
