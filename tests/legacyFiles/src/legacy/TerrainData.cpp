// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "TerrainData.h"
#include "helpers/MultiArray.h"
#include <array>
#include <cstdint>
#include <iostream>

unsigned char TerrainData::GetTextureIdentifier(TerrainType t)
{
    switch(t)
    {
        case TT_SNOW: return 0x02;
        case TT_DESERT: return 0x04;
        case TT_SWAMPLAND: return 0x03;
        case TT_MEADOW_FLOWERS: return 0x0F;
        case TT_MOUNTAIN1: return 0x01;
        case TT_MOUNTAIN2: return 0x0B;
        case TT_MOUNTAIN3: return 0x0C;
        case TT_MOUNTAIN4: return 0x0D;
        case TT_SAVANNAH: return 0x00;
        case TT_MEADOW1: return 0x08;
        case TT_MEADOW2: return 0x09;
        case TT_MEADOW3: return 0x0A;
        case TT_STEPPE: return 0x0E;
        case TT_MOUNTAINMEADOW: return 0x12;
        case TT_WATER: return 0x05;
        case TT_WATER_NOSHIP: return 0x13;
        case TT_BUILDABLE_MOUNTAIN: return 0x22;
        case TT_BUILDABLE_WATER: return 0x06;
        case TT_LAVA: return 0x10;
        case TT_LAVA2: return 0x14;
        case TT_LAVA3: return 0x15;
        case TT_LAVA4: return 0x16;
    }
    throw std::logic_error("Invalid parameters given");
}

TerrainType TerrainData::MapIdx2Terrain(unsigned char mapIdx)
{
    // Unset bit 0x40 (harbour) and 0x80 (unknown)
    mapIdx &= ~(0x40 | 0x80);
    switch(mapIdx)
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
        case 19: return TT_WATER_NOSHIP;
        case 20: return TT_LAVA2;
        case 21: return TT_LAVA3;
        case 22: return TT_LAVA4;
        case 34: return TT_BUILDABLE_MOUNTAIN;
        default:
            RTTR_Assert(false); // Unknown/invalid texture
            return TT_SNOW;
    }
}

Rect TerrainData::GetPosInTexture(TerrainType t)
{
    switch(t)
    {
        case TT_SNOW: return Rect(0, 0, 30, 30);
        case TT_DESERT: return Rect(48, 0, 48, 48);
        case TT_SWAMPLAND: return Rect(96, 0, 30, 30);
        case TT_MEADOW_FLOWERS: return Rect(144, 0, 42, 42);
        case TT_MOUNTAIN1: return Rect(0, 48, 48, 48);
        case TT_MOUNTAIN2:
        case TT_BUILDABLE_MOUNTAIN: return Rect(48, 48, 48, 48);
        case TT_MOUNTAIN3: return Rect(96, 48, 48, 48);
        case TT_MOUNTAIN4: return Rect(144, 48, 48, 48);
        case TT_SAVANNAH: return Rect(0, 96, 48, 48);
        case TT_MEADOW1: return Rect(48, 96, 48, 48);
        case TT_MEADOW2: return Rect(96, 96, 48, 48);
        case TT_MEADOW3: return Rect(144, 96, 48, 48);
        case TT_STEPPE: return Rect(0, 144, 40, 40);
        case TT_MOUNTAINMEADOW: return Rect(48, 144, 30, 30);
        case TT_WATER:
        case TT_WATER_NOSHIP:
        case TT_BUILDABLE_WATER: return Rect(192, 48, 55, 56);
        case TT_LAVA: return Rect(192, 104, 55, 56);
        case TT_LAVA2: return Rect(66, 223, 31, 32);
        case TT_LAVA3: return Rect(99, 223, 31, 32);
        case TT_LAVA4: return Rect(132, 223, 31, 32);
    }
    throw std::logic_error("Invalid parameters given");
}

bool TerrainData::IsAnimated(TerrainType t)
{
    return IsWater(t) || IsLava(t);
}

unsigned TerrainData::GetNumFrames(TerrainType t)
{
    switch(t)
    {
        case TT_WATER:
        case TT_WATER_NOSHIP:
        case TT_BUILDABLE_WATER: return 8;
        case TT_LAVA:
        case TT_LAVA2:
        case TT_LAVA3:
        case TT_LAVA4: return 4;
        default: return 1;
    }
}

unsigned char TerrainData::GetStartColor(TerrainType t)
{
    switch(t)
    {
        case TT_WATER:
        case TT_WATER_NOSHIP:
        case TT_BUILDABLE_WATER: return 240;
        case TT_LAVA:
        case TT_LAVA2:
        case TT_LAVA3:
        case TT_LAVA4: return 248;
        default: return 0;
    }
}

unsigned TerrainData::GetColor(Landscape landsCape, TerrainType t)
{
    switch(landsCape)
    {
        case Landscape::GREENLAND:
            switch(t)
            {
                case TT_SNOW: return 0xFFFFFFFF;
                case TT_DESERT: return 0xFFc09c7c;
                case TT_SWAMPLAND: return 0xFF649014;
                case TT_MEADOW_FLOWERS: return 0xFF48780c;
                case TT_MOUNTAIN1:
                case TT_MOUNTAIN2:
                case TT_MOUNTAIN3:
                case TT_BUILDABLE_MOUNTAIN:
                case TT_MOUNTAINMEADOW: return 0xFF9c8058;
                case TT_MOUNTAIN4: return 0xFF8c7048;
                case TT_SAVANNAH: return 0xFF649014;
                case TT_MEADOW1: return 0xFF48780c;
                case TT_MEADOW2: return 0xFF649014;
                case TT_MEADOW3: return 0xFF407008;
                case TT_STEPPE: return 0xFF88b028;
                case TT_WATER:
                case TT_WATER_NOSHIP:
                case TT_BUILDABLE_WATER: return 0xFF1038a4;
                case TT_LAVA:
                case TT_LAVA2:
                case TT_LAVA3:
                case TT_LAVA4: return 0xFFc02020;
            }
            break;
        case Landscape::WASTELAND:
            switch(t)
            {
                case TT_SNOW: return 0xFF860000; // TT_SNOW is lava, too
                case TT_DESERT: return 0xFF9c7c64;
                case TT_SWAMPLAND: return 0xFF001820;
                case TT_MEADOW_FLOWERS: return 0xFF444850;
                case TT_MOUNTAIN1: return 0xFF706c54;
                case TT_MOUNTAIN2: return 0xFF706454;
                case TT_MOUNTAIN3:
                case TT_MOUNTAIN4: return 0xFF684c24;
                case TT_SAVANNAH: return 0xFF444850;
                case TT_MEADOW1: return 0xFF5c5840;
                case TT_MEADOW2:
                case TT_MEADOW3: return 0xFF646048;
                case TT_STEPPE: return 0xFF88b028;
                case TT_MOUNTAINMEADOW: return 0xFF001820;
                case TT_WATER:
                case TT_WATER_NOSHIP:
                case TT_BUILDABLE_WATER: return 0xFF454520;
                case TT_LAVA:
                case TT_LAVA2:
                case TT_LAVA3:
                case TT_LAVA4: return 0xFFC32020;
                case TT_BUILDABLE_MOUNTAIN: return 0xFF706454;
            }
            break;
        case Landscape::WINTERWORLD:
            switch(t)
            {
                case TT_SNOW: return 0xFF00286C;
                case TT_DESERT: return 0xFF0070b0;
                case TT_SWAMPLAND: return 0xFF00286c;
                case TT_MEADOW_FLOWERS: return 0xFF7c84ac;
                case TT_MOUNTAIN1: return 0xFF54586c;
                case TT_MOUNTAIN2: return 0xFF60607c;
                case TT_MOUNTAIN3:;
                case TT_MOUNTAIN4: return 0xFF686c8c;
                case TT_SAVANNAH: return 0xFFa0accc;
                case TT_MEADOW1: return 0xFFb0a494;
                case TT_MEADOW2: return 0xFF88a874;
                case TT_MEADOW3: return 0xFFa0accc;
                case TT_STEPPE: return 0xFF88b15e;
                case TT_MOUNTAINMEADOW: return 0xFF94a0c0;
                case TT_WATER:
                case TT_WATER_NOSHIP:
                case TT_BUILDABLE_WATER: return 0xFF1038a4;
                case TT_LAVA:
                case TT_LAVA2:
                case TT_LAVA3:
                case TT_LAVA4: return 0xFFc02020;
                case TT_BUILDABLE_MOUNTAIN: return 0xFF60607c;
            }
    }
    throw std::logic_error("Invalid parameters given");
}

EdgeType TerrainData::GetEdgeType(Landscape landsCape, TerrainType t)
{
    switch(landsCape)
    {
        case Landscape::GREENLAND:
            switch(t)
            {
                case TT_SNOW: return ET_SNOW;
                case TT_DESERT:
                case TT_STEPPE:
                case TT_SAVANNAH: return ET_DESSERT;
                case TT_SWAMPLAND:
                case TT_MEADOW_FLOWERS:
                case TT_MEADOW1:
                case TT_MEADOW2:
                case TT_MEADOW3: return ET_MEADOW;
                case TT_MOUNTAIN1:
                case TT_MOUNTAIN2:
                case TT_MOUNTAIN3:
                case TT_MOUNTAIN4:
                case TT_MOUNTAINMEADOW:
                case TT_BUILDABLE_MOUNTAIN: return ET_MOUNTAIN;
                case TT_WATER:
                case TT_WATER_NOSHIP:
                case TT_BUILDABLE_WATER: return ET_WATER;
                case TT_LAVA:
                case TT_LAVA2:
                case TT_LAVA3:
                case TT_LAVA4: return ET_NONE;
            }
            break;
        case Landscape::WASTELAND:
            switch(t)
            {
                case TT_SNOW:
                case TT_LAVA:
                case TT_LAVA2:
                case TT_LAVA3:
                case TT_LAVA4: return ET_NONE;
                case TT_DESERT:
                case TT_STEPPE: return ET_DESSERT;
                case TT_SWAMPLAND:
                case TT_MOUNTAINMEADOW: return ET_SNOW;
                case TT_MEADOW_FLOWERS:
                case TT_MOUNTAIN1:
                case TT_MOUNTAIN2:
                case TT_MOUNTAIN3:
                case TT_MOUNTAIN4:
                case TT_SAVANNAH:
                case TT_MEADOW1:
                case TT_MEADOW2:
                case TT_MEADOW3:
                case TT_BUILDABLE_MOUNTAIN: return ET_MEADOW;
                case TT_WATER:
                case TT_WATER_NOSHIP:
                case TT_BUILDABLE_WATER: return ET_MOUNTAIN;
            }
            break;
        case Landscape::WINTERWORLD:
            switch(t)
            {
                case TT_SNOW:
                case TT_SWAMPLAND:
                case TT_WATER_NOSHIP:
                case TT_WATER:
                case TT_BUILDABLE_WATER: return ET_WATER;
                case TT_DESERT: return ET_DESSERT;
                case TT_MEADOW_FLOWERS: return ET_MEADOW;
                case TT_MOUNTAIN1:
                case TT_MOUNTAIN2:
                case TT_MOUNTAIN3:
                case TT_MOUNTAIN4:
                case TT_BUILDABLE_MOUNTAIN: return ET_MOUNTAIN;
                case TT_SAVANNAH:
                case TT_MEADOW1:
                case TT_MEADOW2:
                case TT_MEADOW3:
                case TT_STEPPE: return ET_MEADOW;
                case TT_MOUNTAINMEADOW: return ET_SNOW;
                case TT_LAVA:
                case TT_LAVA2:
                case TT_LAVA3:
                case TT_LAVA4: return ET_NONE;
            }
    }
    throw std::logic_error("Invalid parameters given");
}

/// Draw priorities for terrain pairs (Row = T1, Col = T2)
/// 0: Nothing is drawn above each other (hard edge)
/// 1: T1 draws over T2 (If T1 is inside T2 then you'd get a "circle")
/// -1: T2 draws over T1 (If T1 is inside T2 then you'd get a dented shape)
const helpers::MultiArray<int8_t, NUM_LTS, NUM_TTS, NUM_TTS> TERRAIN_DRAW_PRIORITY{
  {// Greenland
   {/*00 TT_SNOW*/ {},
    /*01 TT_DESERT*/ {-1},
    /*02 TT_SWAMPLAND*/ {-1, -1},
    /*03 TT_MEADOW_FLOWERS*/ {-1, -1, 1},
    /*04 TT_MOUNTAIN1*/ {-1, -1, 1, 1},
    /*05 TT_MOUNTAIN2*/ {-1, -1, 1, 1, -1},
    /*06 TT_MOUNTAIN3*/ {-1, -1, 1, 1, -1, -1},
    /*07 TT_MOUNTAIN4*/ {-1, -1, 1, 1, -1, -1, -1},
    /*08 TT_SAVANNAH*/ {-1, -1, 1, -1, -1, -1, -1, -1},
    /*09 TT_MEADOW1*/ {-1, -1, 1, -1, -1, -1, -1, -1, 1},
    /*10 TT_MEADOW2*/ {-1, -1, 1, -1, -1, -1, -1, -1, 1, -1},
    /*11 TT_MEADOW3*/ {-1, -1, 1, -1, -1, -1, -1, -1, 1, -1, -1},
    /*12 TT_STEPPE*/ {-1, -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    /*13 TT_MOUNTAINMEADOW*/ {-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    /*14 TT_WATER*/ {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    /*15 TT_LAVA*/ {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    /*16 TT_WATER_NOSHIP*/ {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
    /*17 TT_BUILDABLE_WATER*/ {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0},
    /*18 TT_BUILDABLE_MOUNTAIN*/ {-1, -1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, -1, 0, 1, 1, -1, -1},
    /*19 TT_LAVA2*/ {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1},
    /*20 TT_LAVA3*/ {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0},
    /*21 TT_LAVA4*/ {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0}},
   // Wasteland
   {/*TT_SNOW*/ {},
    /*TT_DESERT*/ {1},
    /*TT_SWAMPLAND*/ {1, 1},
    /*TT_MEADOW_FLOWERS*/ {1, -1, -1},
    /*TT_MOUNTAIN1*/ {1, 1, -1, -1},
    /*TT_MOUNTAIN2*/ {1, 1, -1, -1, -1},
    /*TT_MOUNTAIN3*/ {1, 1, -1, -1, -1, 0},
    /*TT_MOUNTAIN4*/ {1, 1, -1, -1, -1, -1, 0},
    /*TT_SAVANNAH*/ {1, -1, -1, -1, 1, 1, 1, 1},
    /*TT_MEADOW1*/ {1, -1, -1, -1, 1, 1, 1, 1, -1},
    /*TT_MEADOW2*/ {1, -1, -1, -1, 1, 1, 1, 1, -1, -1},
    /*TT_MEADOW3*/ {1, -1, -1, -1, 1, 1, 1, 1, -1, -1, -1},
    /*TT_STEPPE*/ {1, -1, -1, 1, -1, -1, -1, -1, 1, 1, 1, 1},
    /*TT_MOUNTAINMEADOW*/ {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    /*TT_WATER*/ {1, 1, -1, 1, -1, -1, -1, -1, 1, 1, 1, 1, 1, -1},
    /*TT_LAVA*/ {0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    /*TT_WATER_NOSHIP*/ {1, 1, -1, 1, -1, -1, -1, -1, 1, 1, 1, 1, 1, -1, -1, 1},
    /*TT_BUILDABLE_WATER*/ {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 1},
    /*TT_BUILDABLE_MOUNTAIN*/ {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1, -1, -1, 1, -1, -1},
    /*TT_LAVA2*/ {0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1},
    /*TT_LAVA3*/ {0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0},
    /*TT_LAVA4*/ {0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0}},
   // Winterworld
   {/*TT_SNOW*/ {},
    /*TT_DESERT*/ {-1},
    /*TT_SWAMPLAND*/ {0, 1},
    /*TT_MEADOW_FLOWERS*/ {-1, -1, -1},
    /*TT_MOUNTAIN1*/ {1, 1, 1, 1},
    /*TT_MOUNTAIN2*/ {1, 1, 1, 1, -1},
    /*TT_MOUNTAIN3*/ {1, 1, 1, 1, -1, -1},
    /*TT_MOUNTAIN4*/ {1, 1, 1, 1, -1, -1, -1},
    /*TT_SAVANNAH*/ {-1, -1, -1, -1, -1, -1, -1, -1},
    /*TT_MEADOW1*/ {-1, -1, -1, -1, -1, -1, -1, -1, -1},
    /*TT_MEADOW2*/ {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    /*TT_MEADOW3*/ {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    /*TT_STEPPE*/ {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    /*TT_MOUNTAINMEADOW*/ {-1, -1, -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    /*TT_WATER*/ {0, 1, 0, 1, -1, -1, -1, -1, 1, 1, 1, 1, 1, 1},
    /*TT_LAVA*/ {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0},
    /*TT_WATER_NOSHIP*/ {0, 1, 0, 1, -1, -1, -1, -1, 1, 1, 1, 1, 1, 1, -1, 0},
    /*TT_BUILDABLE_WATER*/ {0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1},
    /*TT_BUILDABLE_MOUNTAIN*/ {-1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, 1, -1, -1},
    /*TT_LAVA2*/ {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1},
    /*TT_LAVA3*/ {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0},
    /*TT_LAVA4*/ {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0}}}};

unsigned char TerrainData::GetEdgeType(Landscape landsCape, TerrainType t1, TerrainType t2)
{
    static std::array<std::array<std::array<unsigned char, NUM_TTS>, NUM_TTS>, NUM_LTS> EDGE_TABLE;
    static bool isInitialized = false;
    if(!isInitialized)
    {
        // Init table for faster access
        for(int lt = 0; lt < NUM_LTS; ++lt)
            for(int iT1 = 0; iT1 < NUM_TTS; ++iT1)
                for(int iT2 = 0; iT2 <= iT1; ++iT2)
                {
                    EdgeType et1 = GetEdgeType(Landscape(lt), TerrainType(iT1));
                    EdgeType et2 = GetEdgeType(Landscape(lt), TerrainType(iT2));
                    if(iT1 == iT2 ||                         // Same terrain
                       !TERRAIN_DRAW_PRIORITY[lt][iT1][iT2]) // Same priority
                    {
                        // -> No overdraw
                        EDGE_TABLE[lt][iT1][iT2] = 0;
                        EDGE_TABLE[lt][iT2][iT1] = 0;
                    } else if(TERRAIN_DRAW_PRIORITY[lt][iT1][iT2] > 0)
                    {
                        // T1 over T2
                        EDGE_TABLE[lt][iT1][iT2] = et1;
                        EDGE_TABLE[lt][iT2][iT1] = 0;
                    } else
                    {
                        // T2 over T1
                        EDGE_TABLE[lt][iT1][iT2] = 0;
                        EDGE_TABLE[lt][iT2][iT1] = et2;
                    }
                }
        isInitialized = true;
    }
    return EDGE_TABLE[static_cast<uint8_t>(landsCape)][t1][t2];
}

static void CheckPriorities(Landscape lt, const std::array<int, NUM_TTS>& terrainPrios)
{
    for(int iT1 = 0; iT1 < NUM_TTS; ++iT1)
        std::cout << iT1 << ": " << terrainPrios[iT1] << std::endl;
    unsigned numWrong = 0;
    for(int iT1 = 0; iT1 < NUM_TTS; ++iT1)
    {
        for(int iT2 = 0; iT2 < NUM_TTS; ++iT2)
        {
            unsigned oldEdge = TerrainData::GetEdgeType(lt, TerrainType(iT1), TerrainType(iT2));
            unsigned newEdge;
            if(terrainPrios[iT1] > terrainPrios[iT2])
                newEdge = TerrainData::GetEdgeType(lt, TerrainType(iT1));
            else
                newEdge = 0;
            if(oldEdge != newEdge)
            {
                std::cout << iT1 << "x" << iT2 << ": " << newEdge << "!=" << oldEdge << std::endl;
                numWrong++;
            }
        }
    }
    std::cout << numWrong << " different entries" << std::endl;
}

const std::array<int, NUM_TTS>& TerrainData::GetEdgePrios(Landscape landsCape)
{
    static const std::array<int, NUM_TTS> prioGL{};
    static const std::array<int, NUM_TTS> prioWL{};
    static const std::array<int, NUM_TTS> prioWW{};
    switch(landsCape)
    {
        default:
        case Landscape::GREENLAND: return prioGL;
        case Landscape::WASTELAND: return prioWL;
        case Landscape::WINTERWORLD: return prioWW;
    }
}

void TerrainData::PrintEdgePrios()
{
    for(int lt = 0; lt < NUM_LTS; ++lt)
    {
        std::cout << "Calculation landscape " << lt << std::endl;
        CheckPriorities(Landscape(lt), GetEdgePrios(Landscape(lt)));
    }
}

bool TerrainData::IsUseable(TerrainType t)
{
    TerrainBQ bq = GetBuildingQuality(t);
    return (bq != TerrainBQ::Nothing && bq != TerrainBQ::Danger);
}

bool TerrainData::IsUsableByShip(TerrainType t)
{
    switch(t)
    {
        case TT_WATER: return true;
        default: return false;
    }
}

bool TerrainData::IsUsableByAnimals(TerrainType t)
{
    switch(t)
    {
        case TT_MEADOW_FLOWERS:
        case TT_SAVANNAH:
        case TT_MEADOW1:
        case TT_MEADOW2:
        case TT_MEADOW3:
        case TT_STEPPE:
        case TT_MOUNTAINMEADOW:
        case TT_BUILDABLE_MOUNTAIN: return true;
        default: return false;
    }
}

bool TerrainData::IsVital(TerrainType t)
{
    switch(t)
    {
        case TT_MEADOW_FLOWERS:
        case TT_SAVANNAH:
        case TT_MEADOW1:
        case TT_MEADOW2:
        case TT_MEADOW3:
        case TT_STEPPE: return true;
        default: return false;
    }
}

bool TerrainData::IsWater(TerrainType t)
{
    switch(t)
    {
        case TT_WATER:
        case TT_WATER_NOSHIP:
        case TT_BUILDABLE_WATER: return true;
        default: return false;
    }
}

bool TerrainData::IsLava(TerrainType t)
{
    switch(t)
    {
        case TT_LAVA:
        case TT_LAVA2:
        case TT_LAVA3:
        case TT_LAVA4: return true;
        default: return false;
    }
}

bool TerrainData::IsSnow(Landscape lt, TerrainType t)
{
    switch(lt)
    {
        case Landscape::GREENLAND: return t == TT_SNOW;
        case Landscape::WASTELAND: return false;
        case Landscape::WINTERWORLD: return t == TT_MOUNTAINMEADOW; break;
    }
    throw std::logic_error("Invalid terrain type");
}

bool TerrainData::IsMountain(TerrainType t)
{
    switch(t)
    {
        case TT_MOUNTAIN1:
        case TT_MOUNTAIN2:
        case TT_MOUNTAIN3:
        case TT_MOUNTAIN4:
        case TT_MOUNTAINMEADOW:
        case TT_BUILDABLE_MOUNTAIN: return true;
        default: return false;
    }
}

bool TerrainData::IsMineable(TerrainType t)
{
    return GetBuildingQuality(t) == TerrainBQ::Mine;
}

TerrainBQ TerrainData::GetBuildingQuality(TerrainType t)
{
    switch(t)
    {
        case TT_SNOW:
        case TT_LAVA:
        case TT_LAVA2:
        case TT_LAVA3:
        case TT_LAVA4: return TerrainBQ::Danger;
        case TT_DESERT: return TerrainBQ::Flag;
        case TT_SWAMPLAND:
        case TT_WATER:
        case TT_WATER_NOSHIP: return TerrainBQ::Nothing;
        case TT_MOUNTAIN1:
        case TT_MOUNTAIN2:
        case TT_MOUNTAIN3:
        case TT_MOUNTAIN4: return TerrainBQ::Mine;
        case TT_MEADOW_FLOWERS:
        case TT_SAVANNAH:
        case TT_MEADOW1:
        case TT_MEADOW2:
        case TT_MEADOW3:
        case TT_STEPPE:
        case TT_MOUNTAINMEADOW:
        case TT_BUILDABLE_WATER:
        case TT_BUILDABLE_MOUNTAIN: return TerrainBQ::Castle;
    }
    throw std::logic_error("Invalid terrain type");
}
