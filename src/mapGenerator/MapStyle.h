// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef MapStyle_h__
#define MapStyle_h__

/**
 * Random map types for map generation. A map type describes the basic high-level look of a map.
 */
enum MapStyle
{
    /**
     * Greenland consists moslty of green area, small lakes, trees, stones and rather small mountains.
     */
    Greenland,
    
    /**
     * Riverland is similar to Greenland with additional rivers crossing the map.
     */
    Riverland,
    
    /**
     * Each player starts on its own island. There're also uninhabited islands around the map.
     */
    Islands,
    
    /**
     * All players start on the same island.
     */
    Contient,
    
    /**
     * All players start on their own, small island. There's one big island in the center of the map
     * with lots of resources and mountains.
     */
    Migration,
    
    /**
     * Mega random!
     */
    Random
};

#endif // MapStyle_h__
