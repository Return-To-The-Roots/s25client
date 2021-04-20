// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
