// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation,  either version 2 of the License,  or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not,  see <http://www.gnu.org/licenses/>.

#pragma once

#include <vector>

struct GetEstimateFromPtr
{
    template<typename T>
    static inline unsigned GetValue(T* el)
    {
        return el->estimate;
    }
};

/// A priority queue based on an unsorted vector with same interface as OpenListPrioQueue
/// Requires a policy that returns the value on which elements should be ordered from the element
/// Note: Order of elements with same value is determined by push/pop operations
template<class T, class T_GetOrderValue = GetEstimateFromPtr>
class OpenListVector
{
    std::vector<T> elements;

public:
    OpenListVector() { elements.reserve(255); }

    T pop()
    {
        RTTR_Assert(!empty());
        const int size = static_cast<int>(elements.size());
        if(size == 1)
        {
            T best = elements.front();
            elements.clear();
            return best;
        }
        int bestIdx = 0;
        unsigned bestEstimate = T_GetOrderValue::GetValue(elements.front());
        for(int i = 1; i < size; i++)
        {
            // Note that this check does not consider nodes with the same value
            // However this is a) correct (same estimate = same quality so no preference from the algorithm)
            // and b) still fully deterministic as the entries are NOT sorted and the insertion-extraction-pattern
            // is completely pre-determined by the graph-structur
            const unsigned estimate = T_GetOrderValue::GetValue(elements[i]);
            if(estimate < bestEstimate)
            {
                bestEstimate = estimate;
                bestIdx = i;
            }
        }
        T best = elements[bestIdx];
        elements[bestIdx] = elements.back();
        elements.pop_back();
        return best;
    }

    void clear() { elements.clear(); }

    bool empty() { return elements.empty(); }

    void push(T el) { elements.push_back(el); }

    size_t size() const { return elements.size(); }

    void rearrange(const T& /*target*/) {}
};
