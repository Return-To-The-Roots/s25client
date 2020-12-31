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

#include "BuildingPlanner.h"
#include "AIPlayerJH.h"
#include "GlobalGameSettings.h"
#include "addons/const_addons.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobMilitary.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/GoodTypes.h"
#include "gameData/BuildingProperties.h"
#include <algorithm>
#include <cmath>

namespace AIJH {
BuildingPlanner::BuildingPlanner(const AIPlayerJH& aijh) : buildingsWanted(NUM_BUILDING_TYPES), expansionRequired(false)
{
    RefreshBuildingNums(aijh);
    InitBuildingsWanted(aijh);
    UpdateBuildingsWanted(aijh);
}

void BuildingPlanner::Update(unsigned gf, AIPlayerJH& aijh)
{
    RefreshBuildingNums(aijh);
    expansionRequired = CalcIsExpansionRequired(aijh, gf > 500 && gf % 500 == 0);
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
    bool hasWood = GetNumBuildings(BLD_WOODCUTTER) > 0;
    bool hasBoards = GetNumBuildings(BLD_SAWMILL) > 0;
    bool hasStone = GetNumBuildings(BLD_QUARRY) > 0;
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
                hasWood = aijh.FindPositionForBuildingAround(BLD_WOODCUTTER, bld->GetPos()).isValid();
            if(!hasBoards)
                hasBoards = aijh.FindPositionForBuildingAround(BLD_SAWMILL, bld->GetPos()).isValid();
            if(!hasStone)
                hasStone = aijh.FindPositionForBuildingAround(BLD_QUARRY, bld->GetPos()).isValid();
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

void BuildingPlanner::InitBuildingsWanted(const AIPlayerJH& aijh)
{
    std::fill(buildingsWanted.begin(), buildingsWanted.end(), 0u);
    buildingsWanted[BLD_FORESTER] = 1;
    buildingsWanted[BLD_SAWMILL] = 1;
    buildingsWanted[BLD_WOODCUTTER] = 12;
    buildingsWanted[BLD_GRANITEMINE] = 0;
    buildingsWanted[BLD_COALMINE] = 4;
    buildingsWanted[BLD_IRONMINE] = 2;
    buildingsWanted[BLD_GOLDMINE] = 1;
    buildingsWanted[BLD_CATAPULT] = 5;
    buildingsWanted[BLD_FISHERY] = 6;
    buildingsWanted[BLD_QUARRY] = 6;
    buildingsWanted[BLD_HUNTER] = 2;
    buildingsWanted[BLD_FARM] =
      aijh.player.GetInventory().goods[GoodType::Scythe] + aijh.player.GetInventory().people[Job::Farmer];

    unsigned numAIRelevantSeaIds = aijh.GetNumAIRelevantSeaIds();
    if(numAIRelevantSeaIds > 0)
    {
        buildingsWanted[BLD_HARBORBUILDING] = 99;
        buildingsWanted[BLD_SHIPYARD] = numAIRelevantSeaIds == 1 ? 1 : 99;
    }
}

void BuildingPlanner::UpdateBuildingsWanted(const AIPlayerJH& aijh)
{
    // no military buildings -> usually start only
    const unsigned numMilitaryBlds = aijh.player.GetBuildingRegister().GetMilitaryBuildings().size();

    if(numMilitaryBlds == 0 && aijh.player.GetBuildingRegister().GetStorehouses().size() < 2)
    {
        buildingsWanted[BLD_FORESTER] = 1;
        // probably only has 1 saw+carpenter but if that is the case the ai will try to produce 1 additional saw very
        // quickly
        buildingsWanted[BLD_SAWMILL] = 2;
        buildingsWanted[BLD_WOODCUTTER] = 2;
        buildingsWanted[BLD_QUARRY] = 2;
        buildingsWanted[BLD_GRANITEMINE] = 0;
        buildingsWanted[BLD_COALMINE] = 0;
        buildingsWanted[BLD_IRONMINE] = 0;
        buildingsWanted[BLD_GOLDMINE] = 0;
        buildingsWanted[BLD_CATAPULT] = 0;
        buildingsWanted[BLD_FISHERY] = 0;
        buildingsWanted[BLD_HUNTER] = 0;
        buildingsWanted[BLD_FARM] = 0;
        buildingsWanted[BLD_CHARBURNER] = 0;
    } else
    {
        // at least some expansion happened -> more buildings wanted
        // building wanted usually limited by profession workers+tool for profession with some arbitrary limit. Some
        // buildings which are linked to others in a chain / profession-tool-rivalry have additional limits.
        const Inventory& inventory = aijh.player.GetInventory();

        // foresters
        unsigned max_available_forester = inventory[Job::Forester] + inventory[GoodType::Shovel];
        unsigned additional_forester = GetNumBuildings(BLD_CHARBURNER);

        // 1 mil -> 1 forester, 2 mil -> 2 forester, 4 mil -> 3 forester, 8 mil -> 4 forester, 16 mil -> 5 forester, ...
        // wanted
        if(numMilitaryBlds > 0)
            buildingsWanted[BLD_FORESTER] = (unsigned)(1.45 * log(numMilitaryBlds) + 1);
        // If we are low on wood, we need more foresters
        if(aijh.player.GetBuildingRegister().GetBuildingSites().size()
           > (inventory[GoodType::Boards] + inventory[GoodType::Wood]) * 2)
            additional_forester++;

        buildingsWanted[BLD_FORESTER] += additional_forester;
        buildingsWanted[BLD_FORESTER] = std::min(max_available_forester, buildingsWanted[BLD_FORESTER]);

        // woodcutters
        unsigned max_available_woodcutter = inventory.goods[GoodType::Axe] + inventory.people[Job::Woodcutter];
        buildingsWanted[BLD_WOODCUTTER] = std::min(
          max_available_woodcutter, buildingsWanted[BLD_FORESTER] * 2 + 1); // two per forester + 1 for 'natural' forest

        ////on maps with many trees the ai will build woodcutters all over the place which means the foresters are not
        /// really required
        // TODO: get number of trees in own territory. use it relatively to total size of own territory to adapt number
        // of foresters (less) and woodcutters (more)

        // fishery & hunter
        buildingsWanted[BLD_FISHERY] =
          std::min(inventory.goods[GoodType::RodAndLine] + inventory.people[Job::Fisher], numMilitaryBlds + 1u);
        buildingsWanted[BLD_HUNTER] = std::min(inventory.goods[GoodType::Bow] + inventory.people[Job::Hunter], 4u);

        // quarry: low ware games start at 2 otherwise build as many as we have stonemasons, higher ware games up to 6
        // quarries
        if(inventory.goods[GoodType::PickAxe] + inventory.people[Job::Miner] < 7
           && inventory.people[Job::Stonemason] > 0 && inventory.people[Job::Miner] < 3)
        {
            buildingsWanted[BLD_QUARRY] = std::max(std::min(inventory.people[Job::Stonemason], numMilitaryBlds), 2u);
        } else
        {
            //>6miners = build up to 6 depending on resources, else max out at miners/2
            if(inventory.people[Job::Miner] > 6)
                buildingsWanted[BLD_QUARRY] =
                  std::min(inventory.goods[GoodType::PickAxe] + inventory.people[Job::Stonemason], 6u);
            else
                buildingsWanted[BLD_QUARRY] = inventory.people[Job::Miner] / 2;

            if(buildingsWanted[BLD_QUARRY] > numMilitaryBlds)
                buildingsWanted[BLD_QUARRY] = numMilitaryBlds;
        }
        // sawmills limited by woodcutters and carpenter+saws reduced by char burners minimum of 3
        int resourcelimit = inventory.people[Job::Carpenter] + inventory.goods[GoodType::Saw];
        int numSawmillsFed =
          (static_cast<int>(GetNumBuildings(BLD_WOODCUTTER)) - static_cast<int>(GetNumBuildings(BLD_CHARBURNER) * 2))
          / 2;
        buildingsWanted[BLD_SAWMILL] =
          std::max(std::min(numSawmillsFed, resourcelimit), 3); // min 3
                                                                // iron smelters limited by iron mines or crucibles
        buildingsWanted[BLD_IRONSMELTER] = std::min(
          inventory.goods[GoodType::Crucible] + inventory.people[Job::IronFounder], GetNumBuildings(BLD_IRONMINE));

        buildingsWanted[BLD_MINT] = GetNumBuildings(BLD_GOLDMINE);
        // armory count = smelter -metalworks if there is more than 1 smelter or 1 if there is just 1.
        buildingsWanted[BLD_ARMORY] = (GetNumBuildings(BLD_IRONSMELTER) > 1) ?
                                        std::max(0, static_cast<int>(GetNumBuildings(BLD_IRONSMELTER))
                                                      - static_cast<int>(GetNumBuildings(BLD_METALWORKS))) :
                                        GetNumBuildings(BLD_IRONSMELTER);
        if(aijh.ggs.isEnabled(AddonId::HALF_COST_MIL_EQUIP))
            buildingsWanted[BLD_ARMORY] *= 2;
        // brewery count = 1+(armory/5) if there is at least 1 armory or armory /6 for exhaustible mines
        if(GetNumBuildings(BLD_ARMORY) > 0 && GetNumBuildings(BLD_FARM) > 0)
        {
            if(aijh.ggs.isEnabled(AddonId::INEXHAUSTIBLE_MINES))
                buildingsWanted[BLD_BREWERY] = 1 + GetNumBuildings(BLD_ARMORY) / 5;
            else
                buildingsWanted[BLD_BREWERY] = 1 + GetNumBuildings(BLD_ARMORY) / 6;
        } else
            buildingsWanted[BLD_BREWERY] = 0;
        // metalworks is 1 if there is at least 1 smelter, 2 if mines are inexhaustible and we have at least 4 iron
        // smelters
        buildingsWanted[BLD_METALWORKS] = (GetNumBuildings(BLD_IRONSMELTER) > 0) ? 1 : 0;

        // max processing
        unsigned foodusers = GetNumBuildings(BLD_CHARBURNER) + GetNumBuildings(BLD_MILL) + GetNumBuildings(BLD_BREWERY)
                             + GetNumBuildings(BLD_PIGFARM) + GetNumBuildings(BLD_DONKEYBREEDER);

        if(GetNumBuildings(BLD_FARM) >= foodusers - GetNumBuildings(BLD_MILL))
            buildingsWanted[BLD_MILL] = std::min(GetNumBuildings(BLD_FARM) - (foodusers - GetNumBuildings(BLD_MILL)),
                                                 GetNumBuildings(BLD_BAKERY) + 1);
        else
            buildingsWanted[BLD_MILL] = GetNumBuildings(BLD_MILL);

        resourcelimit = inventory.people[Job::Baker] + inventory.goods[GoodType::Rollingpin] + 1;
        buildingsWanted[BLD_BAKERY] = std::min<unsigned>(GetNumBuildings(BLD_MILL), resourcelimit);

        buildingsWanted[BLD_PIGFARM] =
          (GetNumBuildings(BLD_FARM) < 8) ? GetNumBuildings(BLD_FARM) / 4 : (GetNumBuildings(BLD_FARM) - 2) / 4;
        if(buildingsWanted[BLD_PIGFARM] > GetNumBuildings(BLD_SLAUGHTERHOUSE) + 1)
            buildingsWanted[BLD_PIGFARM] = GetNumBuildings(BLD_SLAUGHTERHOUSE) + 1;
        buildingsWanted[BLD_SLAUGHTERHOUSE] =
          std::min(inventory.goods[GoodType::Cleaver] + inventory.people[Job::Butcher], GetNumBuildings(BLD_PIGFARM));

        buildingsWanted[BLD_WELL] = buildingsWanted[BLD_BAKERY] + buildingsWanted[BLD_PIGFARM]
                                    + buildingsWanted[BLD_DONKEYBREEDER] + buildingsWanted[BLD_BREWERY];

        buildingsWanted[BLD_FARM] =
          std::min<unsigned>(inventory.goods[GoodType::Scythe] + inventory.people[Job::Farmer], foodusers + 3);

        if(inventory.goods[GoodType::PickAxe] + inventory.people[Job::Miner] < 3)
        {
            // almost out of new pickaxes and miners - emergency program: get coal,iron,smelter&metalworks
            buildingsWanted[BLD_COALMINE] = 1;
            buildingsWanted[BLD_IRONMINE] = 1;
            buildingsWanted[BLD_GOLDMINE] = 0;
            buildingsWanted[BLD_IRONSMELTER] = 1;
            buildingsWanted[BLD_METALWORKS] = 1;
            buildingsWanted[BLD_ARMORY] = 0;
            buildingsWanted[BLD_GRANITEMINE] = 0;
            buildingsWanted[BLD_MINT] = 0;
        } else // more than 2 miners
        {
            // coal mine count now depends on iron & gold not linked to food or material supply - might have to add a
            // material check if this makes problems
            if(GetNumBuildings(BLD_IRONMINE) > 0)
                buildingsWanted[BLD_COALMINE] = GetNumBuildings(BLD_IRONMINE) * 2 - 1 + GetNumBuildings(BLD_GOLDMINE);
            else
                buildingsWanted[BLD_COALMINE] = std::max(GetNumBuildings(BLD_GOLDMINE), 1u);
            // more mines planned than food available? -> limit mines
            if(buildingsWanted[BLD_COALMINE] > 2
               && buildingsWanted[BLD_COALMINE] * 2 > GetNumBuildings(BLD_FARM) + GetNumBuildings(BLD_FISHERY) + 1)
                buildingsWanted[BLD_COALMINE] = (GetNumBuildings(BLD_FARM) + GetNumBuildings(BLD_FISHERY)) / 2 + 2;
            if(GetNumBuildings(BLD_FARM) > 7) // quite the empire just scale mines with farms
            {
                if(aijh.ggs.isEnabled(
                     AddonId::INEXHAUSTIBLE_MINES)) // inexhaustible mines? -> more farms required for each mine
                    buildingsWanted[BLD_IRONMINE] =
                      std::min(GetNumBuildings(BLD_IRONSMELTER) + 1, GetNumBuildings(BLD_FARM) * 2 / 5);
                else
                    buildingsWanted[BLD_IRONMINE] =
                      std::min(GetNumBuildings(BLD_FARM) / 2, GetNumBuildings(BLD_IRONSMELTER) + 1);
                buildingsWanted[BLD_GOLDMINE] =
                  (GetNumBuildings(BLD_MINT) > 0) ?
                    GetNumBuildings(BLD_IRONSMELTER) > 6 && GetNumBuildings(BLD_MINT) > 1 ?
                    GetNumBuildings(BLD_IRONSMELTER) > 10 ? 4 : 3 :
                    2 :
                    1;
                buildingsWanted[BLD_DONKEYBREEDER] = 1;
                if(aijh.ggs.isEnabled(AddonId::CHARBURNER)
                   && (buildingsWanted[BLD_COALMINE] > GetNumBuildings(BLD_COALMINE) + 4))
                {
                    resourcelimit = inventory.people[Job::CharBurner] + inventory.goods[GoodType::Shovel] + 1;
                    buildingsWanted[BLD_CHARBURNER] = std::min<unsigned>(
                      std::min(buildingsWanted[BLD_COALMINE] - (GetNumBuildings(BLD_COALMINE) + 1), 3u), resourcelimit);
                }
            } else
            {
                // probably still limited in food supply go up to 4 coal 1 gold 2 iron (gold+coal->coin,
                // iron+coal->tool, iron+coal+coal->weapon)
                unsigned numFoodProducers = GetNumBuildings(BLD_BAKERY) + GetNumBuildings(BLD_SLAUGHTERHOUSE)
                                            + GetNumBuildings(BLD_HUNTER) + GetNumBuildings(BLD_FISHERY);
                buildingsWanted[BLD_IRONMINE] = (inventory.people[Job::Miner] + inventory.goods[GoodType::PickAxe]
                                                   > GetNumBuildings(BLD_COALMINE) + GetNumBuildings(BLD_GOLDMINE) + 1
                                                 && numFoodProducers > 4) ?
                                                  2 :
                                                  1;
                buildingsWanted[BLD_GOLDMINE] = (inventory.people[Job::Miner] > 2) ? 1 : 0;
                resourcelimit = inventory.people[Job::CharBurner] + inventory.goods[GoodType::Shovel];
                if(aijh.ggs.isEnabled(AddonId::CHARBURNER)
                   && (GetNumBuildings(BLD_COALMINE) < 1
                       && (GetNumBuildings(BLD_IRONMINE) + GetNumBuildings(BLD_GOLDMINE) > 0)))
                    buildingsWanted[BLD_CHARBURNER] = std::min(1, resourcelimit);
            }
            if(GetNumBuildings(BLD_QUARRY) + 1 < buildingsWanted[BLD_QUARRY]
               && aijh.AmountInStorage(GoodType::Stones) < 100) // no quarry and low stones -> try granitemines.
            {
                if(inventory.people[Job::Miner] > 6)
                {
                    buildingsWanted[BLD_GRANITEMINE] =
                      std::min<int>(numMilitaryBlds / 15 + 1, // limit granite mines to military / 15
                                    std::max<int>(buildingsWanted[BLD_QUARRY] - GetNumBuildings(BLD_QUARRY), 1));
                } else
                    buildingsWanted[BLD_GRANITEMINE] = 1;
            } else
                buildingsWanted[BLD_GRANITEMINE] = 0;
        }

        // Only build catapults if we have more than 5 military blds and 50 stones. Then reserve 4 stones per catapult
        // but do not build more than 4 additions catapults Note: This is a max amount. A catapult is only placed if
        // reasonable
        buildingsWanted[BLD_CATAPULT] =
          numMilitaryBlds <= 5 || inventory.goods[GoodType::Stones] < 50 ?
            0 :
            std::min((inventory.goods[GoodType::Stones] - 50) / 4, GetNumBuildings(BLD_CATAPULT) + 4);
    }
    if(aijh.ggs.GetMaxMilitaryRank() == 0)
    {
        buildingsWanted[BLD_GOLDMINE] = 0; // max rank is 0 = private / recruit ==> gold is useless!
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
    if(GetNumBuildings(BLD_SAWMILL) > 0)
        return true;
    if(aijh.player.GetInventory().goods[GoodType::Boards] > 30 && GetNumBuildingSites(BLD_SAWMILL) > 0)
        return true;
    return (GetNumMilitaryBlds() + GetNumMilitaryBldSites() > 0);
}

} // namespace AIJH
