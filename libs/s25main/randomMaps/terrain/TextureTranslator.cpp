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
#include "randomMaps/terrain/TextureTranslator.h"
#include <stdexcept>

TextureType TextureTranslator::CoastToMountain(double normalized)
{
    switch (int(normalized * 4))
    {
        case 0:  return Grass1;
        case 1:  return Grass2;
        case 2:  return Grass3;
        case 3:  return GrassFlower;
        default: return Mountain1;
    }
}

TextureType TextureTranslator::MountainToPeak(double normalized)
{
    switch (int(normalized * 5))
    {
        case 0:  return Mountain1;
        case 1:  return Mountain2;
        case 2:  return Mountain3;
        case 3:  return Mountain4;
        default: return MountainPeak;
    }
}

TextureType TextureTranslator::GetTexture(unsigned char height,
                                          unsigned char seaLevel,
                                          unsigned char mountainLevel)
{
    if (mountainLevel <= seaLevel)
    {
        throw std::invalid_argument("Mountain level must be greater than sea level.");
    }
    
    if (height <= seaLevel)
    {
        return Water;
    }
    
    if (height < mountainLevel)
    {
        double minimum = seaLevel + 1;
        double maximum = mountainLevel;
        
        return CoastToMountain((height - minimum) / (maximum - minimum));
    }

    double minimum = mountainLevel;
    double maximum = height_.maximum;

    return MountainToPeak((height - minimum) / (maximum - minimum));
}
