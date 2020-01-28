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

#ifndef TextureType_h__
#define TextureType_h__

enum TextureType
{                   // Green  Waste   Winter
    None = 0,
    Water,          // 0x05   0x05    0x05
    Coast,          // 0x04   0x04    0x04
    CoastToGreen1,  // 0x0E   0x0E    0x12
    CoastToGreen2,  // 0x00   0x00    0x00
    Grass1,         // 0x08   0x08    0x08
    Grass2,         // 0x09   0x09    0x09
    Grass3,         // 0x0A   0x0A    0x0A
    GrassFlower,    // 0x0F   0x0F    0x0F
    GrassToMountain,// 0x12   0x00    0x0F
    Mountain1,      // 0x01   0x01    0x01
    Mountain2,      // 0x0B   0x0B    0x0B
    Mountain3,      // 0x0C   0x0C    0x0C
    Mountain4,      // 0x0D   0x0D    0x0D
    MountainPeak,   // 0x02   0x10    0x03
    Lava            // 0x10   0x10    0x10
};

class Texture
{
public:

    static bool IsBuildable(TextureType texture)
    {
        switch (texture)
        {
            case CoastToGreen1:
            case CoastToGreen2:
            case Grass1:
            case Grass2:
            case Grass3:
            case GrassFlower:
                return true;
            default:
                return false;
        }
    }
    
    static bool IsGrass(TextureType texture)
    {
        switch (texture)
        {
            case Grass1:
            case Grass2:
            case Grass3:
            case GrassFlower:
                return true;
            default:
                return false;
        }
    }
    
    static bool IsMountain(TextureType texture)
    {
        switch (texture)
        {
            case Mountain1:
            case Mountain2:
            case Mountain3:
            case Mountain4:
            case MountainPeak:
                return true;
            default:
                return false;
        }
    }
    
    static bool IsWater(TextureType texture)
    {
        return texture == Water;
    }
};

#endif // TextureType_h__
