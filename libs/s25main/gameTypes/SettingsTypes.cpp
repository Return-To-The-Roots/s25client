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
  DistributionMapping(GoodType::Fish, BLD_GRANITEMINE, 3),
  DistributionMapping(GoodType::Fish, BLD_COALMINE, 5),
  DistributionMapping(GoodType::Fish, BLD_IRONMINE, 7),
  DistributionMapping(GoodType::Fish, BLD_GOLDMINE, 10),

  DistributionMapping(GoodType::Grain, BLD_MILL, 5),
  DistributionMapping(GoodType::Grain, BLD_PIGFARM, 3),
  DistributionMapping(GoodType::Grain, BLD_DONKEYBREEDER, 2),
  DistributionMapping(GoodType::Grain, BLD_BREWERY, 3),
  DistributionMapping(GoodType::Grain, BLD_CHARBURNER, 3),

  DistributionMapping(GoodType::Iron, BLD_ARMORY, 8),
  DistributionMapping(GoodType::Iron, BLD_METALWORKS, 4),

  DistributionMapping(GoodType::Coal, BLD_ARMORY, 8),
  DistributionMapping(GoodType::Coal, BLD_IRONSMELTER, 7),
  DistributionMapping(GoodType::Coal, BLD_MINT, 10),

  DistributionMapping(GoodType::Wood, BLD_SAWMILL, 8),
  DistributionMapping(GoodType::Wood, BLD_CHARBURNER, 3),

  DistributionMapping(GoodType::Boards, BLD_HEADQUARTERS, 10),
  DistributionMapping(GoodType::Boards, BLD_METALWORKS, 4),
  DistributionMapping(GoodType::Boards, BLD_SHIPYARD, 2),

  DistributionMapping(GoodType::Water, BLD_BAKERY, 6),
  DistributionMapping(GoodType::Water, BLD_BREWERY, 3),
  DistributionMapping(GoodType::Water, BLD_PIGFARM, 2),
  DistributionMapping(GoodType::Water, BLD_DONKEYBREEDER, 2),
}};
