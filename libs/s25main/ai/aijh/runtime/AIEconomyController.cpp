// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AIPlayerJH.h"

#include "FindWhConditions.h"
#include "GlobalGameSettings.h"
#include "RTTR_Assert.h"
#include "ToolPriorityEventLogger.h"
#include "addons/const_addons.h"
#include "ai/aijh/config/AIConfig.h"
#include "ai/aijh/planning/AIConstruction.h"
#include "ai/aijh/planning/BuildingPlanner.h"
#include "ai/aijh/planning/GlobalPositionFinder.h"
#include "ai/aijh/planning/Jobs.h"
#include "ai/aijh/runtime/AIEconomyController.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobUsual.h"
#include "gameData/BuildingConsts.h"
#include "gameData/BuildingProperties.h"
#include "gameData/JobConsts.h"
#include "gameData/MilitaryConsts.h"
#include "gameData/ToolConsts.h"
#include "gameTypes/VisualSettings.h"
#include "helpers/EnumRange.h"
#include "helpers/containerUtils.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <memory>
#include <vector>

namespace {

std::size_t GetDistributionIndex(GoodType good, BuildingType building)
{
    for(std::size_t idx = 0; idx < distributionMap.size(); ++idx)
    {
        if(std::get<0>(distributionMap[idx]) == good && std::get<1>(distributionMap[idx]) == building)
            return idx;
    }
    RTTR_Assert(false);
    return std::size_t{};
}

std::size_t GetIronMetalworksDistributionIndex()
{
    static const std::size_t idx = GetDistributionIndex(GoodType::Iron, BuildingType::Metalworks);
    return idx;
}

} // namespace

namespace AIJH {

void AIEconomyController::PlanNewBuildings(const unsigned gf)
{
    owner_.bldPlanner->UpdateBuildingsWanted(owner_);

    std::array<BuildingType, 4> bldToTest = {
      {BuildingType::HarborBuilding, BuildingType::Shipyard, BuildingType::Hunter, BuildingType::Charburner}};

    std::array<BuildingType, 21> globalBldToTest = {
      {BuildingType::Sawmill, BuildingType::Farm,         BuildingType::Woodcutter, BuildingType::Forester,
       BuildingType::DonkeyBreeder, BuildingType::Fishery,      BuildingType::Quarry,     BuildingType::GoldMine,
       BuildingType::IronMine,      BuildingType::CoalMine,     BuildingType::GraniteMine,
       BuildingType::Bakery,        BuildingType::Brewery,      BuildingType::Armory,
       BuildingType::Metalworks,    BuildingType::Ironsmelter,  BuildingType::Slaughterhouse,
       BuildingType::PigFarm,       BuildingType::Mill,         BuildingType::Well, BuildingType::Mint}};

    const std::list<nobBaseWarehouse*>& storehouses = owner_.aii.GetStorehouses();
    if(!storehouses.empty())
    {
        nobBaseWarehouse* wh = GetUpgradeBuildingWarehouse();
        SetGatheringForUpgradeWarehouse(wh);
        SetSendingForUpgradeWarehouse(wh);

        if(owner_.ggs.GetMaxMilitaryRank() > 0)
            DistributeMaxRankSoldiersByBlocking(5, wh);

        DistributeGoodsByBlocking(GoodType::Boards, 30);
        DistributeGoodsByBlocking(GoodType::Stones, 50);

        int randomStore = rand() % (storehouses.size());
        auto it = storehouses.begin();
        std::advance(it, randomStore);
        const MapPoint whPos = (*it)->GetPos();
        owner_.UpdateNodesAround(whPos, 15);
        for(const BuildingType type : bldToTest)
        {
            if(owner_.construction->Wanted(type))
                AddBuildJobAroundEveryWarehouse(type);
        }
        for(const BuildingType type : globalBldToTest)
        {
            if(owner_.construction->Wanted(type))
                AddGlobalBuildJob(type);
        }
        if(gf > 1500 || owner_.aii.GetInventory().goods[GoodType::Boards] > 11)
            AddMilitaryBuildJob(whPos);
    }

    const std::list<nobMilitary*>& militaryBuildings = owner_.aii.GetMilitaryBuildings();
    if(militaryBuildings.empty())
        return;
    int randomMiliBld = rand() % militaryBuildings.size();
    auto it2 = militaryBuildings.begin();
    std::advance(it2, randomMiliBld);
    const MapPoint bldPos = (*it2)->GetPos();
    AddMilitaryBuildJob(bldPos);
    if((*it2)->IsUseless() && (*it2)->IsDemolitionAllowed() && randomMiliBld != UpdateUpgradeBuilding())
        owner_.aii.DestroyBuilding(bldPos);
}

nobBaseWarehouse* AIEconomyController::GetUpgradeBuildingWarehouse()
{
    const std::list<nobBaseWarehouse*>& storehouses = owner_.aii.GetStorehouses();
    if(storehouses.empty())
        return nullptr;
    nobBaseWarehouse* wh = storehouses.front();
    const int uub = UpdateUpgradeBuilding();

    if(uub >= 0 && storehouses.size() > 1)
    {
        auto upgradeBldIt = owner_.aii.GetMilitaryBuildings().begin();
        std::advance(upgradeBldIt, uub);
        wh = owner_.aii.FindWarehouse(**upgradeBldIt, FW::NoCondition(), false, false);
        if(!wh)
            wh = storehouses.front();
    }
    return wh;
}

void AIEconomyController::AddMilitaryBuildJob(MapPoint pt)
{
    const auto milBld = owner_.construction->ChooseMilitaryBuilding(pt);
    if(!milBld)
        return;
    if(BuildingProperties::IsMilitary(*milBld))
        AddGlobalBuildJob(*milBld);
    else
        AddBuildJob(*milBld, pt, false, true);
}

void AIEconomyController::AddGlobalBuildJob(BuildingType type)
{
    owner_.construction->AddGlobalBuildJob(
      std::make_unique<BuildJob>(owner_, type, MapPoint::Invalid(), SearchMode::Global));
}

void AIEconomyController::AddBuildJob(BuildingType type, const MapPoint pt, bool front, bool searchPosition)
{
    owner_.construction->AddBuildJob(
      std::make_unique<BuildJob>(owner_, type, pt, searchPosition ? SearchMode::Radius : SearchMode::None), front);
}

MapPoint AIEconomyController::FindBestPosition(BuildingType bt)
{
    return owner_.globalPositionFinder->FindBestPosition(bt);
}

void AIEconomyController::AddBuildJobAroundEveryWarehouse(BuildingType bt)
{
    for(const nobBaseWarehouse* wh : owner_.aii.GetStorehouses())
        AddBuildJob(bt, wh->GetPos(), false, true);
}

void AIEconomyController::AddBuildJobAroundEveryMilBld(BuildingType bt)
{
    for(const nobMilitary* milBld : owner_.aii.GetMilitaryBuildings())
        AddBuildJob(bt, milBld->GetPos(), false, true);
}

void AIEconomyController::SetSendingForUpgradeWarehouse(nobBaseWarehouse* upgradewarehouse)
{
    for(const nobBaseWarehouse* wh : owner_.aii.GetStorehouses())
    {
        const MapPoint whPos = wh->GetPos();
        if(upgradewarehouse->GetPos() != whPos)
        {
            for(const nobBaseWarehouse* innerWh : owner_.aii.GetStorehouses())
            {
                const MapPoint innerWhPos = innerWh->GetPos();
                if(upgradewarehouse->GetPos() == innerWhPos)
                {
                    owner_.aii.SetInventorySetting(innerWhPos, GoodType::Boards, EInventorySetting::Send);
                    owner_.aii.SetInventorySetting(innerWhPos, GoodType::Stones, EInventorySetting::Send);
                    owner_.aii.SetInventorySetting(innerWhPos, GoodType::Water, EInventorySetting::Send);
                    owner_.aii.SetInventorySetting(innerWhPos, GoodType::Flour, EInventorySetting::Send);
                    owner_.aii.SetInventorySetting(innerWhPos, GoodType::Grain, EInventorySetting::Send);
                }
            }
        }
    }
}

void AIEconomyController::SetGatheringForUpgradeWarehouse(nobBaseWarehouse* upgradewarehouse)
{
    for(const nobBaseWarehouse* wh : owner_.aii.GetStorehouses())
    {
        const MapPoint whPos = wh->GetPos();
        if(upgradewarehouse->GetPos() != whPos)
        {
            if(wh->IsInventorySetting(GoodType::Beer, EInventorySetting::Collect))
                owner_.aii.SetInventorySetting(whPos, GoodType::Beer, InventorySetting());

            if(wh->IsInventorySetting(GoodType::Sword, EInventorySetting::Collect))
                owner_.aii.SetInventorySetting(whPos, GoodType::Sword, InventorySetting());

            if(wh->IsInventorySetting(GoodType::ShieldRomans, EInventorySetting::Collect))
                owner_.aii.SetInventorySetting(whPos, GoodType::ShieldRomans, InventorySetting());

            if(wh->IsInventorySetting(Job::Private, EInventorySetting::Collect))
                owner_.aii.SetInventorySetting(whPos, Job::Private, InventorySetting());

            if(wh->IsInventorySetting(Job::Helper, EInventorySetting::Collect))
                owner_.aii.SetInventorySetting(whPos, Job::Helper, InventorySetting());
        } else
        {
            if(!wh->IsInventorySetting(GoodType::Beer, EInventorySetting::Collect))
                owner_.aii.SetInventorySetting(whPos, GoodType::Beer, EInventorySetting::Collect);

            if(!wh->IsInventorySetting(GoodType::Sword, EInventorySetting::Collect))
                owner_.aii.SetInventorySetting(whPos, GoodType::Sword, EInventorySetting::Collect);

            if(!wh->IsInventorySetting(GoodType::ShieldRomans, EInventorySetting::Collect))
                owner_.aii.SetInventorySetting(whPos, GoodType::ShieldRomans, EInventorySetting::Collect);

            if(!wh->IsInventorySetting(Job::Private, EInventorySetting::Collect) && owner_.ggs.GetMaxMilitaryRank() > 0)
                owner_.aii.SetInventorySetting(whPos, Job::Private, EInventorySetting::Collect);

            if(wh->GetInventory().people[Job::Helper] < 50)
            {
                if(!wh->IsInventorySetting(Job::Helper, EInventorySetting::Collect))
                    owner_.aii.SetInventorySetting(whPos, Job::Helper, EInventorySetting::Collect);
            } else
            {
                if(wh->IsInventorySetting(Job::Helper, EInventorySetting::Collect))
                    owner_.aii.SetInventorySetting(whPos, Job::Helper, InventorySetting());
            }
        }
    }
}

void AIEconomyController::DistributeGoodsByBlocking(const GoodType good, unsigned limit)
{
    const std::list<nobBaseWarehouse*>& storehouses = owner_.aii.GetStorehouses();
    if(owner_.aii.GetHarbors().size() >= storehouses.size() / 2)
    {
        for(nobBaseWarehouse* wh : storehouses)
        {
            if(wh->IsInventorySetting(good, EInventorySetting::Stop))
                owner_.aii.SetInventorySetting(wh->GetPos(), good,
                                               wh->GetInventorySetting(good).Toggle(EInventorySetting::Stop));
        }
        return;
    }

    RTTR_Assert(storehouses.size() >= 2);
    std::vector<std::vector<const nobBaseWarehouse*>> whsByReachability;
    for(const nobBaseWarehouse* wh : storehouses)
    {
        bool foundConnectedWh = false;
        for(std::vector<const nobBaseWarehouse*>& whGroup : whsByReachability)
        {
            if(owner_.aii.FindPathOnRoads(*wh, *whGroup.front()))
            {
                whGroup.push_back(wh);
                foundConnectedWh = true;
                break;
            }
        }
        if(!foundConnectedWh)
            whsByReachability.push_back(std::vector<const nobBaseWarehouse*>(1, wh));
    }

    for(const std::vector<const nobBaseWarehouse*>& whGroup : whsByReachability)
    {
        bool allWHsHaveLimit = true;
        for(const nobBaseWarehouse* wh : whGroup)
        {
            if(wh->GetNumVisualWares(good) <= limit)
            {
                allWHsHaveLimit = false;
                break;
            }
        }
        if(allWHsHaveLimit)
        {
            for(const nobBaseWarehouse* wh : whGroup)
            {
                if(wh->IsInventorySetting(good, EInventorySetting::Stop))
                    owner_.aii.SetInventorySetting(wh->GetPos(), good,
                                                   wh->GetInventorySetting(good).Toggle(EInventorySetting::Stop));
            }
        } else
        {
            for(const nobBaseWarehouse* wh : whGroup)
            {
                if(wh->GetNumVisualWares(good) <= limit)
                {
                    if(wh->IsInventorySetting(good, EInventorySetting::Stop))
                        owner_.aii.SetInventorySetting(wh->GetPos(), good,
                                                       wh->GetInventorySetting(good).Toggle(EInventorySetting::Stop));
                } else
                {
                    if(!wh->IsInventorySetting(good, EInventorySetting::Stop))
                        owner_.aii.SetInventorySetting(wh->GetPos(), good,
                                                       wh->GetInventorySetting(good).Toggle(EInventorySetting::Stop));
                }
            }
        }
    }
}

void AIEconomyController::DistributeMaxRankSoldiersByBlocking(unsigned limit, nobBaseWarehouse* upwh)
{
    const std::list<nobBaseWarehouse*>& storehouses = owner_.aii.GetStorehouses();
    unsigned numCompleteWh = storehouses.size();

    if(numCompleteWh < 1)
        return;

    const Job maxRankJob = SOLDIER_JOBS[owner_.ggs.GetMaxMilitaryRank()];

    if(numCompleteWh == 1)
    {
        nobBaseWarehouse& wh = *storehouses.front();
        if(wh.IsInventorySetting(maxRankJob, EInventorySetting::Stop))
            owner_.aii.SetInventorySetting(wh.GetPos(), maxRankJob,
                                           wh.GetInventorySetting(maxRankJob).Toggle(EInventorySetting::Stop));
        return;
    }

    std::list<const nobMilitary*> frontierMils;
    for(const nobMilitary* wh : owner_.aii.GetMilitaryBuildings())
    {
        if(wh->GetFrontierDistance() != FrontierDistance::Far && !wh->IsNewBuilt())
            frontierMils.push_back(wh);
    }
    std::list<const nobBaseWarehouse*> frontierWhs;
    for(const nobBaseWarehouse* wh : storehouses)
    {
        for(const nobMilitary* milBld : frontierMils)
        {
            if(owner_.gwb.CalcDistance(wh->GetPos(), milBld->GetPos()) < 12)
            {
                frontierWhs.push_back(wh);
                break;
            }
        }
    }

    if(!frontierWhs.empty())
    {
        bool hasUnderstaffedWh = false;
        for(const nobBaseWarehouse* wh : frontierWhs)
        {
            if(wh->GetInventory().people[maxRankJob] < limit)
            {
                hasUnderstaffedWh = true;
                break;
            }
        }
        for(const nobBaseWarehouse* wh : storehouses)
        {
            const bool shouldBlock = !helpers::contains(frontierWhs, wh)
                                     || (hasUnderstaffedWh && wh->GetInventory().people[maxRankJob] >= limit);
            if(shouldBlock != wh->IsInventorySetting(maxRankJob, EInventorySetting::Stop))
                owner_.aii.SetInventorySetting(wh->GetPos(), maxRankJob,
                                               wh->GetInventorySetting(maxRankJob).Toggle(EInventorySetting::Stop));
        }
    } else
    {
        bool hasUnderstaffedWh = false;
        for(const nobBaseWarehouse* wh : storehouses)
        {
            if(wh->GetInventory().people[maxRankJob] < limit && wh->GetPos() != upwh->GetPos())
            {
                hasUnderstaffedWh = true;
                break;
            }
        }
        for(const nobBaseWarehouse* wh : storehouses)
        {
            bool shouldBlock;
            if(wh->GetPos() == upwh->GetPos())
            {
                shouldBlock = true;
            } else if(hasUnderstaffedWh)
            {
                shouldBlock = wh->GetInventory().people[maxRankJob] >= limit;
            } else
            {
                shouldBlock = false;
            }
            if(shouldBlock != wh->IsInventorySetting(maxRankJob, EInventorySetting::Stop))
                owner_.aii.SetInventorySetting(wh->GetPos(), maxRankJob,
                                               wh->GetInventorySetting(maxRankJob).Toggle(EInventorySetting::Stop));
        }
    }
}

unsigned AIEconomyController::SoldierAvailable(int rank)
{
    unsigned freeSoldiers = 0;
    for(const nobBaseWarehouse* wh : owner_.aii.GetStorehouses())
    {
        const Inventory& inventory = wh->GetInventory();
        if(rank < 0)
        {
            for(const Job job : SOLDIER_JOBS)
                freeSoldiers += inventory[job];
        } else
        {
            freeSoldiers += inventory[SOLDIER_JOBS[rank]];
        }
    }
    return freeSoldiers;
}

void AIEconomyController::InitStoreAndMilitarylists()
{
    for(const nobUsual* farm : owner_.aii.GetBuildings(BuildingType::Farm))
        owner_.SetFarmedNodes(farm->GetPos(), true);
    for(const nobUsual* charburner : owner_.aii.GetBuildings(BuildingType::Charburner))
        owner_.SetFarmedNodes(charburner->GetPos(), true);
    UpdateUpgradeBuilding();
}

int AIEconomyController::UpdateUpgradeBuilding()
{
    upgradeBldPos_ = MapPoint::Invalid();
    return -1;
}

void AIEconomyController::InitDistribution()
{
    Distributions goodSettings;
    goodSettings[0] = 10;
    goodSettings[1] = 10;
    goodSettings[2] = 10;
    goodSettings[3] = 10;
    goodSettings[4] = 2;

    goodSettings[5] = 10;
    goodSettings[6] = 10;
    goodSettings[7] = 10;
    goodSettings[8] = 10;
    goodSettings[9] = 10;

    goodSettings[10] = 10;
    goodSettings[11] = 10;

    goodSettings[12] = 10;
    goodSettings[13] = 10;
    goodSettings[14] = 10;

    goodSettings[15] = 10;
    goodSettings[16] = 10;
    goodSettings[17] = 2;

    goodSettings[18] = 10;
    goodSettings[19] = 4;
    goodSettings[20] = 2;

    goodSettings[21] = 10;
    goodSettings[22] = 10;
    goodSettings[23] = 10;
    goodSettings[24] = 10;
    goodSettings[25] = 2;

    const std::size_t ironMetalworksIdx = GetDistributionIndex(GoodType::Iron, BuildingType::Metalworks);
    metalworksIronDistributionBase_ = goodSettings[ironMetalworksIdx];
    distributionAdjusterBase_ = goodSettings;
    owner_.aii.ChangeDistribution(goodSettings);
}

void AIEconomyController::CheckForester()
{
    const std::list<nobUsual*>& foresters = owner_.aii.GetBuildings(BuildingType::Forester);
    if(!foresters.empty() && foresters.size() < 2 && owner_.aii.GetMilitaryBuildings().size() < 3
       && owner_.aii.GetBuildingSites().size() < 3)
    {
        if(!(*foresters.begin())->IsProductionDisabled())
            owner_.aii.SetProductionEnabled(foresters.front()->GetPos(), false);
    } else
    {
        if(!foresters.empty() && (*foresters.begin())->IsProductionDisabled())
            owner_.aii.SetProductionEnabled(foresters.front()->GetPos(), true);
    }
}

void AIEconomyController::CheckGraniteMine()
{
    const bool enableProduction = AmountInStorage(GoodType::Stones) < 100
                                  || AmountInStorage(GoodType::Stones) < 15 * owner_.aii.GetStorehouses().size();
    for(const nobUsual* mine : owner_.aii.GetBuildings(BuildingType::GraniteMine))
    {
        if(mine->IsProductionDisabled() == enableProduction)
            owner_.aii.SetProductionEnabled(mine->GetPos(), enableProduction);
    }
}

void AIEconomyController::ExecuteLuaConstructionOrder(const MapPoint pt, BuildingType bt, bool forced)
{
    if(!owner_.aii.CanBuildBuildingtype(bt))
        return;
    if(forced)
    {
        owner_.aii.SetBuildingSite(pt, bt);
        auto job = std::make_unique<BuildJob>(owner_, bt, pt);
        job->SetState(JobState::ExecutingRoad1);
        job->SetTarget(pt);
        owner_.construction->AddBuildJob(std::move(job), true);
    } else
    {
        if(owner_.construction->Wanted(bt))
            owner_.construction->AddBuildJob(std::make_unique<BuildJob>(owner_, bt, pt), true);
    }
}

unsigned AIEconomyController::AmountInStorage(const GoodType good) const
{
    unsigned counter = 0;
    for(const nobBaseWarehouse* wh : owner_.aii.GetStorehouses())
        counter += wh->GetInventory().goods[good];
    return counter;
}

unsigned AIEconomyController::AmountInStorage(const Job job) const
{
    unsigned counter = 0;
    for(const nobBaseWarehouse* wh : owner_.aii.GetStorehouses())
        counter += wh->GetInventory().people[job];
    return counter;
}

void AIEconomyController::AdjustSettings()
{
    const Inventory& inventory = owner_.aii.GetInventory();
    if(owner_.bldPlanner->GetNumBuildings(BuildingType::Metalworks) > 0u)
    {
        ToolSettings toolsettings{};
        const auto& toolPriority = owner_.config_.toolPriority;
        const auto calcToolPriority = [&](const Tool tool) {
            const GoodType good = TOOL_TO_GOOD[tool];
            unsigned numToolsAvailable = inventory[good];
            for(const auto job : helpers::enumRange<Job>())
            {
                if(JOB_CONSTS[job].tool != good)
                    continue;
                unsigned numBuildingsRequiringWorker = 0;
                for(const auto bld : helpers::enumRange<BuildingType>())
                {
                    if(BLD_WORK_DESC[bld].job == job)
                        numBuildingsRequiringWorker += owner_.bldPlanner->GetNumBuildings(bld);
                }
                const signed requiredTools = static_cast<signed>(numBuildingsRequiringWorker) - inventory[job];
                if(requiredTools > 0)
                {
                    if(requiredTools > static_cast<signed>(numToolsAvailable))
                        return toolPriority[tool];
                    numToolsAvailable -= requiredTools;
                }
            }
            return 0;
        };
        bool has_tool_shortage = false;
        for(const auto tool : helpers::enumRange<Tool>())
        {
            toolsettings[tool] = calcToolPriority(tool);
            has_tool_shortage = has_tool_shortage || toolsettings[tool] > 0;
        }

        if(!has_tool_shortage)
        {
            bool all_meet_basis = true;
            for(const auto tool : helpers::enumRange<Tool>())
            {
                const GoodType good = TOOL_TO_GOOD[tool];
                if(inventory[good] < static_cast<unsigned>(TOOL_BASIS[tool]))
                {
                    toolsettings[tool] = toolPriority[tool];
                    all_meet_basis = false;
                } else
                {
                    toolsettings[tool] = 0;
                }
            }
            if(all_meet_basis)
            {
                for(const auto tool : helpers::enumRange<Tool>())
                    toolsettings[tool] = toolPriority[tool];
            }
        }

        for(const auto tool : helpers::enumRange<Tool>())
        {
            if(toolsettings[tool] != owner_.player.GetToolPriority(tool))
            {
                ToolPriorityEventLogger::LogToolPriorityChanges(owner_.currentGF_, owner_.playerId, toolsettings);
                owner_.aii.ChangeTools(toolsettings);
                break;
            }
        }
    }

    AdjustDistribution();

    MilitarySettings milSettings;
    milSettings[0] = 10;
    milSettings[1] = owner_.HasFrontierBuildings() ? 5 : 0;
    milSettings[2] = 4;
    milSettings[3] = 5;
    milSettings[4] = UpdateUpgradeBuilding() >= 0
                         && (inventory[GoodType::Coins] > 0
                             || (inventory[GoodType::Gold] > 0 && inventory[GoodType::Coal] > 0
                                 && !owner_.aii.GetBuildings(BuildingType::Mint).empty())) ?
                       8 :
                       0;
    milSettings[6] = owner_.ggs.isEnabled(AddonId::SEA_ATTACK) ? 8 : 0;
    milSettings[5] = CalcMilSettings();
    milSettings[7] = 8;
    if(owner_.player.GetMilitarySetting(5) != milSettings[5] || owner_.player.GetMilitarySetting(6) != milSettings[6]
       || owner_.player.GetMilitarySetting(4) != milSettings[4] || owner_.player.GetMilitarySetting(1) != milSettings[1])
    {
        owner_.aii.ChangeMilitary(milSettings);
    }
}

void AIEconomyController::AdjustDistribution()
{
    const bool adjustMetalworks =
      owner_.bldPlanner->GetNumBuildings(BuildingType::Metalworks) > 0u && metalworksIronDistributionBase_ > 0u;

    const Inventory& inventory = owner_.aii.GetInventory();
    VisualSettings visualSettings{};
    owner_.player.FillVisualSettings(visualSettings);
    bool hasChanges = false;

    if(adjustMetalworks)
    {
        int min_surplus = std::numeric_limits<int>::max();
        for(const auto tool : helpers::enumRange<Tool>())
        {
            const GoodType good = TOOL_TO_GOOD[tool];
            const int surplus = static_cast<int>(inventory[good]) - TOOL_BASIS[tool];
            min_surplus = std::min(min_surplus, surplus);
        }

        const std::size_t idx = GetIronMetalworksDistributionIndex();
        const uint8_t currentPriority = visualSettings.distribution[idx];
        uint8_t newPriority = metalworksIronDistributionBase_;
        if(min_surplus >= 0)
        {
            const int decrease = 2 + min_surplus * 2;
            newPriority =
              static_cast<uint8_t>(std::max(0, static_cast<int>(metalworksIronDistributionBase_) - decrease));
        }
        if(newPriority != currentPriority)
        {
            visualSettings.distribution[idx] = newPriority;
            hasChanges = true;
        }
    }

    for(std::size_t idx = 0; idx < distributionMap.size(); ++idx)
    {
        const auto& mapping = distributionMap[idx];
        const GoodType distributedGood = std::get<0>(mapping);
        const BuildingType targetBuilding = std::get<1>(mapping);

        if(distributedGood == GoodType::Iron && targetBuilding == BuildingType::Metalworks)
            continue;
        if(owner_.bldPlanner->GetNumBuildings(targetBuilding) == 0u)
            continue;

        const DistributionParams& params = owner_.config_.distributionParams[distributedGood][targetBuilding];
        if(!params.enabled)
            continue;

        double rawDelta = 0.0;
        bool hasPenalty = false;
        for(const auto good : helpers::enumRange<GoodType>())
        {
            const BuildParams penalty = params.overstockingPenalty[good];
            if(!penalty.enabled)
                continue;
            hasPenalty = true;
            if(inventory[good] <= penalty.min)
                continue;

            const unsigned overstock = inventory[good] - penalty.min;
            const double value = CALC::calcCount(overstock, penalty);
            rawDelta += std::min<double>(value, penalty.max);
        }
        if(!hasPenalty)
            continue;

        const uint8_t currentPriority = visualSettings.distribution[idx];
        const int priorityDelta = static_cast<int>(rawDelta);
        const uint8_t newPriority = static_cast<uint8_t>(
          std::clamp(static_cast<int>(distributionAdjusterBase_[idx]) + priorityDelta, 0, 10));
        if(newPriority != currentPriority)
        {
            visualSettings.distribution[idx] = newPriority;
            hasChanges = true;
        }
    }

    if(hasChanges)
        owner_.aii.ChangeDistribution(visualSettings.distribution);
}

unsigned AIEconomyController::CalcMilSettings()
{
    std::array<unsigned, 5> InlandTroops = {0, 0, 0, 0, 0};
    unsigned numSoldiers = 0;
    for(const auto i : SOLDIER_JOBS)
        numSoldiers += owner_.aii.GetInventory().people[i];

    const unsigned numShouldStayConnected = owner_.GetNumPlannedConnectedInlandMilitaryBlds();
    int count = 0;
    unsigned soldierInUseFixed = 0;
    const int uun = UpdateUpgradeBuilding();
    const std::list<nobMilitary*>& militaryBuildings = owner_.aii.GetMilitaryBuildings();
    for(const nobMilitary* milBld : militaryBuildings)
    {
        if(milBld->GetFrontierDistance() == FrontierDistance::Near
           || milBld->GetFrontierDistance() == FrontierDistance::Harbor
           || (milBld->GetFrontierDistance() == FrontierDistance::Far
               && (militaryBuildings.size() < static_cast<unsigned>(count) + numShouldStayConnected || count == uun)))
        {
            soldierInUseFixed += milBld->CalcRequiredNumTroops(FrontierDistance::Mid, 8);
        }
        else if(milBld->GetFrontierDistance() == FrontierDistance::Mid)
        {
            for(int i = 0; i < 5; i++)
                InlandTroops[i] += milBld->CalcRequiredNumTroops(FrontierDistance::Mid, 4 + i);
        }
        else
        {
            soldierInUseFixed++;
        }

        count++;
    }

    unsigned returnValue = 8;
    while(returnValue > 4)
    {
        if(soldierInUseFixed + InlandTroops[returnValue - 4] < numSoldiers * 10 / 11
           || (owner_.player.GetMilitarySetting(5) >= returnValue
               && soldierInUseFixed + InlandTroops[returnValue - 4] < numSoldiers))
        {
            break;
        }
        returnValue--;
    }
    return returnValue;
}

void AIPlayerJH::PlanNewBuildings(const unsigned gf) { economyController_->PlanNewBuildings(gf); }

nobBaseWarehouse* AIPlayerJH::GetUpgradeBuildingWarehouse() { return economyController_->GetUpgradeBuildingWarehouse(); }

void AIPlayerJH::AddMilitaryBuildJob(MapPoint pt) { economyController_->AddMilitaryBuildJob(pt); }

void AIPlayerJH::AddGlobalBuildJob(BuildingType type) { economyController_->AddGlobalBuildJob(type); }

void AIPlayerJH::AddBuildJob(BuildingType type, const MapPoint pt, bool front, bool searchPosition)
{
    economyController_->AddBuildJob(type, pt, front, searchPosition);
}

MapPoint AIPlayerJH::FindBestPosition(BuildingType bt) { return economyController_->FindBestPosition(bt); }

void AIPlayerJH::AddBuildJobAroundEveryWarehouse(BuildingType bt)
{
    economyController_->AddBuildJobAroundEveryWarehouse(bt);
}

void AIPlayerJH::AddBuildJobAroundEveryMilBld(BuildingType bt)
{
    economyController_->AddBuildJobAroundEveryMilBld(bt);
}

void AIPlayerJH::SetSendingForUpgradeWarehouse(nobBaseWarehouse* upgradewarehouse)
{
    economyController_->SetSendingForUpgradeWarehouse(upgradewarehouse);
}

void AIPlayerJH::SetGatheringForUpgradeWarehouse(nobBaseWarehouse* upgradewarehouse)
{
    economyController_->SetGatheringForUpgradeWarehouse(upgradewarehouse);
}

void AIPlayerJH::DistributeGoodsByBlocking(const GoodType good, unsigned limit)
{
    economyController_->DistributeGoodsByBlocking(good, limit);
}

void AIPlayerJH::DistributeMaxRankSoldiersByBlocking(unsigned limit, nobBaseWarehouse* upwh)
{
    economyController_->DistributeMaxRankSoldiersByBlocking(limit, upwh);
}

unsigned AIPlayerJH::SoldierAvailable(int rank) { return economyController_->SoldierAvailable(rank); }

void AIPlayerJH::InitStoreAndMilitarylists() { economyController_->InitStoreAndMilitarylists(); }

int AIPlayerJH::UpdateUpgradeBuilding() { return economyController_->UpdateUpgradeBuilding(); }

void AIPlayerJH::InitDistribution() { economyController_->InitDistribution(); }

void AIPlayerJH::CheckForester() { economyController_->CheckForester(); }

void AIPlayerJH::CheckGraniteMine() { economyController_->CheckGraniteMine(); }

void AIPlayerJH::ExecuteLuaConstructionOrder(const MapPoint pt, BuildingType bt, bool forced)
{
    economyController_->ExecuteLuaConstructionOrder(pt, bt, forced);
}

unsigned AIPlayerJH::AmountInStorage(GoodType good) const { return economyController_->AmountInStorage(good); }

unsigned AIPlayerJH::AmountInStorage(::Job job) const { return economyController_->AmountInStorage(job); }

const helpers::EnumArray<unsigned, GoodType>& AIPlayerJH::GetProducedGoods() const
{
    return economyController_->GetProducedGoods();
}

void AIPlayerJH::AdjustSettings() { economyController_->AdjustSettings(); }

void AIPlayerJH::AdjustDistribution() { economyController_->AdjustDistribution(); }

unsigned AIPlayerJH::CalcMilSettings() { return economyController_->CalcMilSettings(); }

} // namespace AIJH
