// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AIPlayerJH.h"

#include "ai/aijh/debug/AIRuntimeProfiler.h"
#include "ai/aijh/planning/AIConstruction.h"
#include "ai/aijh/planning/BuildingPlanner.h"
#include "ai/aijh/planning/Jobs.h"
#include "ai/aijh/runtime/AIEventHandler.h"
#include "ai/aijh/runtime/AIRoadController.h"
#include "ai/AIEvents.h"
#include "buildings/noBaseBuilding.h"
#include "buildings/noBuildingSite.h"
#include "buildings/nobHarborBuilding.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobUsual.h"
#include "gameData/BuildingConsts.h"
#include "gameData/MilitaryConsts.h"
#include "helpers/EnumRange.h"
#include "helpers/MaxEnumValue.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noShip.h"

#include <algorithm>
#include <array>
#include <memory>
#include <utility>
#include <vector>

namespace AIJH {

void AIEventHandler::ExecuteAIJob()
{
    unsigned quota = 10;
    {
        const ScopedAIRuntimeProfile eventJobsProfile(AIRuntimeProfileSection::ExecuteEventJobs, quota);
        while(owner_.eventManager.EventAvailable() && quota)
        {
            quota--;
            owner_.currentJob = std::make_unique<EventJob>(owner_, owner_.eventManager.GetEvent());
            owner_.currentJob->ExecuteJob();
        }
    }

    quota = std::max<unsigned>(5, (owner_.aii.GetStorehouses().size() + owner_.aii.GetMilitaryBuildings().size()) * 1);
    if(quota > 40)
        quota = 40;

    const ScopedAIRuntimeProfile constructionJobsProfile(AIRuntimeProfileSection::ExecuteConstructionJobs, quota);
    owner_.construction->ExecuteJobs(quota);
}

void AIEventHandler::HandleNewMilitaryBuildingOccupied(const MapPoint pt)
{
    owner_.ForgetLostMilitaryBuilding(pt);

    RemoveAllUnusedRoads(pt);
    owner_.bldPlanner->UpdateBuildingsWanted(owner_);
    const auto* mil = owner_.gwb.GetSpecObj<nobMilitary>(pt);
    if(!mil)
        return;

    if(mil->GetFrontierDistance() != FrontierDistance::Far)
    {
        if(mil->IsGoldDisabled())
            owner_.aii.SetCoinsAllowed(pt, true);
    } else if((mil->GetBuildingType() == BuildingType::Barracks || mil->GetBuildingType() == BuildingType::Guardhouse)
              && mil->GetBuildingType() != owner_.construction->GetBiggestAllowedMilBuilding())
    {
        if(!mil->IsGoldDisabled())
            owner_.aii.SetCoinsAllowed(pt, false);
    }

    owner_.AddBuildJob(BuildingType::HarborBuilding, pt);
    if(!owner_.IsInvalidShipyardPosition(pt))
        owner_.AddBuildJob(BuildingType::Shipyard, pt);
    if(owner_.SoldierAvailable())
        owner_.AddMilitaryBuildJob(pt);

    if(owner_.construction->Wanted(BuildingType::Storehouse))
        owner_.AddGlobalBuildJob(BuildingType::Storehouse);

    std::array<BuildingType, 8> bldToTest = {
      BuildingType::Quarry,  BuildingType::GoldMine,    BuildingType::CoalMine,   BuildingType::IronMine,
      BuildingType::GraniteMine, BuildingType::Fishery, BuildingType::Hunter, BuildingType::Forester};

    for(const BuildingType type : bldToTest)
    {
        if(owner_.construction->Wanted(type))
            owner_.AddBuildJob(type, pt);
    }
}

void AIEventHandler::HandleBuildingDestroyed(MapPoint pt, BuildingType bld)
{
    switch(bld)
    {
        case BuildingType::Charburner:
        case BuildingType::Farm: owner_.SetFarmedNodes(pt, false); break;
        case BuildingType::HarborBuilding:
        {
            for(const MapPoint curPt : owner_.gwb.GetPointsInRadius(pt, 2))
            {
                const auto* const bb = owner_.gwb.GetSpecObj<noBaseBuilding>(curPt);
                if(bb)
                {
                    owner_.aii.DestroyBuilding(curPt);
                } else
                {
                    const auto* const bs = owner_.gwb.GetSpecObj<noBuildingSite>(curPt);
                    if(bs)
                        owner_.aii.DestroyFlag(owner_.gwb.GetNeighbour(curPt, Direction::SouthEast));
                }
            }
            break;
        }
        default: break;
    }
}

void AIEventHandler::HandleRoadConstructionComplete(MapPoint pt, Direction dir)
{
    owner_.roadController_->HandleRoadConstructionComplete(pt, dir);
}

void AIEventHandler::HandleRoadConstructionFailed(const MapPoint pt, Direction dir)
{
    owner_.roadController_->HandleRoadConstructionFailed(pt, dir);
}

void AIEventHandler::HandleMilitaryBuildingLost(const MapPoint pt)
{
    owner_.RememberLostMilitaryBuilding(pt);
    HandleLostLand(pt);
}

void AIEventHandler::HandleBuildingFinished(const MapPoint pt, BuildingType bld)
{
    switch(bld)
    {
        case BuildingType::HarborBuilding:
            owner_.UpdateNodesAround(pt, 8);
            RemoveAllUnusedRoads(pt);
            owner_.aii.ChangeReserve(pt, 0, 1);

            if(owner_.HarborPosRelevant(owner_.gwb.GetHarborPointID(pt), true))
                owner_.aii.StartStopExpedition(pt, true);
            break;

        case BuildingType::Shipyard: owner_.aii.SetShipYardMode(pt, true); break;
        case BuildingType::Storehouse: break;
        default: break;
    }
}

void AIEventHandler::HandleNewColonyFounded(const MapPoint pt)
{
    owner_.construction->AddConnectFlagJob(owner_.gwb.GetSpecObj<noFlag>(owner_.gwb.GetNeighbour(pt, Direction::SouthEast)));
}

void AIEventHandler::HandleExpedition(const noShip* ship)
{
    if(!ship->IsWaitingForExpeditionInstructions())
        return;
    if(ship->IsAbleToFoundColony())
    {
        owner_.aii.FoundColony(ship);
    } else
    {
        const unsigned offset = rand() % helpers::MaxEnumValue_v<ShipDirection>;
        for(auto dir : helpers::EnumRange<ShipDirection>{})
        {
            dir = ShipDirection((rttr::enum_cast(dir) + offset) % helpers::MaxEnumValue_v<ShipDirection>);
            if(owner_.aii.IsExplorationDirectionPossible(ship->GetPos(), ship->GetCurrentHarbor(), dir))
            {
                owner_.aii.TravelToNextSpot(dir, ship);
                return;
            }
        }
        owner_.aii.CancelExpedition(ship);
    }
}

void AIEventHandler::HandleExpedition(const MapPoint pt)
{
    const noShip* ship = nullptr;

    for(const noBase& obj : owner_.gwb.GetFigures(pt))
    {
        if(obj.GetGOT() == GO_Type::Ship)
        {
            const auto& curShip = static_cast<const noShip&>(obj);
            if(curShip.GetPlayerId() == owner_.playerId && curShip.IsWaitingForExpeditionInstructions())
            {
                ship = &curShip;
                break;
            }
        }
    }
    if(ship)
        HandleExpedition(ship);
}

void AIEventHandler::HandleTreeChopped(const MapPoint pt)
{
    owner_.GetAINode(pt).reachable = true;

    owner_.UpdateNodesAround(pt, 3);

    if(rand() % 2 == 0)
        owner_.AddMilitaryBuildJob(pt);
}

void AIEventHandler::HandleNoMoreResourcesReachable(const MapPoint pt, BuildingType bld)
{
    if(!owner_.aii.IsObjectTypeOnNode(pt, NodalObjectType::Building))
        return;

    if(bld == BuildingType::Woodcutter)
    {
        for(const nobUsual* forester : owner_.aii.GetBuildings(BuildingType::Forester))
        {
            if(owner_.gwb.CalcDistance(pt, forester->GetPos()) <= RES_RADIUS[AIResource::Wood])
            {
                const unsigned maxdist = owner_.gwb.CalcDistance(pt, forester->GetPos());
                int betterwoodcutters = 0;
                for(const nobUsual* woodcutter : owner_.aii.GetBuildings(BuildingType::Woodcutter))
                {
                    if(pt == woodcutter->GetPos())
                        continue;
                    if(owner_.gwb.CalcDistance(woodcutter->GetPos(), pt) > RES_RADIUS[AIResource::Wood])
                        continue;
                    if(owner_.gwb.CalcDistance(woodcutter->GetPos(), forester->GetPos()) <= maxdist)
                    {
                        betterwoodcutters++;
                        if(betterwoodcutters >= 2)
                            break;
                    }
                }
                if(betterwoodcutters < 2)
                    return;
            }
        }
    }

    owner_.aii.DestroyBuilding(pt);

    if(bld == BuildingType::Fishery)
        owner_.GetResMap(AIResource::Fish).avoidPosition(pt);

    owner_.UpdateNodesAround(pt, 11);
    RemoveUnusedRoad(*owner_.gwb.GetSpecObj<noFlag>(owner_.gwb.GetNeighbour(pt, Direction::SouthEast)),
                     Direction::NorthWest, true);

    owner_.AddMilitaryBuildJob(pt);

    if(bld != BuildingType::Hunter && bld != BuildingType::Woodcutter)
        owner_.AddBuildJob(bld, pt);
}

void AIEventHandler::HandleShipBuilt(const MapPoint pt)
{
    const std::list<nobUsual*>& shipyards = owner_.aii.GetBuildings(BuildingType::Shipyard);
    bool wantMoreShips;
    const unsigned numRelevantSeas = owner_.GetNumAIRelevantSeaIds();
    if(numRelevantSeas == 0)
        wantMoreShips = false;
    else if(numRelevantSeas == 1)
        wantMoreShips = owner_.aii.GetNumShips() <= owner_.gwb.GetNumHarborPoints();
    else
    {
        const unsigned wantedShipCt = std::min<unsigned>(7, 3 * shipyards.size());
        wantMoreShips = owner_.aii.GetNumShips() < wantedShipCt;
    }
    if(!wantMoreShips)
    {
        unsigned mindist = 12;
        const nobUsual* creatingShipyard = nullptr;
        for(const nobUsual* shipyard : shipyards)
        {
            const unsigned distance = owner_.gwb.CalcDistance(shipyard->GetPos(), pt);
            if(distance < mindist)
            {
                mindist = distance;
                creatingShipyard = shipyard;
            }
        }
        if(creatingShipyard)
            owner_.aii.SetProductionEnabled(creatingShipyard->GetPos(), false);
    }
}

void AIEventHandler::HandleBorderChanged(const MapPoint pt)
{
    owner_.UpdateNodesAround(pt, 11);

    const auto* mil = owner_.gwb.GetSpecObj<nobMilitary>(pt);
    if(mil)
    {
        if(mil->GetFrontierDistance() != FrontierDistance::Far)
        {
            if(mil->IsGoldDisabled())
                owner_.aii.SetCoinsAllowed(pt, true);

            for(unsigned rank = 0; rank < NUM_SOLDIER_RANKS; ++rank)
            {
                if(mil->GetTroopLimit(rank) != mil->GetMaxTroopsCt())
                    owner_.aii.SetTroopLimit(mil->GetPos(), rank, mil->GetMaxTroopsCt());
            }
        }
        if(mil->GetBuildingType() != owner_.construction->GetBiggestAllowedMilBuilding())
            owner_.AddMilitaryBuildJob(pt);
    }
}

void AIEventHandler::HandleLostLand(const MapPoint pt)
{
    if(owner_.aii.GetStorehouses().empty())
        return;
    RemoveAllUnusedRoads(pt);
}

void AIEventHandler::CheckExpeditions()
{
    const std::list<nobHarborBuilding*>& harbors = owner_.aii.GetHarbors();
    for(const nobHarborBuilding* harbor : harbors)
    {
        const bool isHarborRelevant = owner_.HarborPosRelevant(harbor->GetHarborPosID(), true);
        if(harbor->IsExpeditionActive() != isHarborRelevant)
            owner_.aii.StartStopExpedition(harbor->GetPos(), isHarborRelevant);
    }

    const std::vector<noShip*>& ships = owner_.aii.GetShips();
    for(const noShip* ship : ships)
    {
        if(ship->IsWaitingForExpeditionInstructions())
            HandleExpedition(ship);
    }
}

void AIEventHandler::SendAIEvent(std::unique_ptr<AIEvent::Base> ev)
{
    owner_.eventManager.AddAIEvent(std::move(ev));
}

bool AIEventHandler::IsFlagPartOfCircle(const noFlag& startFlag, unsigned maxlen, const noFlag& curFlag,
                                        helpers::OptionalEnum<Direction> excludeDir,
                                        std::vector<const noFlag*> oldFlags)
{
    return owner_.roadController_->IsFlagPartOfCircle(startFlag, maxlen, curFlag, excludeDir, std::move(oldFlags));
}

void AIEventHandler::RemoveAllUnusedRoads(const MapPoint pt)
{
    owner_.roadController_->RemoveAllUnusedRoads(pt);
}

void AIEventHandler::CheckForUnconnectedBuildingSites()
{
    owner_.roadController_->CheckForUnconnectedBuildingSites();
}

bool AIEventHandler::RemoveUnusedRoad(const noFlag& startFlag, helpers::OptionalEnum<Direction> excludeDir,
                                      bool firstflag, bool allowcircle, bool keepstartflag)
{
    return owner_.roadController_->RemoveUnusedRoad(startFlag, excludeDir, firstflag, allowcircle, keepstartflag);
}

void AIPlayerJH::ExecuteAIJob() { eventHandler_->ExecuteAIJob(); }

void AIPlayerJH::HandleNewMilitaryBuildingOccupied(const MapPoint pt)
{
    eventHandler_->HandleNewMilitaryBuildingOccupied(pt);
}

void AIPlayerJH::HandleBuildingDestroyed(MapPoint pt, BuildingType bld)
{
    eventHandler_->HandleBuildingDestroyed(pt, bld);
}

void AIPlayerJH::HandleRoadConstructionComplete(MapPoint pt, Direction dir)
{
    eventHandler_->HandleRoadConstructionComplete(pt, dir);
}

void AIPlayerJH::HandleRoadConstructionFailed(const MapPoint pt, Direction dir)
{
    eventHandler_->HandleRoadConstructionFailed(pt, dir);
}

void AIPlayerJH::HandleMilitaryBuildingLost(const MapPoint pt)
{
    eventHandler_->HandleMilitaryBuildingLost(pt);
}

void AIPlayerJH::HandleBuildingFinished(const MapPoint pt, BuildingType bld)
{
    eventHandler_->HandleBuildingFinished(pt, bld);
}

void AIPlayerJH::HandleNewColonyFounded(const MapPoint pt) { eventHandler_->HandleNewColonyFounded(pt); }

void AIPlayerJH::HandleExpedition(const noShip* ship) { eventHandler_->HandleExpedition(ship); }

void AIPlayerJH::HandleExpedition(const MapPoint pt) { eventHandler_->HandleExpedition(pt); }

void AIPlayerJH::HandleTreeChopped(const MapPoint pt) { eventHandler_->HandleTreeChopped(pt); }

void AIPlayerJH::HandleNoMoreResourcesReachable(const MapPoint pt, BuildingType bld)
{
    eventHandler_->HandleNoMoreResourcesReachable(pt, bld);
}

void AIPlayerJH::HandleShipBuilt(const MapPoint pt) { eventHandler_->HandleShipBuilt(pt); }

void AIPlayerJH::HandleBorderChanged(const MapPoint pt) { eventHandler_->HandleBorderChanged(pt); }

void AIPlayerJH::HandleLostLand(const MapPoint pt) { eventHandler_->HandleLostLand(pt); }

void AIPlayerJH::CheckExpeditions() { eventHandler_->CheckExpeditions(); }

void AIPlayerJH::SendAIEvent(std::unique_ptr<AIEvent::Base> ev) { eventHandler_->SendAIEvent(std::move(ev)); }

bool AIPlayerJH::IsFlagPartOfCircle(const noFlag& startFlag, unsigned maxlen, const noFlag& curFlag,
                                    helpers::OptionalEnum<Direction> excludeDir, std::vector<const noFlag*> oldFlags)
{
    return eventHandler_->IsFlagPartOfCircle(startFlag, maxlen, curFlag, excludeDir, std::move(oldFlags));
}

void AIPlayerJH::RemoveAllUnusedRoads(const MapPoint pt) { eventHandler_->RemoveAllUnusedRoads(pt); }

void AIPlayerJH::CheckForUnconnectedBuildingSites() { eventHandler_->CheckForUnconnectedBuildingSites(); }

bool AIPlayerJH::RemoveUnusedRoad(const noFlag& startFlag, helpers::OptionalEnum<Direction> excludeDir,
                                  bool firstflag, bool allowcircle, bool keepstartflag)
{
    return eventHandler_->RemoveUnusedRoad(startFlag, excludeDir, firstflag, allowcircle, keepstartflag);
}

} // namespace AIJH
