// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
    using const_iterator = typename std::vector<Node>::const_iterator;

    void Resize(const MapExtent& newSize) override;
    void Resize(const MapExtent& newSize, const Node& defaultValue);

    /// Access given node
    Node& operator[](unsigned idx) { return nodes[idx]; }
    const Node& operator[](unsigned idx) const { return nodes[idx]; }
    Node& operator[](const MapPoint& pt);
    const Node& operator[](const MapPoint& pt) const;

    const_iterator begin() const noexcept { return nodes.begin(); }
    const_iterator end() const noexcept { return nodes.end(); }
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
