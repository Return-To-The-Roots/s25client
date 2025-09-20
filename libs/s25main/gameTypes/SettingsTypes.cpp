// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "SettingsTypes.h"

const DistributionMap distributionMap = {{
  DistributionMapping(GoodType::Fish, BuildingType::GraniteMine, 3),
  DistributionMapping(GoodType::Fish, BuildingType::CoalMine, 5),
  DistributionMapping(GoodType::Fish, BuildingType::IronMine, 7),
  DistributionMapping(GoodType::Fish, BuildingType::GoldMine, 10),
  DistributionMapping(GoodType::Fish, BuildingType::Temple, 8),

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
  DistributionMapping(GoodType::Wood, BuildingType::Vineyard, 2),

  DistributionMapping(GoodType::Boards, BuildingType::Headquarters, 10),
  DistributionMapping(GoodType::Boards, BuildingType::Metalworks, 4),
  DistributionMapping(GoodType::Boards, BuildingType::Shipyard, 2),
  DistributionMapping(GoodType::Boards, BuildingType::Tannery, 1),

  DistributionMapping(GoodType::Water, BuildingType::Bakery, 6),
  DistributionMapping(GoodType::Water, BuildingType::Brewery, 3),
  DistributionMapping(GoodType::Water, BuildingType::PigFarm, 2),
  DistributionMapping(GoodType::Water, BuildingType::DonkeyBreeder, 2),
  DistributionMapping(GoodType::Water, BuildingType::Vineyard, 2),

  DistributionMapping(GoodType::Ham, BuildingType::Slaughterhouse, 8),
  DistributionMapping(GoodType::Ham, BuildingType::Skinner, 3),
}};
