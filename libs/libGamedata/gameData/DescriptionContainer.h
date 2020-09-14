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

#pragma once

#include "DescIdx.h"
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

/// Hold describing data about a type with access by name and index
template<typename T>
struct DescriptionContainer
{
    /// Add a new description. Throws if one with the same name already exists
    DescIdx<T> add(T desc);
    /// Return the number of descriptions stored
    unsigned size() const { return static_cast<unsigned>(items.size()); }
    /// Return the index of the item with the given name
    DescIdx<T> getIndex(const std::string& name) const;
    /// Return the entry with the given name or nullptr
    const T* tryGet(const std::string& name) const;
    /// Return the entry with the given idx or nullptr
    const T* tryGet(DescIdx<T> idx) const;
    /// Return the item at the given index
    const T& get(DescIdx<T> idx) const;
    /// Return a mutable reference to the given item
    T& getMutable(DescIdx<T> idx);

private:
    std::vector<T> items;
    std::map<std::string, unsigned> name2Idx;
};

template<typename T>
inline DescIdx<T> DescriptionContainer<T>::add(T desc)
{
    if(!getIndex(desc.name))
    {
        if(size() >= DescIdx<T>::INVALID)
            throw std::runtime_error("To many entries!");
        DescIdx<T> idx(size());
        items.push_back(desc);
        name2Idx[desc.name] = idx.value;
        return idx;
    }
    throw std::runtime_error(std::string("Duplicate entry with name ") + desc.name + " added!");
}

template<typename T>
inline DescIdx<T> DescriptionContainer<T>::getIndex(const std::string& name) const
{
    auto it = name2Idx.find(name);
    if(it == name2Idx.end())
        return DescIdx<T>();
    return DescIdx<T>(it->second);
}

template<typename T>
inline const T* DescriptionContainer<T>::tryGet(const DescIdx<T> idx) const
{
    if(!idx || idx.value >= size())
        return nullptr;
    return &items[idx.value];
}

template<typename T>
inline const T* DescriptionContainer<T>::tryGet(const std::string& name) const
{
    return tryGet(getIndex(name));
}

template<typename T>
inline const T& DescriptionContainer<T>::get(const DescIdx<T> idx) const
{
    return items[idx.value];
}

template<typename T>
inline T& DescriptionContainer<T>::getMutable(const DescIdx<T> idx)
{
    return items[idx.value];
}
