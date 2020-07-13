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
  DistributionMapping(GD_FISH, BLD_GRANITEMINE, 3),     DistributionMapping(GD_FISH, BLD_COALMINE, 5),
  DistributionMapping(GD_FISH, BLD_IRONMINE, 7),        DistributionMapping(GD_FISH, BLD_GOLDMINE, 10),

  DistributionMapping(GD_GRAIN, BLD_MILL, 5),           DistributionMapping(GD_GRAIN, BLD_PIGFARM, 3),
  DistributionMapping(GD_GRAIN, BLD_DONKEYBREEDER, 2),  DistributionMapping(GD_GRAIN, BLD_BREWERY, 3),
  DistributionMapping(GD_GRAIN, BLD_CHARBURNER, 3),

  DistributionMapping(GD_IRON, BLD_ARMORY, 8),          DistributionMapping(GD_IRON, BLD_METALWORKS, 4),

  DistributionMapping(GD_COAL, BLD_ARMORY, 8),          DistributionMapping(GD_COAL, BLD_IRONSMELTER, 7),
  DistributionMapping(GD_COAL, BLD_MINT, 10),

  DistributionMapping(GD_WOOD, BLD_SAWMILL, 8),         DistributionMapping(GD_WOOD, BLD_CHARBURNER, 3),

  DistributionMapping(GD_BOARDS, BLD_HEADQUARTERS, 10), DistributionMapping(GD_BOARDS, BLD_METALWORKS, 4),
  DistributionMapping(GD_BOARDS, BLD_SHIPYARD, 2),

  DistributionMapping(GD_WATER, BLD_BAKERY, 6),         DistributionMapping(GD_WATER, BLD_BREWERY, 3),
  DistributionMapping(GD_WATER, BLD_PIGFARM, 2),        DistributionMapping(GD_WATER, BLD_DONKEYBREEDER, 2),
}};
