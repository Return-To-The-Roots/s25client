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

#ifndef TextureMap_h__
#define TextureMap_h__

#include "randomMaps/terrain/TextureType.h"
#include "randomMaps/terrain/TextureTranslator.h"
#include "gameTypes/MapCoordinates.h"
#include <vector>

class TextureMap
{
private:
    TextureTranslator& translator_;
    
    void FixGrassland(std::vector<TextureType>& textures,
                      const MapExtent& size,
                      TextureType neighbor,
                      TextureType replace);

public:
    TextureMap(TextureTranslator& translator) : translator_(translator) {};
    
    std::vector<TextureType> Create(const std::vector<unsigned char>& heightMap,
                                    const std::vector<bool>& waterMap,
                                    const MapExtent& size,
                                    unsigned char seaLevel,
                                    unsigned char mountainLevel);
};

#endif // TextureMap_h__
