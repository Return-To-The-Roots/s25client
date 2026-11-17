// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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

AIJob::AIJob(AIPlayerJH& aijh) : aijh(aijh), state(JobState::Waiting) {}

void BuildJob::executeJob()
{
    // are we allowed to plan construction work in the area in this nwf?
    if(!aijh.getConstruction().canStillConstructHere(around))
        return;

    if(state == JobState::Waiting)
        state = JobState::Start;

    switch(state)
    {
        case JobState::Start: tryToBuild(); break;

        case JobState::ExecutingRoad1: buildMainRoad(); break;

        case JobState::ExecutingRoad2: tryToBuildSecondaryRoad(); break;
        case JobState::ExecutingRoad2_2:
            // evtl noch prüfen ob auch dieser Straßenbau erfolgreich war?
            aijh.recalcGround(target, route);
            state = JobState::Finished;
            break;

        default: RTTR_Assert(false); break;
    }

    // Fertig?
    if(state == JobState::Failed || state == JobState::Finished)
        return;

    if(BuildingProperties::isMilitary(type) && target.isValid()
       && aijh.getWorld().isMilitaryBuildingNearNode(target, aijh.getPlayerId()))
    {
        state = JobState::Failed;
#ifdef DEBUG_AI
        std::cout << "Player " << (unsigned)aijh.getPlayerId() << ", Job failed: Military building too near for "
                  << BUILDING_NAMES[type] << " at " << target.x << "/" << target.y << "." << std::endl;
#endif
        return;
    }
}

void BuildJob::tryToBuild()
{
    AIConstruction& aiConstruction = aijh.getConstruction();

    if(aijh.getInterface().getBuildingSites().size() > 40)
    {
        return;
    }

    if(!aiConstruction.wanted(type))
    {
        state = JobState::Finished;
        return;
    }

    if(searchMode == SearchMode::Global)
    {
        // TODO: tmp solution for testing: only woodcutter
        // hier machen für mehre gebäude
        /*erstmal wieder rausgenommen weil kaputt - todo: fix positionsearch
        if (type == BuildingType::Woodcutter)
        {
            PositionSearch *search = new PositionSearch(around, WOOD, 20, BuildingType::Woodcutter, true);
            SearchJob *job = new SearchJob(aijh, search);
            aijh.AddJob(job, true);
            status = JobState::Finished;
            return;
        }*/
        searchMode = SearchMode::Radius;
    }

    MapPoint foundPos = MapPoint::invalid();
    if(searchMode == SearchMode::Radius)
    {
        foundPos = aijh.findPositionForBuildingAround(type, around);
        if(BuildingProperties::isMilitary(type))
        {
            if(foundPos.isValid())
            {
                // could we build a bigger military building? check if the location is surrounded by terrain that does
                // not allow normal buildings (probably important map part)
                AIInterface& aiInterface = aijh.getInterface();
                RTTR_Assert(aiInterface.getBuildingQuality(foundPos) == aijh.getAINode(foundPos).bq);
                if(type != BuildingType::Fortress && aiInterface.getBuildingQuality(foundPos) != BuildingQuality::Mine
                   && aiInterface.getBuildingQuality(foundPos) > BUILDING_SIZE[type]
                   && aijh.bQsurroundcheck(foundPos, 6, true, 10) < 10)
                {
                    // more than 80% is unbuildable in range 7 -> upgrade
                    for(BuildingType bld : BuildingProperties::militaryBldTypes)
                    {
                        if(BUILDING_SIZE[bld] > BUILDING_SIZE[type] && aiInterface.canBuildBuildingtype(bld))
                        {
                            type = bld;
                            break;
                        }
                    }
                }
            } else if(aijh.getBldPlanner().isExpansionRequired() && BUILDING_SIZE[type] != BuildingQuality::Hut)
            {
                // Downgrade to the next smaller building
                for(BuildingType bld : BuildingProperties::militaryBldTypes | boost::adaptors::reversed)
                {
                    if(BUILDING_SIZE[bld] < BUILDING_SIZE[type] && aijh.getInterface().canBuildBuildingtype(bld))
                    {
                        aijh.addBuildJob(bld, around);
                        break;
                    }
                }
            }
        }
    } else if(searchMode == SearchMode::None)
        foundPos = around;

    if(!foundPos.isValid())
    {
        state = JobState::Failed;
#ifdef DEBUG_AI
        std::cout << "Player " << (unsigned)aijh.getPlayerId() << ", Job failed: No Position found for "
                  << BUILDING_NAMES[type] << " around " << foundPos << "." << std::endl;
#endif
        return;
    }

#ifdef DEBUG_AI
    if(type == BuildingType::Farm)
        std::cout << " Player " << (unsigned)aijh.getPlayerId() << " built farm at " << foundPos << " on value of "
                  << aijh.resourceMaps[PLANTSPACE][foundPos] << std::endl;
#endif

    if(!aijh.getInterface().SetBuildingSite(foundPos, type))
    {
        state = JobState::Failed;
        return;
    }
    target = foundPos;
    state = JobState::ExecutingRoad1;
    aiConstruction.constructionOrdered(*this);
}

void BuildJob::buildMainRoad()
{
    AIInterface& aiInterface = aijh.getInterface();
    const auto* bld = aiInterface.gwb.getSpecObj<noBuildingSite>(target);
    if(!bld)
    {
        // We don't have a building site where it should be. Maybe the BQ has changed due to another object next to it
        // If so, we update the BQ (TODO: Check if required, shouldn't be) and readd the build job
        // Note: If the BQ still allows construction it might as well be that the command was not executed yet
        BuildingQuality bq = aiInterface.getBuildingQuality(target);
        if(!canUseBq(bq, BUILDING_SIZE[type]))
        {
            state = JobState::Failed;
#ifdef DEBUG_AI
            std::cout << "Player " << (unsigned)aijh.getPlayerId() << ", Job failed: BQ changed for "
                      << BUILDING_NAMES[type] << " at " << target.x << "/" << target.y << ". Retrying..." << std::endl;
#endif
            aijh.getAINode(target).bq = bq;
            aijh.addBuildJob(type, around);
        }
        return;
    }

    if(bld->getBuildingType() != type)
    {
#ifdef DEBUG_AI
        std::cout << "Player " << (unsigned)aijh.getPlayerId() << ", Job failed: Wrong Builingsite found for "
                  << BUILDING_NAMES[type] << " at " << target.x << "/" << target.y << "." << std::endl;
#endif
        state = JobState::Failed;
        return;
    }
    const noFlag* houseFlag = bld->getFlag();
    // Gucken noch nicht ans Wegnetz angeschlossen
    AIConstruction& aiConstruction = aijh.getConstruction();
    if(!aiConstruction.isConnectedToRoadSystem(houseFlag))
    {
        // Bau unmöglich?
        if(!aiConstruction.connectFlagToRoadSytem(houseFlag, route))
        {
            state = JobState::Failed;
#ifdef DEBUG_AI
            std::cout << "Player " << (unsigned)aijh.getPlayerId() << ", Job failed: Cannot connect "
                      << BUILDING_NAMES[type] << " at " << target.x << "/" << target.y << ". Retrying..." << std::endl;
#endif
            aijh.getAINode(target).reachable = false;
            // We thought this had be reachable, but it is not (might be blocked by building site itself):
            // It has to be reachable in a check for 20x times, to avoid retrying it too often.
            aijh.getAINode(target).failed_penalty = 20;
            aiInterface.DestroyBuilding(target);
            aiInterface.DestroyFlag(houseFlag->getPos());
            aijh.addBuildJob(type, around);
            return;
        }
    }

    // Wir sind angeschlossen, BQ für den eben gebauten Weg aktualisieren
    // aijh.RecalcBQAround(target);
    // aijh.recalcGround(target, route);

    switch(type)
    {
        case BuildingType::Forester: aijh.addBuildJob(BuildingType::Woodcutter, target); break;
        case BuildingType::Charburner:
        case BuildingType::Farm: aijh.setFarmedNodes(target, true); break;
        case BuildingType::Mill: aijh.addBuildJob(BuildingType::Bakery, target); break;
        case BuildingType::PigFarm: aijh.addBuildJob(BuildingType::Slaughterhouse, target); break;
        case BuildingType::Bakery:
        case BuildingType::Slaughterhouse:
        case BuildingType::Brewery: aijh.addBuildJob(BuildingType::Well, target); break;
        default: break;
    }

    // Just 4 Fun Gelehrten rufen
    if(BUILDING_SIZE[type] == BuildingQuality::Mine)
    {
        aiInterface.CallSpecialist(houseFlag->getPos(), Job::Geologist);
    }
    if(!BuildingProperties::isMilitary(type)) // not a military building? -> build secondary road now
    {
        state = JobState::ExecutingRoad2;
        return tryToBuildSecondaryRoad();
    } else // military buildings only get 1 road
    {
        state = JobState::Finished;
    }
}

void BuildJob::tryToBuildSecondaryRoad()
{
    const auto* houseFlag =
      aijh.getWorld().getSpecObj<noFlag>(aijh.getWorld().getNeighbour(target, Direction::SouthEast));

    if(!houseFlag)
    {
        // Baustelle wurde wohl zerstört, oh schreck!
        state = JobState::Failed;
#ifdef DEBUG_AI
        std::cout << "Player " << (unsigned)aijh.getPlayerId() << ", Job failed: House flag is gone, "
                  << BUILDING_NAMES[type] << " at " << target.x << "/" << target.y << ". Retrying..." << std::endl;
#endif
        aijh.addBuildJob(type, around);
        return;
    }

    if(aijh.getConstruction().buildAlternativeRoad(houseFlag, route))
    {
        state = JobState::ExecutingRoad2_2;
    } else
        state = JobState::Finished;
}

EventJob::EventJob(AIPlayerJH& aijh, std::unique_ptr<AIEvent::Base> ev) : AIJob(aijh), ev(std::move(ev)) {}

EventJob::~EventJob() = default;

void EventJob::executeJob()
{
    // for now it is assumed that all these will be finished or failed after execution (no wait or progress)
    using AIEvent::EventType;
    switch(ev->getType())
    {
        case EventType::BuildingConquered:
        {
            const auto& evb = *checkedCast<AIEvent::Building*>(ev.get());
            aijh.handleNewMilitaryBuildingOccupied(evb.getPos());
            state = JobState::Finished;
        }
        break;
        case EventType::BuildingLost:
        {
            const auto& evb = *checkedCast<AIEvent::Building*>(ev.get());
            aijh.handleMilitaryBuilingLost(evb.getPos());
            state = JobState::Finished;
        }
        break;
        case EventType::LostLand:
        {
            const auto& evb = *checkedCast<AIEvent::Building*>(ev.get());
            aijh.handleLostLand(evb.getPos());
            state = JobState::Finished;
        }
        break;
        case EventType::BuildingDestroyed:
        {
            // todo maybe do sth about it?
            const auto& evb = *checkedCast<AIEvent::Building*>(ev.get());
            aijh.handleBuilingDestroyed(evb.getPos(), evb.getBuildingType());
            state = JobState::Finished;
        }
        break;
        case EventType::NoMoreResourcesReachable:
        {
            const auto& evb = *checkedCast<AIEvent::Building*>(ev.get());
            aijh.handleNoMoreResourcesReachable(evb.getPos(), evb.getBuildingType());
            state = JobState::Finished;
        }
        break;
        case EventType::BorderChanged:
        {
            const auto& evb = *checkedCast<AIEvent::Building*>(ev.get());
            aijh.handleBorderChanged(evb.getPos());
            state = JobState::Finished;
        }
        break;
        case EventType::BuildingFinished:
        {
            const auto& evb = *checkedCast<AIEvent::Building*>(ev.get());
            aijh.handleBuildingFinished(evb.getPos(), evb.getBuildingType());
            state = JobState::Finished;
        }
        break;
        case EventType::ExpeditionWaiting:
        {
            const auto& lvb = *checkedCast<AIEvent::Location*>(ev.get());
            aijh.handleExpedition(lvb.getPos());
            state = JobState::Finished;
        }
        break;
        case EventType::TreeChopped:
        {
            const auto& lvb = *checkedCast<AIEvent::Location*>(ev.get());
            aijh.handleTreeChopped(lvb.getPos());
            state = JobState::Finished;
        }
        break;
        case EventType::NewColonyFounded:
        {
            const auto& lvb = *checkedCast<AIEvent::Location*>(ev.get());
            aijh.handleNewColonyFounded(lvb.getPos());
            state = JobState::Finished;
        }
        break;
        case EventType::ShipBuilt:
        {
            const auto& lvb = *checkedCast<AIEvent::Location*>(ev.get());
            aijh.handleShipBuilt(lvb.getPos());
            state = JobState::Finished;
        }
        break;
        case EventType::RoadConstructionComplete:
        {
            const auto& dvb = *checkedCast<AIEvent::Direction*>(ev.get());
            aijh.handleRoadConstructionComplete(dvb.getPos(), dvb.getDirection());
            state = JobState::Finished;
        }
        break;
        case EventType::RoadConstructionFailed:
        {
            const auto& dvb = *checkedCast<AIEvent::Direction*>(ev.get());
            aijh.handleRoadConstructionFailed(dvb.getPos(), dvb.getDirection());
            state = JobState::Finished;
        }
        break;
        case EventType::LuaConstructionOrder:
        {
            const auto& evb = *checkedCast<AIEvent::Building*>(ev.get());
            aijh.executeLuaConstructionOrder(evb.getPos(), evb.getBuildingType(), true);
            state = JobState::Finished;
        }
        break;
        default:
            // status = JobState::Failed;
            break;
    }

    // temp only:
    state = JobState::Finished;
}

void ConnectJob::executeJob()
{
#ifdef DEBUG_AI
    std::cout << "Player " << (unsigned)aijh.getPlayerId() << ", ConnectJob executed..." << std::endl;
#endif

    // can the ai still construct here? else return and try again later
    AIConstruction& construction = aijh.getConstruction();
    if(!construction.canStillConstructHere(flagPos))
        return;

    const GameWorldBase& world = aijh.getWorld();
    const auto* flag = world.getSpecObj<noFlag>(flagPos);

    if(!flag)
    {
#ifdef DEBUG_AI
        std::cout << "Flag is gone." << std::endl;
#endif
        state = JobState::Failed;
        return;
    }

    // is flag of a military building and has some road connection alraedy (not necessarily to a warehouse so this is
    // required to avoid multiple connections on mil buildings)
    if(world.isMilitaryBuildingOnNode(world.getNeighbour(flag->getPos(), Direction::NorthWest), true))
    {
        for(unsigned dir = 2; dir < 7; dir++)
        {
            if(flag->getRoute(convertToDirection(dir)))
            {
                state = JobState::Finished;
                return;
            }
        }
    }

    // already connected?
    if(!construction.isConnectedToRoadSystem(flag))
    {
#ifdef DEBUG_AI
        std::cout << "Flag is not connected..." << std::endl;
#endif
        // building road possible?
        if(!construction.connectFlagToRoadSytem(flag, route, 24))
        {
#ifdef DEBUG_AI
            std::cout << "Flag is not connectable." << std::endl;
#endif
            state = JobState::Failed;
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
        aijh.recalcGround(flagPos, route);
        state = JobState::Finished;
    }
}

void SearchJob::executeJob()
{
    state = JobState::Failed;
    PositionSearchState searchState = search->execute(aijh);

    if(searchState == PositionSearchState::InProgress)
        state = JobState::Waiting;
    else if(searchState == PositionSearchState::Failed)
        state = JobState::Failed;
    else
    {
        state = JobState::Finished;
        aijh.addBuildJob(search->getBld(), search->getResultPt(), true, false);
    }
}

SearchJob::~SearchJob()
{
    delete search;
}

} // namespace AIJH
