// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "BuildingPlanner.h"
#include "AIConfig.h"
#include "AIPlayerJH.h"
#include "BuildingCalculator.h"
#include "GlobalGameSettings.h"
#include "PlannerHelper.h"
#include "addons/const_addons.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobMilitary.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/GoodTypes.h"
#include "gameData/BuildingProperties.h"
#include <algorithm>
#include <cmath>

namespace AIJH {
BuildingPlanner::BuildingPlanner(const AIPlayerJH& aijh) : buildingsWanted(), expansionRequired(false)
{
    RefreshBuildingNums(aijh);
    InitBuildingsWanted(aijh);
    UpdateBuildingsWanted(aijh);
}

void BuildingPlanner::Update(unsigned gf, AIPlayerJH& aijh)
{
    RefreshBuildingNums(aijh);
    expansionRequired = CalcIsExpansionRequired(aijh, gf > 500 && gf % 500 == 0);

    signed boardsDemand = aijh.player.GetBuildingRegister().CalcBoardsDemand();
    boardsBalance = aijh.player.GetInventory().goods[GoodType::Boards] - boardsDemand;
    if(gf % 2500 == 0)
    {
        woodAvailable = aijh.GetAvailableResources(AISurfaceResource::Wood);
        stoneAvailable = aijh.GetAvailableResources(AISurfaceResource::Stones);
    }
}

void BuildingPlanner::RefreshBuildingNums(const AIPlayerJH& aijh)
{
    buildingNums = aijh.player.GetBuildingRegister().GetBuildingNums();
}

bool BuildingPlanner::CalcIsExpansionRequired(AIPlayerJH& aijh, bool recalc) const
{
    if(!expansionRequired && !recalc)
        return false;
    if(GetNumMilitaryBldSites() > 0 || GetNumMilitaryBlds() > 4)
        return false;
    bool hasWood = GetNumBuildings(BuildingType::Woodcutter) > 0;
    bool hasBoards = GetNumBuildings(BuildingType::Sawmill) > 0;
    bool hasStone = GetNumBuildings(BuildingType::Quarry) > 0;
    if(expansionRequired)
    {
        if(hasWood && hasBoards && hasStone)
            return false;
    } else
    {
        // Check if we could build the missing building around any military building or storehouse. If not, expand.
        const BuildingRegister& buildingRegister = aijh.player.GetBuildingRegister();
        std::vector<noBuilding*> blds(buildingRegister.GetMilitaryBuildings().begin(),
                                      buildingRegister.GetMilitaryBuildings().end());
        blds.insert(blds.end(), buildingRegister.GetStorehouses().begin(), buildingRegister.GetStorehouses().end());
        for(const noBuilding* bld : blds)
        {
            if(!hasWood)
                hasWood = aijh.FindPositionForBuildingAround(BuildingType::Woodcutter, bld->GetPos()).isValid();
            if(!hasBoards)
                hasBoards = aijh.FindPositionForBuildingAround(BuildingType::Sawmill, bld->GetPos()).isValid();
            if(!hasStone)
                hasStone = aijh.FindPositionForBuildingAround(BuildingType::Quarry, bld->GetPos()).isValid();
        }
        return !(hasWood && hasBoards && hasStone);
    }
    return expansionRequired;
}

unsigned BuildingPlanner::GetNumBuildings(BuildingType type) const
{
    return buildingNums.buildings[type] + buildingNums.buildingSites[type];
}

unsigned BuildingPlanner::GetNumBuildingSites(BuildingType type) const
{
    return buildingNums.buildingSites[type];
}

unsigned BuildingPlanner::GetNumMilitaryBlds() const
{
    unsigned result = 0;
    for(BuildingType bld : BuildingProperties::militaryBldTypes)
        result += GetNumBuildings(bld);
    return result;
}

unsigned BuildingPlanner::GetNumMilitaryBldSites() const
{
    unsigned result = 0;
    for(BuildingType bld : BuildingProperties::militaryBldTypes)
        result += GetNumBuildingSites(bld);
    return result;
}
unsigned BuildingPlanner::GetNumBuildingsWanted(BuildingType type) const
{
    return buildingsWanted[type];
}

void BuildingPlanner::InitBuildingsWanted(const AIPlayerJH& aijh)
{
    std::fill(buildingsWanted.begin(), buildingsWanted.end(), 0u);
    buildingsWanted[BuildingType::Forester] = 0;
    buildingsWanted[BuildingType::Sawmill] = 0;
    buildingsWanted[BuildingType::Woodcutter] = 0;
    buildingsWanted[BuildingType::GraniteMine] = 0;
    buildingsWanted[BuildingType::CoalMine] = 0;
    buildingsWanted[BuildingType::IronMine] = 0;
    buildingsWanted[BuildingType::GoldMine] = 0;
    buildingsWanted[BuildingType::Catapult] = 0;
    buildingsWanted[BuildingType::Fishery] = 0;
    buildingsWanted[BuildingType::Quarry] = 0;
    buildingsWanted[BuildingType::Hunter] = 0;
    buildingsWanted[BuildingType::Farm] = 0;
    // aijh.player.GetInventory().goods[GoodType::Scythe] + aijh.player.GetInventory().people[Job::Farmer];

    unsigned numAIRelevantSeaIds = aijh.GetNumAIRelevantSeaIds();
    if(numAIRelevantSeaIds > 0)
    {
        buildingsWanted[BuildingType::HarborBuilding] = 99;
        buildingsWanted[BuildingType::Shipyard] = numAIRelevantSeaIds == 1 ? 1 : 99;
    }
}

void BuildingPlanner::UpdateBuildingsWanted(const AIPlayerJH& aijh)
{
    const Inventory& inventory = aijh.player.GetInventory();
    BuildCalculator calculator = BuildCalculator(aijh, buildingNums, inventory, woodAvailable);

    // no military buildings -> usually start only
    const unsigned numMilitaryBlds = aijh.player.GetBuildingRegister().GetMilitaryBuildings().size();

    if(numMilitaryBlds == 0 && GetNumMilitaryBldSites() == 0)
    {
        return;
    }
    // std::size_t storehouses = aijh.player.GetBuildingRegister().GetStorehouses().size();

    // buildingsWanted[BuildingType::Forester] = calculator.CalcForesters();

    // fishery & hunter
    buildingsWanted[BuildingType::Fishery] = std::min(maxFishers(aijh), numMilitaryBlds + 1u);
    buildingsWanted[BuildingType::Hunter] = std::min(maxHunters(aijh), 4u);

    // Iterate through all building types and calculate wanted amounts
    for (const auto bldType : helpers::enumRange<BuildingType>()) {
        // Skip special handling building types
        if (bldType == BuildingType::Fishery || bldType == BuildingType::Hunter || 
            bldType == BuildingType::HarborBuilding || bldType == BuildingType::Shipyard ||
            bldType == BuildingType::Catapult || bldType == BuildingType::Mint ||
            bldType == BuildingType::GraniteMine || bldType == BuildingType::Charburner)
            continue;

        // Special handling for Mint
        if (bldType == BuildingType::Mint) {
            buildingsWanted[bldType] = GetNumBuildings(BuildingType::GoldMine);
            continue;
        }

        // Special handling for Catapult
        if (bldType == BuildingType::Catapult) {
            // Only build catapults if we have more than 5 military blds and 50 stones. Then reserve 4 stones per catapult
            // but do not build more than 4 additions catapults Note: This is a max amount. A catapult is only placed if
            // reasonable
            buildingsWanted[bldType] = numMilitaryBlds <= 5 || inventory.goods[GoodType::Stones] < 50 ?
                0 :
                std::min((inventory.goods[GoodType::Stones] - 50) / 4, GetNumBuildings(BuildingType::Catapult) + 4);
            continue;
        }

        // Handle normal building types
        buildingsWanted[bldType] = calculator.Calc(bldType);
    }

    if(inventory.goods[GoodType::PickAxe] + inventory.people[Job::Miner] < 3)
    {
        // almost out of new pickaxes and miners - emergency program: get coal,iron,smelter&metalworks
        buildingsWanted[BuildingType::CoalMine] = 1;
        buildingsWanted[BuildingType::IronMine] = 1;
        buildingsWanted[BuildingType::GoldMine] = 0;
        buildingsWanted[BuildingType::Ironsmelter] = 1;
        buildingsWanted[BuildingType::Metalworks] = 1;
        buildingsWanted[BuildingType::Armory] = 0;
        buildingsWanted[BuildingType::GraniteMine] = 0;
        buildingsWanted[BuildingType::Mint] = 0;
    } else // more than 2 miners
    {
        buildingsWanted[BuildingType::IronMine] = calculator.Calc(BuildingType::IronMine);
        buildingsWanted[BuildingType::CoalMine] = calculator.Calc(BuildingType::CoalMine);
    }

    if(!IsGoldEnabled(aijh))
    {
        buildingsWanted[BuildingType::GoldMine] = 0; // max rank is 0 = private / recruit ==> gold is useless!
    }
}

int BuildingPlanner::GetNumAdditionalBuildingsWanted(BuildingType type) const
{
    return static_cast<int>(buildingsWanted[type]) - static_cast<int>(GetNumBuildings(type));
}

bool BuildingPlanner::IsGoldEnabled(const AIPlayerJH& aijh)
{
    return aijh.ggs.GetMaxMilitaryRank() == 0 || !aijh.ggs.isEnabled(AddonId::CHANGE_GOLD_DEPOSITS);
}

bool BuildingPlanner::WantMoreMilitaryBlds(const AIPlayerJH& aijh) const
{
    if(GetNumMilitaryBldSites() >= GetNumMilitaryBlds() + 3)
        return false;
    if(expansionRequired)
        return true;
    if(GetNumBuildings(BuildingType::Sawmill) > 0)
        return true;
    if(aijh.player.GetInventory().goods[GoodType::Boards] > 30 && GetNumBuildingSites(BuildingType::Sawmill) > 0)
        return true;
    return GetNumMilitaryBlds() + GetNumMilitaryBldSites() < 3;
}

void BuildingPlanner::setBuildingsWanted(helpers::EnumArray<unsigned, BuildingType> values)
{
    for(BuildingType type : helpers::EnumRange<BuildingType>{})
    {
        int value = values[type];
        if(value != 0)
        {
            buildingsWanted[type] = value;
        }
    }
}
} // namespace AIJH
