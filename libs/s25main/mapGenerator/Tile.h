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

#ifndef Tile_h__
#define Tile_h__

#include "mapGenerator/GridUtility.h"
#include <set>

namespace rttr {
namespace mapGenerator {

/**
 * A tile consists of two triangles connected by one edge.
 * This is not necessarily the same as a grid point:
 *
 *    /|  /      \  |\         /_\
 *   /_|/   or    \|_\ or   \  /
 *
 */
struct Tile
{
    /**
     * Right-side-up triangle position of the tile.
     */
    Position rsu;

    /**
     * Left-side-down triangle position of the tile.
     */
    Position lsd;
    
    /**
     * Creates an invalid texture for memory reservation (e.g. for initialization of vectors).
     */
    Tile() : rsu(Position::Invalid()), lsd(Position::Invalid())
    {
        
    }
    
    /**
     * Creates a new tile for the specified position.
     * @param position position of both triangles of the tile
     */
    Tile(const Position& position) : rsu(position), lsd(position)
    {
        
    }
    
    /**
     * Creates a new tile from the specified triangles.
     * @param rsu position of RSU triangle of the tile
     * @param lsd position of LSD triangle of the tile
     */
    Tile(const Position& rsu, const Position& lsd) : rsu(rsu), lsd(lsd)
    {
        
    }
    
    /**
     * Checks if the right-side-up and left-side-down triangles shape a valid tile.
     * @param size map size in x- and y-direction
     */
    bool IsValid(const MapExtent& size) const;
    
    /**
     * Computes the index of the tile's RSU texture.
     * @param size map size in x- and y-direction
     */
    int IndexRsu(const MapExtent& size) const;

    /**
     * Computes the index of the tile's LSD texture.
     * @param size map size in x- and y-direction
     */
    int IndexLsd(const MapExtent& size) const;

    /**
     * Creates a new tile with the specified offsets.
     * @param x1 rsu offset in x direction
     * @param y1 rsu offset in y direction
     * @param x2 lsd offset in x direction
     * @param y2 lsd offset in y direction
     * @param size map size
     */
    Tile Next(int x1, int y1, int x2, int y2, const MapExtent& size) const;
    
    /**
     * Collects all tiles with the specified offsets.
     * @param o rsu and lsd offsets in x- and y-direction
     * @param size map size
     */
    std::vector<Tile> Next(std::vector<int> o, const MapExtent& size) const;
    
    /**
     * Find and returns all neighbors of this tile.
     */
    std::vector<Tile> Neighbors(const MapExtent& size) const;
};

/**
 * Implements a comparator for tiles to ensure correct ordering of tiles within a std::set.
 */
struct TileCompare
{
    /**
     * Compares the tile of the left-hand-side (lhs) to the tile of the right-hand-side (rhs).
     * To determine equality of tiles this function can be called twice with swapped rhs & lhs.
     * If both calls return false, the tile can be considered equal.
     * @param lhs first tile for the comparison
     * @param rhs second tile for the comparison
     * @return true if lhs is less than rhs, false otherwise.
     */
    bool operator() (const Tile& lhs, const Tile& rhs) const {
        return
            (lhs.rsu.x < rhs.rsu.x) ||
            (lhs.rsu.x == rhs.rsu.x && lhs.rsu.y < rhs.rsu.y) ||
            (lhs.rsu.x == rhs.rsu.x && lhs.rsu.y == rhs.rsu.y && lhs.lsd.x < rhs.lsd.x) ||
            (lhs.rsu.x == rhs.rsu.x && lhs.rsu.y == rhs.rsu.y && lhs.lsd.x == rhs.lsd.x && lhs.lsd.y < rhs.lsd.y);
    }
};

/**
 * Implements a comparator for positions to ensure correct ordering of positions within a std::set.
 */
struct LessByPosition
{
    bool operator() (const Position& lhs, const Position& rhs) const {
        return lhs.y < rhs.y || (lhs.x < rhs.x && lhs.y == rhs.y);
    }
};

/**
 * Converts a set of tiles into a vector of positions by adding all positions which are covered either by an
 * RSU or LSD texture of a tile.
 * @param tiles set of tiles to convert
 * @return a vector of unique positions coverted by the tiles
 */
std::vector<Position> TileSetToPositions(const std::set<Tile, TileCompare>& tiles);

}}

#endif // Tile_h__
