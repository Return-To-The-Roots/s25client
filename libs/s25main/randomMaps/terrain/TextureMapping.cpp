// Copyright (c) 2017 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "rttrDefines.h" // IWYU pragma: keep
#include "randomMaps/terrain/TextureMapping.h"

unsigned char GreenlandMapping::GetType()
{
    return 0x0;
}

unsigned char GreenlandMapping::Get(TextureType texture)
{
    switch (texture)
    {
        case Water:         return 0x05;
        case Coast:         return 0x04;
        case CoastToGreen1: return 0x0E;
        case CoastToGreen2: return 0x00;
        case Grass1:        return 0x08;
        case Grass2:        return 0x09;
        case Grass3:        return 0x0A;
        case GrassFlower:   return 0x0F;
        case GrassToMountain: return 0x12;
        case Mountain1:     return 0x01;
        case Mountain2:     return 0x0B;
        case Mountain3:     return 0x0C;
        case Mountain4:     return 0x0D;
        case MountainPeak:  return 0x02;
        case Lava:          return 0x10;
        default:            return 0x8;
    }
}

unsigned char WastelandMapping::GetType()
{
    return 0x1;
}

unsigned char WastelandMapping::Get(TextureType texture)
{
    switch (texture)
    {
        case Water:         return 0x05;
        case Coast:         return 0x04;
        case CoastToGreen1: return 0x0E;
        case CoastToGreen2: return 0x00;
        case Grass1:        return 0x08;
        case Grass2:        return 0x09;
        case Grass3:        return 0x0A;
        case GrassFlower:   return 0x0F;
        case GrassToMountain: return 0x00;
        case Mountain1:     return 0x01;
        case Mountain2:     return 0x0B;
        case Mountain3:     return 0x0C;
        case Mountain4:     return 0x0D;
        case MountainPeak:  return 0x10;
        case Lava:          return 0x10;
        default:            return 0x8;
    }
}

unsigned char WinterMapping::GetType()
{
    return 0x2;
}

unsigned char WinterMapping::Get(TextureType texture)
{
    switch (texture)
    {
        case Water:         return 0x05;
        case Coast:         return 0x04;
        case CoastToGreen1: return 0x12;
        case CoastToGreen2: return 0x00;
        case Grass1:        return 0x08;
        case Grass2:        return 0x09;
        case Grass3:        return 0x0A;
        case GrassFlower:   return 0x0F;
        case GrassToMountain: return 0x0F;
        case Mountain1:     return 0x01;
        case Mountain2:     return 0x0B;
        case Mountain3:     return 0x0C;
        case Mountain4:     return 0x0D;
        case MountainPeak:  return 0x03;
        case Lava:          return 0x10;
        default:            return 0x8;
    }
}
