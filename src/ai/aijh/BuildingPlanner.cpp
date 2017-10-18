// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "defines.h" // IWYU pragma: keep
#include "BuildingPlanner.h"
#include "AIPlayerJH.h"
#include "GlobalGameSettings.h"
#include "addons/const_addons.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/GoodTypes.h"
#include "gameData/BuildingProperties.h"
#include <boost/foreach.hpp>
#include <algorithm>

namespace AIJH {
BuildingPlanner::BuildingPlanner(const AIPlayerJH& aijh) : aijh(aijh), buildingsWanted(BUILDING_TYPES_COUNT)
{
    RefreshBuildingCount();
    InitBuildingsWanted();
    UpdateBuildingsWanted();
}

void BuildingPlanner::RefreshBuildingCount()
{
    buildingCounts = aijh.player.GetBuildingRegister().GetBuildingCount();
}

unsigned BuildingPlanner::GetBuildingCount(BuildingType type) const
{
    return buildingCounts.buildings[type] + buildingCounts.buildingSites[type];
}

unsigned BuildingPlanner::GetBuildingSitesCount(BuildingType type) const
{
    return buildingCounts.buildingSites[type];
}

unsigned BuildingPlanner::GetMilitaryBldCount() const
{
    unsigned result = 0;
    BOOST_FOREACH(BuildingType bld, BuildingProperties::militaryBldTypes)
        result += GetBuildingCount(bld);
    return result;
}

unsigned BuildingPlanner::GetMilitaryBldSiteCount() const
{
    unsigned result = 0;
    BOOST_FOREACH(BuildingType bld, BuildingProperties::militaryBldTypes)
        result += GetBuildingSitesCount(bld);
    return result;
}

void BuildingPlanner::InitBuildingsWanted()
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
    buildingsWanted[BLD_FARM] = aijh.player.GetInventory().goods[GD_SCYTHE] + aijh.player.GetInventory().people[JOB_FARMER];

    unsigned numAIRelevantSeaIds = aijh.GetCountofAIRelevantSeaIds();
    if(numAIRelevantSeaIds > 0)
    {
        buildingsWanted[BLD_HARBORBUILDING] = 99;
        buildingsWanted[BLD_SHIPYARD] = numAIRelevantSeaIds == 1 ? 1 : 99;
    }
}

void BuildingPlanner::UpdateBuildingsWanted()
{
    // no military buildings -> usually start only
    const unsigned numMilitaryBlds = aijh.player.GetBuildingRegister().GetMilitaryBuildings().size();

    if(numMilitaryBlds == 0 && aijh.player.GetBuildingRegister().GetStorehouses().size() < 2)
    {
        buildingsWanted[BLD_FORESTER] = 1;
        // probably only has 1 saw+carpenter but if that is the case the ai will try to produce 1 additional saw very quickly
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
    } else // at least some expansion happened -> more buildings wanted
           // building wanted usually limited by profession workers+tool for profession with some arbitrary limit. Some buildings which are
           // linked to others in a chain / profession-tool-rivalry have additional limits.
    {
        const Inventory& inventory = aijh.player.GetInventory();

        // foresters
        unsigned max_available_forester = inventory.people[JOB_FORESTER] + inventory.goods[GD_SHOVEL];
        unsigned additional_forester = GetBuildingCount(BLD_CHARBURNER);

        // 1 mil -> 1 forester, 2 mil -> 2 forester, 4 mil -> 3 forester, 8 mil -> 4 forester, 16 mil -> 5 forester, ... wanted
        if(numMilitaryBlds > 0)
            buildingsWanted[BLD_FORESTER] = (unsigned)(1.45 * log(numMilitaryBlds) + 1);

        buildingsWanted[BLD_FORESTER] += additional_forester;
        buildingsWanted[BLD_FORESTER] = std::min(max_available_forester, buildingsWanted[BLD_FORESTER]);

        // woodcutters
        unsigned max_available_woodcutter = inventory.goods[GD_AXE] + inventory.people[JOB_WOODCUTTER];
        buildingsWanted[BLD_WOODCUTTER] =
          std::min(max_available_woodcutter, buildingsWanted[BLD_FORESTER] * 2 + 1); // two per forester + 1 for 'natural' forest

        ////on maps with many trees the ai will build woodcutters all over the place which means the foresters are not really required
        // TODO: get number of trees in own territory. use it relatively to total size of own territory to adapt number of foresters (less)
        // and woodcutters (more)

        // fishery & hunter
        buildingsWanted[BLD_FISHERY] = std::min(inventory.goods[GD_RODANDLINE] + inventory.people[JOB_FISHER], numMilitaryBlds + 1u);
        buildingsWanted[BLD_HUNTER] = std::min(inventory.goods[GD_BOW] + inventory.people[JOB_HUNTER], 4u);

        // quarry: low ware games start at 2 otherwise build as many as we have stonemasons, higher ware games up to 6 quarries
        if(inventory.goods[GD_PICKAXE] + inventory.people[JOB_MINER] < 7 && inventory.people[JOB_STONEMASON] > 0
           && inventory.people[JOB_MINER] < 3)
        {
            buildingsWanted[BLD_QUARRY] = std::max(std::min(inventory.people[JOB_STONEMASON], numMilitaryBlds), 2u);
        } else
        {
            //>6miners = build up to 6 depending on resources, else max out at miners/2
            if(inventory.people[JOB_MINER] > 6)
                buildingsWanted[BLD_QUARRY] = std::min(inventory.goods[GD_PICKAXE] + inventory.people[JOB_STONEMASON], 6u);
            else
                buildingsWanted[BLD_QUARRY] = inventory.people[JOB_MINER] / 2;

            if(buildingsWanted[BLD_QUARRY] > numMilitaryBlds)
                buildingsWanted[BLD_QUARRY] = numMilitaryBlds;
        }
        // sawmills limited by woodcutters and carpenter+saws reduced by char burners minimum of 3
        int resourcelimit = inventory.people[JOB_CARPENTER] + inventory.goods[GD_SAW];
        int numSawmillsFed =
          (static_cast<int>(GetBuildingCount(BLD_WOODCUTTER)) - static_cast<int>(GetBuildingCount(BLD_CHARBURNER) * 2)) / 2;
        buildingsWanted[BLD_SAWMILL] =
          std::max(std::min(numSawmillsFed, resourcelimit), 3); // min 3
                                                                // iron smelters limited by iron mines or crucibles
        buildingsWanted[BLD_IRONSMELTER] =
          std::min(inventory.goods[GD_CRUCIBLE] + inventory.people[JOB_IRONFOUNDER], GetBuildingCount(BLD_IRONMINE));

        buildingsWanted[BLD_MINT] = GetBuildingCount(BLD_GOLDMINE);
        // armory count = smelter -metalworks if there is more than 1 smelter or 1 if there is just 1.
        buildingsWanted[BLD_ARMORY] =
          (GetBuildingCount(BLD_IRONSMELTER) > 1) ?
            std::max(0, static_cast<int>(GetBuildingCount(BLD_IRONSMELTER)) - static_cast<int>(GetBuildingCount(BLD_METALWORKS))) :
            GetBuildingCount(BLD_IRONSMELTER);
        if(aijh.ggs.isEnabled(AddonId::HALF_COST_MIL_EQUIP))
            buildingsWanted[BLD_ARMORY] *= 2;
        // brewery count = 1+(armory/5) if there is at least 1 armory or armory /6 for exhaustible mines
        if(GetBuildingCount(BLD_ARMORY) > 0 && GetBuildingCount(BLD_FARM) > 0)
        {
            if(aijh.ggs.isEnabled(AddonId::INEXHAUSTIBLE_MINES))
                buildingsWanted[BLD_BREWERY] = 1 + GetBuildingCount(BLD_ARMORY) / 5;
            else
                buildingsWanted[BLD_BREWERY] = 1 + GetBuildingCount(BLD_ARMORY) / 6;
        } else
            buildingsWanted[BLD_BREWERY] = 0;
        // metalworks is 1 if there is at least 1 smelter, 2 if mines are inexhaustible and we have at least 4 iron smelters
        buildingsWanted[BLD_METALWORKS] = (GetBuildingCount(BLD_IRONSMELTER) > 0) ? 1 : 0;

        // max processing
        unsigned foodusers = GetBuildingCount(BLD_CHARBURNER) + GetBuildingCount(BLD_MILL) + GetBuildingCount(BLD_BREWERY)
                             + GetBuildingCount(BLD_PIGFARM) + GetBuildingCount(BLD_DONKEYBREEDER);

        if(GetBuildingCount(BLD_FARM) >= foodusers - GetBuildingCount(BLD_MILL))
            buildingsWanted[BLD_MILL] =
              std::min(GetBuildingCount(BLD_FARM) - (foodusers - GetBuildingCount(BLD_MILL)), GetBuildingCount(BLD_BAKERY) + 1);
        else
            buildingsWanted[BLD_MILL] = GetBuildingCount(BLD_MILL);

        resourcelimit = inventory.people[JOB_BAKER] + inventory.goods[GD_ROLLINGPIN] + 1;
        buildingsWanted[BLD_BAKERY] = std::min<unsigned>(GetBuildingCount(BLD_MILL), resourcelimit);

        buildingsWanted[BLD_PIGFARM] =
          (GetBuildingCount(BLD_FARM) < 8) ? GetBuildingCount(BLD_FARM) / 4 : (GetBuildingCount(BLD_FARM) - 2) / 4;
        if(buildingsWanted[BLD_PIGFARM] > GetBuildingCount(BLD_SLAUGHTERHOUSE) + 1)
            buildingsWanted[BLD_PIGFARM] = GetBuildingCount(BLD_SLAUGHTERHOUSE) + 1;
        buildingsWanted[BLD_SLAUGHTERHOUSE] =
          std::min(inventory.goods[GD_CLEAVER] + inventory.people[JOB_BUTCHER], GetBuildingCount(BLD_PIGFARM));

        buildingsWanted[BLD_WELL] =
          buildingsWanted[BLD_BAKERY] + buildingsWanted[BLD_PIGFARM] + buildingsWanted[BLD_DONKEYBREEDER] + buildingsWanted[BLD_BREWERY];

        buildingsWanted[BLD_FARM] = min<unsigned>(inventory.goods[GD_SCYTHE] + inventory.people[JOB_FARMER], foodusers + 3);

        if(inventory.goods[GD_PICKAXE] + inventory.people[JOB_MINER] < 3)
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
            // coal mine count now depends on iron & gold not linked to food or material supply - might have to add a material check if this
            // makes problems
            if(GetBuildingCount(BLD_IRONMINE) > 0)
                buildingsWanted[BLD_COALMINE] = GetBuildingCount(BLD_IRONMINE) * 2 - 1 + GetBuildingCount(BLD_GOLDMINE);
            else
                buildingsWanted[BLD_COALMINE] = std::max(GetBuildingCount(BLD_GOLDMINE), 1u);
            // more mines planned than food available? -> limit mines
            if(buildingsWanted[BLD_COALMINE] > 2
               && buildingsWanted[BLD_COALMINE] * 2 > GetBuildingCount(BLD_FARM) + GetBuildingCount(BLD_FISHERY) + 1)
                buildingsWanted[BLD_COALMINE] = (GetBuildingCount(BLD_FARM) + GetBuildingCount(BLD_FISHERY)) / 2 + 2;
            if(GetBuildingCount(BLD_FARM) > 7) // quite the empire just scale mines with farms
            {
                if(aijh.ggs.isEnabled(AddonId::INEXHAUSTIBLE_MINES)) // inexhaustible mines? -> more farms required for each mine
                    buildingsWanted[BLD_IRONMINE] = std::min(GetBuildingCount(BLD_IRONSMELTER) + 1, GetBuildingCount(BLD_FARM) * 2 / 5);
                else
                    buildingsWanted[BLD_IRONMINE] = std::min(GetBuildingCount(BLD_FARM) / 2, GetBuildingCount(BLD_IRONSMELTER) + 1);
                buildingsWanted[BLD_GOLDMINE] = (GetBuildingCount(BLD_MINT) > 0) ?
                                                  GetBuildingCount(BLD_IRONSMELTER) > 6 && GetBuildingCount(BLD_MINT) > 1 ?
                                                  GetBuildingCount(BLD_IRONSMELTER) > 10 ? 4 : 3 :
                                                  2 :
                                                  1;
                buildingsWanted[BLD_DONKEYBREEDER] = 1;
                if(aijh.ggs.isEnabled(AddonId::CHARBURNER) && (buildingsWanted[BLD_COALMINE] > GetBuildingCount(BLD_COALMINE) + 4))
                {
                    resourcelimit = inventory.people[JOB_CHARBURNER] + inventory.goods[GD_SHOVEL] + 1;
                    buildingsWanted[BLD_CHARBURNER] =
                      std::min<unsigned>(std::min(buildingsWanted[BLD_COALMINE] - (GetBuildingCount(BLD_COALMINE) + 1), 3u), resourcelimit);
                }
            } else
            {
                // probably still limited in food supply go up to 4 coal 1 gold 2 iron (gold+coal->coin, iron+coal->tool,
                // iron+coal+coal->weapon)
                unsigned numFoodProducers = GetBuildingCount(BLD_BAKERY) + GetBuildingCount(BLD_SLAUGHTERHOUSE)
                                            + GetBuildingCount(BLD_HUNTER) + GetBuildingCount(BLD_FISHERY);
                buildingsWanted[BLD_IRONMINE] = (inventory.people[JOB_MINER] + inventory.goods[GD_PICKAXE]
                                                   > GetBuildingCount(BLD_COALMINE) + GetBuildingCount(BLD_GOLDMINE) + 1
                                                 && numFoodProducers > 4) ?
                                                  2 :
                                                  1;
                buildingsWanted[BLD_GOLDMINE] = (inventory.people[JOB_MINER] > 2) ? 1 : 0;
                resourcelimit = inventory.people[JOB_CHARBURNER] + inventory.goods[GD_SHOVEL];
                if(aijh.ggs.isEnabled(AddonId::CHARBURNER)
                   && (GetBuildingCount(BLD_COALMINE) < 1 && (GetBuildingCount(BLD_IRONMINE) + GetBuildingCount(BLD_GOLDMINE) > 0)))
                    buildingsWanted[BLD_CHARBURNER] = min(1, resourcelimit);
            }
            if(GetBuildingCount(BLD_QUARRY) + 1 < buildingsWanted[BLD_QUARRY]
               && aijh.AmountInStorage(GD_STONES) < 100) // no quarry and low stones -> try granitemines.
            {
                if(inventory.people[JOB_MINER] > 6)
                {
                    buildingsWanted[BLD_GRANITEMINE] =
                      std::min<int>(numMilitaryBlds / 15 + 1, // limit granite mines to military / 15
                                    std::max<int>(buildingsWanted[BLD_QUARRY] - GetBuildingCount(BLD_QUARRY), 1));
                } else
                    buildingsWanted[BLD_GRANITEMINE] = 1;
            } else
                buildingsWanted[BLD_GRANITEMINE] = 0;
        }
    }
    if(aijh.ggs.GetMaxMilitaryRank() == 0)
    {
        buildingsWanted[BLD_GOLDMINE] = 0; // max rank is 0 = private / recruit ==> gold is useless!
    }
}

int BuildingPlanner::GetNumAdditionalBuildingsWanted(BuildingType type) const
{
    return static_cast<int>(buildingsWanted[type]) - static_cast<int>(GetBuildingCount(type));
}

bool BuildingPlanner::WantMoreMilitaryBlds() const
{
    unsigned complete = GetMilitaryBldCount();
    unsigned inconstruction = GetMilitaryBldSiteCount();
    return complete + 3 > inconstruction;
}

} // namespace AIJH
