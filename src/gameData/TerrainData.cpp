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

#include "main.h"
#include "TerrainData.h"
#include <boost/array.hpp>

TerrainType TerrainData::MapIdx2Terrain(unsigned char mapIdx)
{
    // Unset bit 0x40 (harbour) and 0x80 (unknown)
    mapIdx &= ~(0x40 | 0x80);
    switch (mapIdx)
    {
    case 0: return TT_SAVANNAH;
    case 1: return TT_MOUNTAIN1;
    case 2: return TT_SNOW;
    case 3: return TT_SWAMPLAND;
    case 4: return TT_DESERT;
    case 5: return TT_WATER;
    case 6: return TT_BUILDABLE_WATER;
    case 7: return TT_DESERT;
    case 8: return TT_MEADOW1;
    case 9: return TT_MEADOW2;
    case 10: return TT_MEADOW3;
    case 11: return TT_MOUNTAIN2;
    case 12: return TT_MOUNTAIN3;
    case 13: return TT_MOUNTAIN4;
    case 14: return TT_STEPPE;
    case 15: return TT_MEADOW_FLOWERS;
    case 16: return TT_LAVA;
    case 18: return TT_MOUNTAINMEADOW;
    case 19: return TT_WATER2;
    case 20: return TT_LAVA2;
    case 21: return TT_LAVA3;
    case 22: return TT_LAVA4;
    case 34: return TT_BUILDABLE_MOUNTAIN;
    default:
        assert(false); // Unknown/invalid texture
        return TT_SNOW;
    }
}

Rect TerrainData::GetPosInTexture(TerrainType t)
{
    switch (t)
    {
    case TT_SNOW:               return Rect(0,   0,   48, 48);
    case TT_DESERT:             return Rect(48,  0,   48, 48);
    case TT_SWAMPLAND:          return Rect(96,  0,   48, 48);
    case TT_MEADOW_FLOWERS:     return Rect(144, 0,   48, 48);
    case TT_MOUNTAIN1:          return Rect(0,   48,  48, 48);
    case TT_MOUNTAIN2:
    case TT_BUILDABLE_MOUNTAIN: return Rect(48,  48,  48, 48);
    case TT_MOUNTAIN3:          return Rect(96,  48,  48, 48);
    case TT_MOUNTAIN4:          return Rect(144, 48,  48, 48);
    case TT_SAVANNAH:           return Rect(0,   96,  48, 48);
    case TT_MEADOW1:            return Rect(48,  96,  48, 48);
    case TT_MEADOW2:            return Rect(96,  96,  48, 48);
    case TT_MEADOW3:            return Rect(144, 96,  48, 48);
    case TT_STEPPE:             return Rect(0,   144, 48, 48);
    case TT_MOUNTAINMEADOW:     return Rect(48,  144, 48, 48);
    case TT_WATER:
    case TT_WATER2:
    case TT_BUILDABLE_WATER:    return Rect(192, 48,  55, 56);
    case TT_LAVA:               return Rect(192, 104, 55, 56);
    case TT_LAVA2:              return Rect(66,  223, 31, 32);
    case TT_LAVA3:              return Rect(99,  223, 31, 32);
    case TT_LAVA4:              return Rect(132, 223, 31, 32);
    }
    throw std::logic_error("Invalid parameters given");
}

bool TerrainData::IsAnimated(TerrainType t)
{
    return IsWater(t) || IsLava(t);
}

unsigned TerrainData::GetFrameCount(TerrainType t)
{
    switch (t)
    {
    case TT_WATER:
    case TT_WATER2:
    case TT_BUILDABLE_WATER:
       return 8;
    case TT_LAVA:
    case TT_LAVA2:
    case TT_LAVA3:
    case TT_LAVA4:
        return 4;
    default:
        return 1;
    }
}

unsigned char TerrainData::GetStartColor(TerrainType t)
{
    switch (t)
    {
    case TT_WATER:
    case TT_WATER2:
    case TT_BUILDABLE_WATER:
        return 240;
    case TT_LAVA:
    case TT_LAVA2:
    case TT_LAVA3:
    case TT_LAVA4:
        return 248;
    default:
        return 0;
    }
}

unsigned TerrainData::GetColor(LandscapeType landsCape, TerrainType t)
{
    switch (landsCape)
    {
    case LT_GREENLAND:
        switch (t)
        {
        case TT_SNOW: return 0xFFFFFFFF;
        case TT_DESERT: return 0xFFc09c7c;
        case TT_SWAMPLAND: return 0xFF649014;
        case TT_MEADOW_FLOWERS: return 0xFF48780c;
        case TT_MOUNTAIN1: return 0xFF9c8058;
        case TT_MOUNTAIN2: return 0xFF9c8058;
        case TT_MOUNTAIN3: return 0xFF9c8058;
        case TT_MOUNTAIN4: return 0xFF8c7048;
        case TT_SAVANNAH: return 0xFF649014;
        case TT_MEADOW1: return 0xFF48780c;
        case TT_MEADOW2: return 0xFF649014;
        case TT_MEADOW3: return 0xFF407008;
        case TT_STEPPE: return 0xFF88b028;
        case TT_MOUNTAINMEADOW: return 0xFF9c8058;
        case TT_WATER: return 0xFF1038a4;
        case TT_LAVA:
        case TT_LAVA2:
        case TT_LAVA3:
        case TT_LAVA4:
            return 0xFFc02020;
        case TT_WATER2: return 0xFF2259EA;
        case TT_BUILDABLE_WATER: return 0xFF1038a4;
        case TT_BUILDABLE_MOUNTAIN: return 0xFF9c8058;
        }
        break;
    case LT_WASTELAND:
        switch (t)
        {
        case TT_SNOW: return 0xFF860000; //TT_SNOW is lava, too
        case TT_DESERT: return 0xFF9c7c64;
        case TT_SWAMPLAND: return 0xFF001820;
        case TT_MEADOW_FLOWERS: return 0xFF444850;
        case TT_MOUNTAIN1: return 0xFF706c54;
        case TT_MOUNTAIN2: return 0xFF706454;
        case TT_MOUNTAIN3: return 0xFF684c24;
        case TT_MOUNTAIN4: return 0xFF684c24;
        case TT_SAVANNAH: return 0xFF444850;
        case TT_MEADOW1: return 0xFF5c5840;
        case TT_MEADOW2: return 0xFF646048;
        case TT_MEADOW3: return 0xFF646048;
        case TT_STEPPE: return 0xFF88b028;
        case TT_MOUNTAINMEADOW: return 0xFF001820;
        case TT_WATER: return 0xFF454520;
        case TT_LAVA:
        case TT_LAVA2:
        case TT_LAVA3:
        case TT_LAVA4:
            return 0xFFC32020;
        case TT_WATER2: return 0xFF454520;
        case TT_BUILDABLE_WATER: return 0xFF454520;
        case TT_BUILDABLE_MOUNTAIN: return 0xFF706454;
        }
        break;
    case LT_WINTERWORLD:
        switch (t)
        {
        case TT_SNOW: return 0xFF00286C;
        case TT_DESERT: return 0xFF0070b0;
        case TT_SWAMPLAND: return 0xFF00286c;
        case TT_MEADOW_FLOWERS: return 0xFF7c84ac;
        case TT_MOUNTAIN1: return 0xFF54586c;
        case TT_MOUNTAIN2: return 0xFF60607c;
        case TT_MOUNTAIN3: return 0xFF686c8c;
        case TT_MOUNTAIN4: return 0xFF686c8c;
        case TT_SAVANNAH: return 0xFFa0accc;
        case TT_MEADOW1: return 0xFFb0a494;
        case TT_MEADOW2: return 0xFF88a874;
        case TT_MEADOW3: return 0xFFa0accc;
        case TT_STEPPE: return 0xFF88b15e;
        case TT_MOUNTAINMEADOW: return 0xFF94a0c0;
        case TT_WATER: return 0xFF1038a4;
        case TT_LAVA:
        case TT_LAVA2:
        case TT_LAVA3:
        case TT_LAVA4:
            return 0xFFc02020;
        case TT_WATER2: return 0xFF1344C4;
        case TT_BUILDABLE_WATER: return 0xFF1038a4;
        case TT_BUILDABLE_MOUNTAIN: return 0xFF60607c;
        }
    }
    throw std::logic_error("Invalid parameters given");
}

EdgeType TerrainData::GetEdgeType(LandscapeType landsCape, TerrainType t)
{
    switch (landsCape)
    {
    case LT_GREENLAND:
        switch (t)
        {
        case TT_SNOW:
            return ET_SNOW;
        case TT_DESERT:
        case TT_STEPPE:
        case TT_SAVANNAH:
            return ET_DESSERT;
        case TT_SWAMPLAND:
        case TT_MEADOW_FLOWERS:
        case TT_MEADOW1:
        case TT_MEADOW2:
        case TT_MEADOW3:
            return ET_MEADOW;
        case TT_MOUNTAIN1:
        case TT_MOUNTAIN2:
        case TT_MOUNTAIN3:
        case TT_MOUNTAIN4:
        case TT_MOUNTAINMEADOW:
        case TT_BUILDABLE_MOUNTAIN:
            return ET_MOUNTAIN;
        case TT_WATER:
        case TT_WATER2:
        case TT_BUILDABLE_WATER:
            return ET_WATER;
        case TT_LAVA:
        case TT_LAVA2:
        case TT_LAVA3:
        case TT_LAVA4:
            return ET_NONE;
        }
        break;
    case LT_WASTELAND:
        switch (t)
        {
        case TT_SNOW:
        case TT_LAVA:
        case TT_LAVA2:
        case TT_LAVA3:
        case TT_LAVA4:
            return ET_NONE;
        case TT_DESERT:
        case TT_STEPPE:
            return ET_DESSERT;
        case TT_SWAMPLAND:
        case TT_MOUNTAINMEADOW:
            return ET_SNOW;
        case TT_MEADOW_FLOWERS:
        case TT_MOUNTAIN1:
        case TT_MOUNTAIN2:
        case TT_MOUNTAIN3:
        case TT_MOUNTAIN4:
        case TT_SAVANNAH:
        case TT_MEADOW1:
        case TT_MEADOW2:
        case TT_MEADOW3:
        case TT_BUILDABLE_MOUNTAIN:
            return ET_MEADOW;
        case TT_WATER:
        case TT_WATER2:
        case TT_BUILDABLE_WATER:
            return ET_MOUNTAIN;
        }
        break;
    case LT_WINTERWORLD:
        switch (t)
        {
        case TT_SNOW:
        case TT_SWAMPLAND:
        case TT_WATER2:
        case TT_WATER:
        case TT_BUILDABLE_WATER:
            return ET_WATER;
        case TT_DESERT:
            return ET_DESSERT;
        case TT_MEADOW_FLOWERS:
            return ET_MEADOW;
        case TT_MOUNTAIN1:
        case TT_MOUNTAIN2:
        case TT_MOUNTAIN3:
        case TT_MOUNTAIN4:
        case TT_BUILDABLE_MOUNTAIN:
            return ET_MOUNTAIN;
        case TT_SAVANNAH:
        case TT_MEADOW1:
        case TT_MEADOW2:
        case TT_MEADOW3:
        case TT_STEPPE:
            return ET_MEADOW;
        case TT_MOUNTAINMEADOW:
            return ET_SNOW;
        case TT_LAVA:
        case TT_LAVA2:
        case TT_LAVA3:
        case TT_LAVA4:
            return ET_NONE;
        }
    }
    throw std::logic_error("Invalid parameters given");
}

/// Draw priorities for terrain pairs (Row = T1, Col = T2)
/// 0: Nothing is drawn above each other (hard edge)
/// 1: T1 draws over T2 (If T1 is inside T2 then you'd get a "circle")
/// -1: T2 draws over T1 (If T1 is inside T2 then you'd get a dented shape)
const signed char TERRAIN_DRAW_PRIORITY[LT_COUNT][TT_COUNT][TT_COUNT] =
{
    // Greenland
    {
        {  },
        { -1 },
        { -1, -1 },
        { -1, -1,  1 },
        { -1, -1,  1,  1 },
        { -1, -1,  1,  1, -1 },
        { -1, -1,  1,  1, -1, -1 },
        { -1, -1,  1,  1, -1, -1, -1 },
        { -1, -1,  1, -1, -1, -1, -1, -1 },
        { -1, -1,  1, -1, -1, -1, -1, -1, -1 },
        { -1, -1,  1, -1, -1, -1, -1, -1, -1, -1 },
        { -1, -1,  1, -1, -1, -1, -1, -1, -1, -1, -1 },
        { -1, -1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1 },
        { -1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1 },
        { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  1, -1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, -1,  0,  1 },
        { -1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  1,  0 },
        { -1, -1,  1,  1,  0,  0,  0,  0,  0,  1,  1,  1, -1,  0,  1,  1, -1, -1 },
        { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0, -1, -1, -1 },
        { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0, -1, -1, -1, 0 },
        { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0, -1, -1, -1, 0, 0 }
    },
    // Wasteland
    {
        {  },
        {  1 },
        {  1,  1 },
        {  1, -1, -1 },
        {  1,  1, -1, -1 },
        {  1,  1, -1, -1,  0 },
        {  1,  1, -1, -1,  0,  0 },
        {  1,  1, -1, -1,  0,  0,  0 },
        {  1, -1, -1,  0,  1,  1,  1,  1 },
        {  1, -1, -1,  0,  1,  1,  1,  1,  0 },
        {  1, -1, -1,  0,  1,  1,  1,  1,  0,  0 },
        {  1, -1, -1,  0,  1,  1,  1,  1,  0,  0,  0 },
        {  1, -1, -1,  1, -1, -1, -1, -1,  1,  1,  1,  1 },
        {  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1 },
        {  1,  1, -1,  1, -1, -1, -1, -1,  1,  1,  1,  1,  1, -1 },
        {  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  1,  1, -1,  1, -1, -1, -1, -1,  1,  1,  1,  1,  1, -1,  0,  1 },
        {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, -1,  0,  0,  0 },
        { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1, -1, -1,  1, -1, -1 },
        {  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0, -1, -1, -1 },
        {  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0, -1, -1, -1, 0 },
        {  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0, -1, -1, -1, 0, 0 }
    },
    // Winterworld
    {
        {  },
        { -1 },
        {  0,  1 },
        { -1, -1, -1 },
        {  1,  1,  1,  1 },
        {  1,  1,  1,  1, -1 },
        {  1,  1,  1,  1, -1, -1 },
        {  1,  1,  1,  1, -1, -1, -1 },
        { -1, -1, -1, -1, -1, -1, -1, -1 },
        { -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        { -1, -1, -1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1 },
        {  0,  1,  0,  1, -1, -1, -1, -1,  1,  1,  1,  1,  1,  1 },
        { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0 },
        {  0,  1,  0,  1, -1, -1, -1, -1,  1,  1,  1,  1,  1,  1,  0,  0 },
        {  0,  1,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  1,  0 },
        { -1, -1, -1, -1, -1,  0,  0,  0, -1, -1, -1, -1, -1, -1, -1,  1, -1, -1 },
        { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0, -1, -1, -1 },
        { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0, -1, -1, -1, 0 },
        { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0, -1, -1, -1, 0, 0 }
    }
};

unsigned char TerrainData::GetEdgeType(LandscapeType landsCape, TerrainType t1, TerrainType t2)
{
    static boost::array< boost::array< boost::array<unsigned char, TT_COUNT>, TT_COUNT >, LT_COUNT > EDGE_TABLE;
    static bool isInitialized = false;
    if(!isInitialized)
    {
        // Init table for faster access
        for(int lt = 0; lt < LT_COUNT; ++lt)
            for(int iT1 = 0; iT1 < TT_COUNT; ++iT1)
                for(int iT2 = 0; iT2<=iT1; ++iT2)
                {
                    EdgeType et1 = GetEdgeType(LandscapeType(lt), TerrainType(iT1));
                    EdgeType et2 = GetEdgeType(LandscapeType(lt), TerrainType(iT2));
                    if(iT1 == iT2 || // Same terrain 
                        !TERRAIN_DRAW_PRIORITY[lt][iT1][iT2]) // Same priority
                    {
                        // -> No overdraw
                        EDGE_TABLE[lt][iT1][iT2] = 0;
                        EDGE_TABLE[lt][iT2][iT1] = 0;
                    }else if(TERRAIN_DRAW_PRIORITY[lt][iT1][iT2] > 0)
                    {
                        // T1 over T2
                        EDGE_TABLE[lt][iT1][iT2] = et1;
                        EDGE_TABLE[lt][iT2][iT1] = 0;
                    }else
                    {
                        // T2 over T1
                        EDGE_TABLE[lt][iT1][iT2] = 0;
                        EDGE_TABLE[lt][iT2][iT1] = et2;
                    }
                }
        isInitialized = true;
    }
    return EDGE_TABLE[landsCape][t1][t2];
}

bool TerrainData::IsUseable(TerrainType t)
{
    if(IsLava(t) || IsWater(t))
        return false;
    switch (t)
    {
    case TT_SNOW:
    case TT_SWAMPLAND:
        return false;
    default:
        return true;
    }
}

bool TerrainData::IsUsableByShip(TerrainType t)
{
    switch (t)
    {
    case TT_WATER:
    case TT_BUILDABLE_WATER:
        return true;
    default:
        return false;
    }
}

bool TerrainData::IsVital(TerrainType t)
{
    switch (t)
    {
    case TT_MEADOW_FLOWERS:
    case TT_SAVANNAH:
    case TT_MEADOW1:
    case TT_MEADOW2:
    case TT_MEADOW3:
    case TT_STEPPE:
        return true;
    default:
        return false;
    }
}

bool TerrainData::IsWater(TerrainType t)
{
    switch (t)
    {
    case TT_WATER:
    case TT_WATER2:
    case TT_BUILDABLE_WATER:
        return true;
    default:
        return false;
    }
}

bool TerrainData::IsLava(TerrainType t)
{
    switch (t)
    {
    case TT_LAVA:
    case TT_LAVA2:
    case TT_LAVA3:
    case TT_LAVA4:
        return true;
    default:
        return false;
    }
}

bool TerrainData::IsMountain(TerrainType t)
{
    switch (t)
    {
    case TT_MOUNTAIN1:
    case TT_MOUNTAIN2:
    case TT_MOUNTAIN3:
    case TT_MOUNTAIN4:
    case TT_MOUNTAINMEADOW:
    case TT_BUILDABLE_MOUNTAIN:
        return true;
    default:
        return false;
    }
}

bool TerrainData::IsMineable(TerrainType t)
{
    switch (t)
    {
    case TT_MOUNTAIN1:
    case TT_MOUNTAIN2:
    case TT_MOUNTAIN3:
    case TT_MOUNTAIN4:
        return true;
    default:
        return false;
    }
}

BuildingQuality TerrainData::GetBuildingQuality(TerrainType t)
{
    switch(t)
    {
    case TT_SNOW:
    case TT_LAVA:
    case TT_LAVA2:
    case TT_LAVA3:
    case TT_LAVA4:
        return BQ_DANGER;
    case TT_DESERT:
        return BQ_FLAG;
    case TT_SWAMPLAND:
    case TT_WATER:
    case TT_WATER2:
        return BQ_NOTHING;
    case TT_MOUNTAIN1:
    case TT_MOUNTAIN2:
    case TT_MOUNTAIN3:
    case TT_MOUNTAIN4:
        return BQ_MINE;
    case TT_MEADOW_FLOWERS:
    case TT_SAVANNAH:
    case TT_MEADOW1:
    case TT_MEADOW2:
    case TT_MEADOW3:
    case TT_STEPPE:
    case TT_MOUNTAINMEADOW:
    case TT_BUILDABLE_WATER:
    case TT_BUILDABLE_MOUNTAIN:
        return BQ_CASTLE;
    }
    throw std::logic_error("Invalid terrain type");
}