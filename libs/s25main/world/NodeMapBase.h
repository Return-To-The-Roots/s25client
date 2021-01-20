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

#pragma once

#include "MapBase.h"
#include "gameTypes/MapCoordinates.h"
#include <functional>
#include <vector>

/// Base class for a map with the geometry of the world (MapBase)
/// Provides nodes of a given type with accessors to the node accepting flat indices or points
template<typename T_Node>
class NodeMapBase final : public MapBase
{
    std::vector<T_Node> nodes;

public:
    using Node = T_Node;

    void Resize(const MapExtent& newSize) override;
    void Resize(const MapExtent& newSize, const Node& defaultValue);

    /// Access given node
    Node& operator[](unsigned idx) { return nodes[idx]; }
    const Node& operator[](unsigned idx) const { return nodes[idx]; }
    Node& operator[](const MapPoint& pt);
    const Node& operator[](const MapPoint& pt) const;

    typename std::vector<Node>::const_iterator begin() const noexcept { return nodes.begin(); }
    typename std::vector<Node>::const_iterator end() const noexcept { return nodes.end(); }
};

//////////////////////////////////////////////////////////////////////////
// Implementation

template<typename T_Node>
inline const T_Node& NodeMapBase<T_Node>::operator[](const MapPoint& pt) const
{
    return nodes[GetIdx(pt)];
}

template<typename T_Node>
inline T_Node& NodeMapBase<T_Node>::operator[](const MapPoint& pt)
{
    return nodes[GetIdx(pt)];
}

template<typename T_Node>
void NodeMapBase<T_Node>::Resize(const MapExtent& newSize)
{
    MapBase::Resize(newSize);
    nodes.clear();
    nodes.resize(prodOfComponents(newSize));
}

template<typename T_Node>
void NodeMapBase<T_Node>::Resize(const MapExtent& newSize, const T_Node& defaultValue)
{
    MapBase::Resize(newSize);
    nodes.clear();
    nodes.resize(prodOfComponents(newSize), defaultValue);
}
