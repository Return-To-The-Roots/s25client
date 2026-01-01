// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AIPlayerJH.h"

#include "BuildingPlanner.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "RTTR_Assert.h"
#include "addons/const_addons.h"
#include "buildings/nobMilitary.h"
#include "gameData/BuildingConsts.h"
#include "gameData/BuildingProperties.h"
#include "gameData/JobConsts.h"
#include "gameData/MilitaryConsts.h"
#include "gameData/ToolConsts.h"
#include "gameTypes/VisualSettings.h"
#include "helpers/EnumRange.h"

#include <algorithm>
#include <array>
#include <list>
#include <limits>

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

void AIPlayerJH::AdjustSettings()
{
    const Inventory& inventory = aii.GetInventory();
    if(bldPlanner->GetNumBuildings(BuildingType::Metalworks) > 0u)
    {
        ToolSettings toolsettings{};
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
                        numBuildingsRequiringWorker += bldPlanner->GetNumBuildings(bld);
                }
                const signed requiredTools = static_cast<signed>(numBuildingsRequiringWorker) - inventory[job];
                if(requiredTools > 0)
                {
                    if(requiredTools > static_cast<signed>(numToolsAvailable))
                        return TOOL_PRIORITY[tool];
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
                    toolsettings[tool] = TOOL_PRIORITY[tool];
                    all_meet_basis = false;
                }
                else
                {
                    toolsettings[tool] = 0;
                }
            }
            if(all_meet_basis)
            {
                for(const auto tool : helpers::enumRange<Tool>())
                    toolsettings[tool] = TOOL_PRIORITY[tool];
            }
        }

        for(const auto tool : helpers::enumRange<Tool>())
        {
            if(toolsettings[tool] != player.GetToolPriority(tool))
            {
                aii.ChangeTools(toolsettings);
                break;
            }
        }
    }

    AdjustDistribution();

    MilitarySettings milSettings;
    milSettings[0] = 10;
    milSettings[1] = HasFrontierBuildings() ? 5 : 0;
    milSettings[2] = 4;
    milSettings[3] = 5;
    milSettings[4] = UpdateUpgradeBuilding() >= 0
                         && (inventory[GoodType::Coins] > 0
                             || (inventory[GoodType::Gold] > 0 && inventory[GoodType::Coal] > 0
                                 && !aii.GetBuildings(BuildingType::Mint).empty())) ?
                       8 :
                       0;
    milSettings[6] = ggs.isEnabled(AddonId::SEA_ATTACK) ? 8 : 0;
    milSettings[5] = CalcMilSettings();
    milSettings[7] = 8;
    if(player.GetMilitarySetting(5) != milSettings[5] || player.GetMilitarySetting(6) != milSettings[6]
       || player.GetMilitarySetting(4) != milSettings[4] || player.GetMilitarySetting(1) != milSettings[1])
        aii.ChangeMilitary(milSettings);
}

void AIPlayerJH::AdjustDistribution()
{
    if(bldPlanner->GetNumBuildings(BuildingType::Metalworks) == 0u || metalworksIronDistributionBase_ == 0u)
        return;

    const Inventory& inventory = aii.GetInventory();
    int min_surplus = std::numeric_limits<int>::max();
    for(const auto tool : helpers::enumRange<Tool>())
    {
        const GoodType good = TOOL_TO_GOOD[tool];
        const int surplus = static_cast<int>(inventory[good]) - TOOL_BASIS[tool];
        min_surplus = std::min(min_surplus, surplus);
    }

    const std::size_t idx = GetIronMetalworksDistributionIndex();
    VisualSettings visualSettings{};
    player.FillVisualSettings(visualSettings);
    const uint8_t currentPriority = visualSettings.distribution[idx];

    uint8_t newPriority = metalworksIronDistributionBase_;
    if(min_surplus >= 0)
    {
        const int decrease = 2 + min_surplus * 2;
        newPriority = static_cast<uint8_t>(
          std::max(0, static_cast<int>(metalworksIronDistributionBase_) - decrease));
    }

    if(newPriority == currentPriority)
        return;

    visualSettings.distribution[idx] = newPriority;
    aii.ChangeDistribution(visualSettings.distribution);
}

unsigned AIPlayerJH::CalcMilSettings()
{
    std::array<unsigned, 5> InlandTroops = {0, 0, 0, 0, 0};
    unsigned numSoldiers = 0;
    for(auto i : SOLDIER_JOBS)
        numSoldiers += aii.GetInventory().people[i];

    const unsigned numShouldStayConnected = GetNumPlannedConnectedInlandMilitaryBlds();
    int count = 0;
    unsigned soldierInUseFixed = 0;
    const int uun = UpdateUpgradeBuilding();
    const std::list<nobMilitary*>& militaryBuildings = aii.GetMilitaryBuildings();
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
            soldierInUseFixed++;

        count++;
    }

    unsigned returnValue = 8;
    while(returnValue > 4)
    {
        if(soldierInUseFixed + InlandTroops[returnValue - 4] < numSoldiers * 10 / 11
           || (player.GetMilitarySetting(5) >= returnValue
               && soldierInUseFixed + InlandTroops[returnValue - 4] < numSoldiers))
            break;
        returnValue--;
    }
    return returnValue;
}

} // namespace AIJH
