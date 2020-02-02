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

#ifndef ValueMap_h__
#define ValueMap_h__

#include "world/NodeMapBase.h"
#include <cmath>

namespace rttr {
namespace mapGenerator {

template<typename T_Value>
struct ValueRange
{
    ValueRange(T_Value minimum, T_Value maximum) :
        minimum(minimum), maximum(maximum)
    {
        
    }
    
    const T_Value minimum;
    const T_Value maximum;
    
    /**
     * Computes the difference between maximum and minimum value.
     * @returns the difference between maximum and minimum.
     */
    T_Value GetDifference() const
    {
        return maximum - minimum;
    }
};

template<typename T_Value>
class ValueMap : public NodeMapBase<T_Value>
{
public:
    
    /**
     * Finds the maximum value of the map.
     * @returns the maximum value of the map.
     */
    const T_Value& GetMaximum() const
    {
        return *std::max_element(this->nodes.begin(), this->nodes.end());
    }
    
    /**
     * Finds the point with the maximum value on the map.
     * @returns the point which contains the maximum value.
     */
    MapPoint GetMaximumPoint() const
    {
        auto maximum = std::max_element(this->nodes.begin(), this->nodes.end());
        auto index = std::distance(this->nodes.begin(), maximum);
        
        return MapPoint(index % this->GetWidth(), index / this->GetWidth());
    }
    
    /**
     * Computes the range of values covered by the map.
     * @returns range of values covered by the map.
     */
    ValueRange<T_Value> GetRange() const
    {
        auto range = std::minmax_element(this->nodes.begin(), this->nodes.end());

        return ValueRange<T_Value>(*range.first, *range.second);
    }
};

}}

#endif // ValueMap_h__
