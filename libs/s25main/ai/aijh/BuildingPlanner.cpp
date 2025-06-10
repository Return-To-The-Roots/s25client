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

    buildingsWanted[BuildingType::Woodcutter] = calculator.Calc(BuildingType::Woodcutter);
    buildingsWanted[BuildingType::Forester] = calculator.Calc(BuildingType::Forester);
    buildingsWanted[BuildingType::Quarry] = calculator.Calc(BuildingType::Quarry);
    buildingsWanted[BuildingType::Well] = calculator.Calc(BuildingType::Well);
    buildingsWanted[BuildingType::Sawmill] = calculator.Calc(BuildingType::Sawmill);
    buildingsWanted[BuildingType::Mill] = calculator.Calc(BuildingType::Mill);
    buildingsWanted[BuildingType::Bakery] = calculator.Calc(BuildingType::Bakery);
    buildingsWanted[BuildingType::Ironsmelter] = calculator.Calc(BuildingType::Ironsmelter);
    buildingsWanted[BuildingType::Armory] = calculator.Calc(BuildingType::Armory);
    buildingsWanted[BuildingType::Metalworks] = calculator.Calc(BuildingType::Metalworks);
    buildingsWanted[BuildingType::Brewery] = calculator.Calc(BuildingType::Brewery);
    buildingsWanted[BuildingType::IronMine] = calculator.Calc(BuildingType::IronMine);
    buildingsWanted[BuildingType::CoalMine] = calculator.Calc(BuildingType::CoalMine);
    buildingsWanted[BuildingType::DonkeyBreeder] = calculator.Calc(BuildingType::DonkeyBreeder);
    buildingsWanted[BuildingType::PigFarm] = calculator.Calc(BuildingType::PigFarm);
    buildingsWanted[BuildingType::Slaughterhouse] = calculator.Calc(BuildingType::Slaughterhouse);
    buildingsWanted[BuildingType::Farm] = calculator.Calc(BuildingType::Farm);

    buildingsWanted[BuildingType::Mint] = GetNumBuildings(BuildingType::GoldMine);

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
        // coal mine count now depends on iron & gold not linked to food or material supply - might have to add a
        // material check if this makes problems
        // if(GetNumBuildings(BuildingType::IronMine) > 0)
        //     buildingsWanted[BuildingType::CoalMine] =
        //       GetNumBuildings(BuildingType::IronMine) * 2 - 1 + GetNumBuildings(BuildingType::GoldMine);
        // else
        //     buildingsWanted[BuildingType::CoalMine] = std::max(GetNumBuildings(BuildingType::GoldMine), 1u);
        // // more mines planned than food available? -> limit mines
        // if(buildingsWanted[BuildingType::CoalMine] > 2
        //    && buildingsWanted[BuildingType::CoalMine] * 2
        //         > GetNumBuildings(BuildingType::Farm) + GetNumBuildings(BuildingType::Fishery) + 1)
        //     buildingsWanted[BuildingType::CoalMine] =
        //       (GetNumBuildings(BuildingType::Farm) + GetNumBuildings(BuildingType::Fishery)) / 2 + 2;
        //
        // if(GetNumBuildings(BuildingType::Farm) > 7) // quite the empire just scale mines with farms
        // {
        //     if(aijh.ggs.isEnabled(AddonId::INEXHAUSTIBLE_MINES))
        //     {
        //         buildingsWanted[BuildingType::CoalMine] = unsigned(GetNumBuildings(BuildingType::IronMine) * 2);
        //     }
        //
        //     buildingsWanted[BuildingType::DonkeyBreeder] = 1;
        //     if(aijh.ggs.isEnabled(AddonId::CHARBURNER)
        //        && (buildingsWanted[BuildingType::CoalMine] > GetNumBuildings(BuildingType::CoalMine) + 4))
        //     {
        //         int resourcelimit = inventory.people[Job::CharBurner] + inventory.goods[GoodType::Shovel] + 1;
        //         buildingsWanted[BuildingType::Charburner] = std::min<unsigned>(
        //           std::min(buildingsWanted[BuildingType::CoalMine] - (GetNumBuildings(BuildingType::CoalMine) + 1),
        //                    3u),
        //           resourcelimit);
        //     }
        // } else
        // {
        //     // buildingsWanted[BuildingType::GoldMine] = (inventory.people[Job::Miner] > 2) ? 1 : 0;
        //     int resourcelimit = inventory.people[Job::CharBurner] + inventory.goods[GoodType::Shovel];
        //     if(aijh.ggs.isEnabled(AddonId::CHARBURNER)
        //        && (GetNumBuildings(BuildingType::CoalMine) < 1
        //            && (GetNumBuildings(BuildingType::IronMine) + GetNumBuildings(BuildingType::GoldMine) > 0)))
        //         buildingsWanted[BuildingType::Charburner] = std::min(1, resourcelimit);
        // }
        // unsigned quarries = GetNumBuildings(BuildingType::Quarry);
        // if(quarries < 6 && quarries + 1 < buildingsWanted[BuildingType::Quarry]
        //    && aijh.AmountInStorage(GoodType::Stones) < 100) // no quarry and low stones -> try granitemines.
        // {
        //     buildingsWanted[BuildingType::GraniteMine] = std::min<int>(
        //       numMilitaryBlds / 8 + 1, // limit granite mines to military / 10
        //       std::max<int>(buildingsWanted[BuildingType::Quarry] - GetNumBuildings(BuildingType::Quarry), 1));
        // } else
        //     buildingsWanted[BuildingType::GraniteMine] = 0;
    }

    // buildingsWanted[BuildingType::IronMine] = calculator.CalcIronMines();

    // Only build catapults if we have more than 5 military blds and 50 stones. Then reserve 4 stones per catapult
    // but do not build more than 4 additions catapults Note: This is a max amount. A catapult is only placed if
    // reasonable
    buildingsWanted[BuildingType::Catapult] =
      numMilitaryBlds <= 5 || inventory.goods[GoodType::Stones] < 50 ?
        0 :
        std::min((inventory.goods[GoodType::Stones] - 50) / 4, GetNumBuildings(BuildingType::Catapult) + 4);
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