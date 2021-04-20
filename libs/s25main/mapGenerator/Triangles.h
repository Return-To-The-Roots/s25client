// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "world/MapGeometry.h"
#include <array>
#include <cmath>

namespace rttr { namespace mapGenerator {

    /**
     * Triangle is a geometrical representation for an LSD (left-side-down) or RSD (right-side-up) texture.
     */
    struct Triangle
    {
        const bool rsu;
        const MapPoint position;

        Triangle(bool rsu, const MapPoint& position) : rsu(rsu), position(position) {}
        Triangle(bool rsu, const Position& position, const MapExtent& size)
            : rsu(rsu), position(MakeMapPoint(position, size))
        {}
    };

    /**
     * Gets a triangle pair around the specified point in the specified direction.
     *
     * @param p point of reference
     * @param size size of the map
     * @param direction direction of the edge which goes right through the target triangle pair
     *
     * @returns a triangle pair around the target point in the specified direction.
     */
    std::array<Triangle, 2> GetTriangles(const MapPoint& p, const MapExtent& size, Direction direction);

    /**
     * Finds all triangles connected to the specified map point.
     * @param p center point of the triangles
     * @param size map size
     *
     * @returns a list of all triangles which are connected to the point.
     */
    std::array<Triangle, 6> GetTriangles(const MapPoint& p, const MapExtent& size);

    /**
     * Computes all neighboring triangles for the specified triangle.
     * @param triangle triangle to find neighbors for
     * @param size map size
     *
     * @returns all three neighboring triangles
     */
    std::array<Triangle, 3> GetTriangleNeighbors(const Triangle& triangle, const MapExtent& size);

    /**
     * Computes the edge points for the specified triangle - useful for interpolation.
     * @param triangle triangle to find edge points for
     * @param size size of the map
     *
     * @returns edge points of the triangle.
     */
    std::array<MapPoint, 3> GetTriangleEdges(const Triangle& triangle, const MapExtent& size);

}} // namespace rttr::mapGenerator
