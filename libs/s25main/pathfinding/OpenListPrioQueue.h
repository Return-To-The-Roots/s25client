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

#include <algorithm>
#include <queue>
#include <vector>

/// A priority queue based on a vector with rearange capabilities in case of a change
template<class T, class Pr = std::less<T>>
class OpenListPrioQueue : public std::priority_queue<T, std::vector<T>, Pr>
{
    using Parent = std::priority_queue<T, std::vector<T>, Pr>;

public:
    using iterator = typename std::vector<T>::iterator;

    OpenListPrioQueue() : Parent() { Parent::c.reserve(255); }

    void rearrange(const T& target)
    {
        iterator it = find(target);
        rearrange(it);
    }

    void rearrange(iterator it) { std::push_heap(Parent::c.begin(), it + 1, Parent::comp); }

    iterator find(const T& target) { return std::find(Parent::c.begin(), Parent::c.end(), target); }

    void clear() { Parent::c.clear(); }

    /// Removes and returns the first element
    T pop()
    {
        T result = Parent::top();
        Parent::pop();
        return result;
    }
};
