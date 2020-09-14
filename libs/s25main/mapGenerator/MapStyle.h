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

#pragma once

/**
 * Random map types for map generation. A map type describes the basic high-level look of a map.
 */
enum class MapStyle
{ /** Greenland maps are covered by grass and mountains with very few lakes. */
  Greenland,
  /** Riverland maps are close to greenland maps but with plenty of water. */
  Riverland,
  /** Ringland maps usually covered by water apart from a ring-shaped piece of land. */
  Ringland,
  /** On migration style maps players are starting on their own little island.
   *  The main resources (mountains), however, are available only on one large
   *  island in the center of the map. */
  Migration,
  /** Each player starts on its own island which contains all relevant resources
   *  (trees, stone piles, mountains). */
  Islands,
  /** Continent maps are big islands surrounded by water.
   *  Each player starts on the same big island. */
  Continent,
  /** full random map */
  Random
};
