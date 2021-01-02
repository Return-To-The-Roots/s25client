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

#include "SettingsTypes.h"

const DistributionMap distributionMap = {{
  DistributionMapping(GoodType::Fish, BuildingType::GraniteMine, 3),
  DistributionMapping(GoodType::Fish, BuildingType::CoalMine, 5),
  DistributionMapping(GoodType::Fish, BuildingType::IronMine, 7),
  DistributionMapping(GoodType::Fish, BuildingType::GoldMine, 10),

  DistributionMapping(GoodType::Grain, BuildingType::Mill, 5),
  DistributionMapping(GoodType::Grain, BuildingType::PigFarm, 3),
  DistributionMapping(GoodType::Grain, BuildingType::DonkeyBreeder, 2),
  DistributionMapping(GoodType::Grain, BuildingType::Brewery, 3),
  DistributionMapping(GoodType::Grain, BuildingType::Charburner, 3),

  DistributionMapping(GoodType::Iron, BuildingType::Armory, 8),
  DistributionMapping(GoodType::Iron, BuildingType::Metalworks, 4),

  DistributionMapping(GoodType::Coal, BuildingType::Armory, 8),
  DistributionMapping(GoodType::Coal, BuildingType::Ironsmelter, 7),
  DistributionMapping(GoodType::Coal, BuildingType::Mint, 10),

  DistributionMapping(GoodType::Wood, BuildingType::Sawmill, 8),
  DistributionMapping(GoodType::Wood, BuildingType::Charburner, 3),

  DistributionMapping(GoodType::Boards, BuildingType::Headquarters, 10),
  DistributionMapping(GoodType::Boards, BuildingType::Metalworks, 4),
  DistributionMapping(GoodType::Boards, BuildingType::Shipyard, 2),

  DistributionMapping(GoodType::Water, BuildingType::Bakery, 6),
  DistributionMapping(GoodType::Water, BuildingType::Brewery, 3),
  DistributionMapping(GoodType::Water, BuildingType::PigFarm, 2),
  DistributionMapping(GoodType::Water, BuildingType::DonkeyBreeder, 2),
}};
