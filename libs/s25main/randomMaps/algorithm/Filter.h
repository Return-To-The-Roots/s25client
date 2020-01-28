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

#ifndef Filter_h__
#define Filter_h__

#include "randomMaps/algorithm/GridPredicate.h"
#include "gameTypes/MapCoordinates.h"
#include <vector>

class Filter
{
public:
    Filter() {}
    virtual ~Filter() {}
    virtual std::vector<Position> Apply(const std::vector<Position>& input) = 0;
};

class ConditionFilter : public Filter
{
private:
    GridPredicate& condition_;
    MapExtent size_;

public:
    ConditionFilter(GridPredicate& condition, const MapExtent& size) : condition_(condition), size_(size) {}
    ~ConditionFilter() {}
    std::vector<Position> Apply(const std::vector<Position>& input)
    {
        std::vector<Position> result;
        
        for (auto point = input.begin(); point != input.end(); ++point)
        {
            if (condition_.Check(*point, size_))
            {
                result.push_back(*point);
            }
        }
        
        return result;
    }
};


class ItemFilter : public Filter
{
private:
    std::vector<Position> remove_;
    
public:
    ItemFilter(const std::vector<Position>& remove) : remove_(remove) {}
    ~ItemFilter() {}
    std::vector<Position> Apply(const std::vector<Position>& input)
    {
        std::vector<Position> result;
        
        for (auto point = input.begin(); point != input.end(); ++point)
        {
            if (std::find(remove_.begin(), remove_.end(), *point) == remove_.end())
            {
                result.push_back(*point);
            }
        }
        
        return result;
    }
};

#endif // Filter_h__
