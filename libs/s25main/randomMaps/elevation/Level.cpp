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
#include "randomMaps/elevation/Level.h"

int Level::CountBelowOrEqual(const std::vector<unsigned char>& z,
                             unsigned char level)
{
    int count = 0;
    for (unsigned char height : z)
    {
        if (height <= level)
        {
            count++;
        }
    }
    return count;
}

unsigned char Level::Water(const std::vector<unsigned char>& z, double coverage)
{
    int nodes = z.size();
    int count = 0;
    unsigned char seaLevel = 0;
    double ratio = 0.0;
    
    while (ratio < coverage && count != nodes)
    {
        count = 0;
        for (int i = 0; i < nodes; ++i)
        {
            if (z[i] <= seaLevel)
            {
                count++;
            }
        }
        ratio = double(count) / nodes;
        seaLevel++;
    }
    return seaLevel == 0 ? 0 : seaLevel - 1;
}

unsigned char Level::Mountain(const std::vector<unsigned char>& z, double coverage)
{
    std::set<int> indices;
    
    for (int i = 0; i < (int)z.size(); ++i)
    {
        indices.insert(i);
    }
    
    return Mountain(z, coverage, indices);
}

unsigned char Level::Mountain(const std::vector<unsigned char>& z, double coverage, const std::set<int>& subset)
{
    int nodes = subset.size();
    int count = 0;
    unsigned char mountainLevel = 0;
    double ratio = 1.0;
    
    while (ratio > coverage)
    {
        count = 0;
        for (auto i: subset)
        {
            if (z[i] >= mountainLevel)
            {
                count++;
            }
        }
        ratio = double(count) / nodes;
        mountainLevel++;
    }
    
    return mountainLevel == 0 ? 0 : mountainLevel - 1;
}
