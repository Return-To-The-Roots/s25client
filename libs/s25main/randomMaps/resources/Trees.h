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

#ifndef Trees_h__
#define Trees_h__

#include "randomMaps/algorithm/RandomUtility.h"
#include "randomMaps/Map.h"

class Trees
{
private:
    
    RandomUtility& rnd_;
    
    std::vector<int> GetTreeTypes(TextureType texture);
    std::vector<int> GetTreeInfos(TextureType texture);
    
    int GetProbability(TextureType texture, int distance);

public:
        
    Trees(RandomUtility& rnd) : rnd_(rnd) {}

    void Generate(Map& map, const std::vector<int>& freeZone);
};

#endif // Trees_h__
