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
    if(gf % 5000 == 0)
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
    // no military buildings -> usually start only
    const unsigned numMilitaryBlds = aijh.player.GetBuildingRegister().GetMilitaryBuildings().size();

    if(numMilitaryBlds == 0 && GetNumMilitaryBldSites() == 0)
    {
        return;
    }
    std::size_t storehouses = aijh.player.GetBuildingRegister().GetStorehouses().size();

    if(numMilitaryBlds < AI_CONFIG.startupMilBuildings && storehouses < 2)
    {
        setBuildingsWanted(GetStartupSet(numMilitaryBlds, woodAvailable));
    } else
    {
        const Inventory& inventory = aijh.player.GetInventory();

        unsigned woodcutters = GetNumBuildings(BuildingType::Woodcutter);
        buildingsWanted[BuildingType::Forester] = CalcForesters(aijh, woodAvailable);

        unsigned woodcuttersWanted = buildingsWanted[BuildingType::Woodcutter];
        unsigned forestersActive = buildingNums.buildings[BuildingType::Forester];
        unsigned foresters = GetNumBuildings(BuildingType::Forester);
        unsigned baseWoodCutters = (unsigned)(AI_CONFIG.woodcutterToForesterRatio * foresters);
        unsigned additional_woodcutters = 0;
        unsigned extraWoodCutters = woodcuttersWanted - baseWoodCutters;
        unsigned extraWood = woodAvailable - 6 * forestersActive;
        if((extraWood > 40 && extraWoodCutters < 2) || (woodcuttersWanted > 0 && extraWood / woodcuttersWanted > 20))
        {
            additional_woodcutters = std::min((unsigned)(woodcutters * 0.4), extraWood / 20);
        }

        // unsigned sawmills = GetNumBuildings(BuildingType::Sawmill);
        unsigned max_available_woodcutter = maxWoodcutter(aijh);
        buildingsWanted[BuildingType::Woodcutter] =
          std::min(max_available_woodcutter, additional_woodcutters + baseWoodCutters);

        // fishery & hunter
        buildingsWanted[BuildingType::Fishery] = std::min(maxFishers(aijh), numMilitaryBlds + 1u);
        buildingsWanted[BuildingType::Hunter] = std::min(maxHunters(aijh), 4u);

        // quarry: low ware games start at 2 otherwise build as many as we have stonemasons, higher ware games up to 6
        // quarries
        if(inventory.goods[GoodType::PickAxe] + inventory.people[Job::Miner] < 7
           && inventory.people[Job::Stonemason] > 0 && inventory.people[Job::Miner] < 3)
        {
            buildingsWanted[BuildingType::Quarry] =
              std::max(std::min(inventory.people[Job::Stonemason], numMilitaryBlds), 2u);
        } else
        {
            //>6miners = build up to 6 depending on resources, else max out at miners/2
            if(inventory.people[Job::Miner] > 6)
                buildingsWanted[BuildingType::Quarry] =
                  std::min(inventory.goods[GoodType::PickAxe] + inventory.people[Job::Stonemason], 6u);
            else
                buildingsWanted[BuildingType::Quarry] = inventory.people[Job::Miner] / 2;

            if(buildingsWanted[BuildingType::Quarry] > numMilitaryBlds)
                buildingsWanted[BuildingType::Quarry] = numMilitaryBlds;
        }
        buildingsWanted[BuildingType::Quarry] =
          std::max(buildingsWanted[BuildingType::Quarry], (unsigned)(GetNumBuildings(BuildingType::Sawmill) / 1.5 + 1));
        // sawmills limited by woodcutters and carpenter+saws reduced by char burners minimum of 3
        int resourcelimit = inventory.people[Job::Carpenter] + inventory.goods[GoodType::Saw];
        // int numSawmillsFed = (static_cast<int>(woodcutters) -
        // static_cast<int>(GetNumBuildings(BuildingType::Charburner) * 2)) / 2;
        const auto milToSaw = AI_CONFIG.milToSawmill;
        buildingsWanted[BuildingType::Sawmill] =
          (unsigned)(milToSaw.constant + log2(milToSaw.logTwo.constant + milToSaw.logTwo.linear * numMilitaryBlds));
        // 4 + std::max(std::min(numSawmillsFed, resourcelimit), 3); // min 3

        buildingsWanted[BuildingType::Ironsmelter] =
          (unsigned)(GetNumBuildings(BuildingType::IronMine) * AI_CONFIG.ironsmelterToIronMineRatio) + 1;

        buildingsWanted[BuildingType::Mint] = GetNumBuildings(BuildingType::GoldMine);
        // armory count = smelter -metalworks if there is more than 1 smelter or 1 if there is just 1.
        unsigned ironsmelters = GetNumBuildings(BuildingType::Ironsmelter);
        unsigned metalworks = GetNumBuildings(BuildingType::Metalworks);
        buildingsWanted[BuildingType::Armory] =
          (ironsmelters > 1) ?
            std::max(0, static_cast<int>(ironsmelters - metalworks)) :
            ironsmelters;

        if(aijh.ggs.isEnabled(AddonId::HALF_COST_MIL_EQUIP))
            buildingsWanted[BuildingType::Armory] *= 2;
        // brewery count = 1+(armory/5) if there is at least 1 armory or armory /6 for exhaustible mines
        unsigned farms = buildingNums.buildings[BuildingType::Farm];
        if(GetNumBuildings(BuildingType::Armory) > 0 && farms > 2)
        {
            if(aijh.ggs.isEnabled(AddonId::INEXHAUSTIBLE_MINES))
                buildingsWanted[BuildingType::Brewery] =
                  1 + unsigned(GetNumBuildings(BuildingType::Armory) / AI_CONFIG.breweryToArmoryRatio);
            else
                buildingsWanted[BuildingType::Brewery] = 1 + GetNumBuildings(BuildingType::Armory) / 6;
        } else
            buildingsWanted[BuildingType::Brewery] = 0;

        // metalworks is 1 if there is at least 1 smelter, 2 if mines are inexhaustible and we have at least 4 iron
        // smelters
        if(aijh.ggs.isEnabled(AddonId::INEXHAUSTIBLE_MINES))
        {
            buildingsWanted[BuildingType::Metalworks] =
              (unsigned)(std::min((ironsmelters > 0) ? (5 + ironsmelters) / 6.0 : 0.0, AI_CONFIG.maxMetalworks));
        } else
        {
            buildingsWanted[BuildingType::Metalworks] = (ironsmelters > 0) ? 1 : 0;
        }
        unsigned millsNum = GetNumBuildings(BuildingType::Mill);
        // max processing
        unsigned foodusers = GetNumBuildings(BuildingType::Charburner) + millsNum
                             + GetNumBuildings(BuildingType::Brewery) + GetNumBuildings(BuildingType::PigFarm)
                             + GetNumBuildings(BuildingType::DonkeyBreeder);

            if(farms >= foodusers - millsNum)
            buildingsWanted[BuildingType::Mill] = std::min(
              unsigned(AI_CONFIG.millToFarmRatio * (farms - (foodusers - millsNum))),
              GetNumBuildings(BuildingType::Bakery) + 1);
        else
            buildingsWanted[BuildingType::Mill] = millsNum;

        buildingsWanted[BuildingType::Bakery] =
          std::min<unsigned>(millsNum, maxBakers(aijh) + 1);

        buildingsWanted[BuildingType::PigFarm] = CalcPigFarms(buildingNums);
        buildingsWanted[BuildingType::Slaughterhouse] =
          std::min(maxButcher(aijh), GetNumBuildings(BuildingType::PigFarm));

        buildingsWanted[BuildingType::Well] =
          buildingsWanted[BuildingType::Bakery] + buildingsWanted[BuildingType::PigFarm]
          + buildingsWanted[BuildingType::DonkeyBreeder] + buildingsWanted[BuildingType::Brewery];

        buildingsWanted[BuildingType::Farm] = CalcFarms(aijh, numMilitaryBlds);

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
            // coal mine count now depends on iron & gold not linked to food or material supply - might have to add a
            // material check if this makes problems
            if(GetNumBuildings(BuildingType::IronMine) > 0)
                buildingsWanted[BuildingType::CoalMine] =
                  GetNumBuildings(BuildingType::IronMine) * 2 - 1 + GetNumBuildings(BuildingType::GoldMine);
            else
                buildingsWanted[BuildingType::CoalMine] = std::max(GetNumBuildings(BuildingType::GoldMine), 1u);
            // more mines planned than food available? -> limit mines
            if(buildingsWanted[BuildingType::CoalMine] > 2
               && buildingsWanted[BuildingType::CoalMine] * 2
                    > GetNumBuildings(BuildingType::Farm) + GetNumBuildings(BuildingType::Fishery) + 1)
                buildingsWanted[BuildingType::CoalMine] =
                  (GetNumBuildings(BuildingType::Farm) + GetNumBuildings(BuildingType::Fishery)) / 2 + 2;
            if(GetNumBuildings(BuildingType::Farm) > 7) // quite the empire just scale mines with farms
            {
                if(aijh.ggs.isEnabled(
                     AddonId::INEXHAUSTIBLE_MINES)) // inexhaustible mines? -> more farms required for each mine
                    buildingsWanted[BuildingType::IronMine] =
                      unsigned(GetNumBuildings(BuildingType::Farm) / AI_CONFIG.farmToIronMineRatio);
                else
                    buildingsWanted[BuildingType::IronMine] =
                      std::min(GetNumBuildings(BuildingType::Farm) / 2, GetNumBuildings(BuildingType::Ironsmelter) + 1);
                buildingsWanted[BuildingType::GoldMine] =
                  (GetNumBuildings(BuildingType::Mint) > 0) ?
                    GetNumBuildings(BuildingType::Ironsmelter) > 6 && GetNumBuildings(BuildingType::Mint) > 1 ?
                    GetNumBuildings(BuildingType::Ironsmelter) > 10 ? 5 : 4 :
                    3 :
                    2;
                buildingsWanted[BuildingType::DonkeyBreeder] = 1;
                if(aijh.ggs.isEnabled(AddonId::CHARBURNER)
                   && (buildingsWanted[BuildingType::CoalMine] > GetNumBuildings(BuildingType::CoalMine) + 4))
                {
                    resourcelimit = inventory.people[Job::CharBurner] + inventory.goods[GoodType::Shovel] + 1;
                    buildingsWanted[BuildingType::Charburner] = std::min<unsigned>(
                      std::min(buildingsWanted[BuildingType::CoalMine] - (GetNumBuildings(BuildingType::CoalMine) + 1),
                               3u),
                      resourcelimit);
                }
            } else
            {
                // probably still limited in food supply go up to 4 coal 1 gold 2 iron (gold+coal->coin,
                // iron+coal->tool, iron+coal+coal->weapon)
                unsigned numFoodProducers =
                  GetNumBuildings(BuildingType::Bakery) + GetNumBuildings(BuildingType::Slaughterhouse)
                  + GetNumBuildings(BuildingType::Hunter) + GetNumBuildings(BuildingType::Fishery);
                buildingsWanted[BuildingType::IronMine] =
                  (inventory.people[Job::Miner] + inventory.goods[GoodType::PickAxe]
                     > GetNumBuildings(BuildingType::CoalMine) + GetNumBuildings(BuildingType::GoldMine) + 1
                   && numFoodProducers > 4) ?
                    2 :
                    1;
                buildingsWanted[BuildingType::GoldMine] = (inventory.people[Job::Miner] > 2) ? 1 : 0;
                resourcelimit = inventory.people[Job::CharBurner] + inventory.goods[GoodType::Shovel];
                if(aijh.ggs.isEnabled(AddonId::CHARBURNER)
                   && (GetNumBuildings(BuildingType::CoalMine) < 1
                       && (GetNumBuildings(BuildingType::IronMine) + GetNumBuildings(BuildingType::GoldMine) > 0)))
                    buildingsWanted[BuildingType::Charburner] = std::min(1, resourcelimit);
            }
            if(GetNumBuildings(BuildingType::Quarry) + 1 < buildingsWanted[BuildingType::Quarry]
               && aijh.AmountInStorage(GoodType::Stones) < 100) // no quarry and low stones -> try granitemines.
            {
                buildingsWanted[BuildingType::GraniteMine] = std::min<int>(
                  numMilitaryBlds / 8 + 1, // limit granite mines to military / 10
                  std::max<int>(buildingsWanted[BuildingType::Quarry] - GetNumBuildings(BuildingType::Quarry), 1));
            } else
                buildingsWanted[BuildingType::GraniteMine] = 0;
        }

        // Only build catapults if we have more than 5 military blds and 50 stones. Then reserve 4 stones per catapult
        // but do not build more than 4 additions catapults Note: This is a max amount. A catapult is only placed if
        // reasonable
        buildingsWanted[BuildingType::Catapult] =
          numMilitaryBlds <= 5 || inventory.goods[GoodType::Stones] < 50 ?
            0 :
            std::min((inventory.goods[GoodType::Stones] - 50) / 4, GetNumBuildings(BuildingType::Catapult) + 4);
    }
    if(aijh.ggs.GetMaxMilitaryRank() == 0)
    {
        buildingsWanted[BuildingType::GoldMine] = 0; // max rank is 0 = private / recruit ==> gold is useless!
    }
}

int BuildingPlanner::GetNumAdditionalBuildingsWanted(BuildingType type) const
{
    return static_cast<int>(buildingsWanted[type]) - static_cast<int>(GetNumBuildings(type));
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