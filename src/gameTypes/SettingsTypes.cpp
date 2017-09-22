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
#include "SettingsTypes.h"

const DistributionMap SUPPRESS_UNUSED distributionMap = {{
  DistributionMapping(GD_FISH, BLD_GRANITEMINE),    DistributionMapping(GD_FISH, BLD_COALMINE),
  DistributionMapping(GD_FISH, BLD_IRONMINE),       DistributionMapping(GD_FISH, BLD_GOLDMINE),

  DistributionMapping(GD_GRAIN, BLD_MILL),          DistributionMapping(GD_GRAIN, BLD_PIGFARM),
  DistributionMapping(GD_GRAIN, BLD_DONKEYBREEDER), DistributionMapping(GD_GRAIN, BLD_BREWERY),
  DistributionMapping(GD_GRAIN, BLD_CHARBURNER),

  DistributionMapping(GD_IRON, BLD_ARMORY),         DistributionMapping(GD_IRON, BLD_METALWORKS),

  DistributionMapping(GD_COAL, BLD_ARMORY),         DistributionMapping(GD_COAL, BLD_IRONSMELTER),
  DistributionMapping(GD_COAL, BLD_MINT),

  DistributionMapping(GD_WOOD, BLD_SAWMILL),        DistributionMapping(GD_WOOD, BLD_CHARBURNER),

  DistributionMapping(GD_BOARDS, BLD_HEADQUARTERS), DistributionMapping(GD_BOARDS, BLD_METALWORKS),
  DistributionMapping(GD_BOARDS, BLD_SHIPYARD),

  DistributionMapping(GD_WATER, BLD_BAKERY),        DistributionMapping(GD_WATER, BLD_BREWERY),
  DistributionMapping(GD_WATER, BLD_PIGFARM),       DistributionMapping(GD_WATER, BLD_DONKEYBREEDER),
}};
