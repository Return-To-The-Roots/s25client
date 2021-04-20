// Copyright (c) 2017 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "mapGenerator/Triangles.h"

#include <stdexcept>
#include <vector>

namespace rttr { namespace mapGenerator {

    std::array<Triangle, 2> GetTriangles(const MapPoint& p, const MapExtent& size, Direction direction)
    {
        if(p.y & 1)
        {
            //   Y is ODD
            //
            //   R    0/ 0 (S)
            //   S    0/-1 (NW)
            //   U    1/-1 (NE)
            //
            //   L    0/ 0 (SE)
            //   S   -1/ 0 (SW)
            //   D    0/-1 (N)

            switch(direction)
            {
                case Direction::West:
                    return {Triangle(true, Position(p.x, p.y - 1), size),
                            Triangle(false, Position(p.x - 1, p.y), size)};

                case Direction::SouthWest:
                    return {Triangle(true, Position(p.x, p.y), size), Triangle(false, Position(p.x - 1, p.y), size)};

                case Direction::SouthEast:
                    return {Triangle(true, Position(p.x, p.y), size), Triangle(false, Position(p.x, p.y), size)};

                case Direction::East:
                    return {Triangle(true, Position(p.x + 1, p.y - 1), size),
                            Triangle(false, Position(p.x, p.y), size)};

                case Direction::NorthEast:
                    return {Triangle(true, Position(p.x + 1, p.y - 1), size),
                            Triangle(false, Position(p.x, p.y - 1), size)};

                case Direction::NorthWest:
                    return {Triangle(true, Position(p.x, p.y - 1), size),
                            Triangle(false, Position(p.x, p.y - 1), size)};
            }
        } else
        {
            //   Y is EVEN
            //
            //   R    0/ 0 (S)
            //   S   -1/-1 (NW)
            //   U    0/-1 (NE)
            //
            //   L    0/ 0 (SE)
            //   S   -1/ 0 (SW)
            //   D   -1/-1 (N)

            switch(direction)
            {
                case Direction::West:
                    return {Triangle(true, Position(p.x - 1, p.y - 1), size),
                            Triangle(false, Position(p.x - 1, p.y), size)};

                case Direction::SouthWest:
                    return {Triangle(true, Position(p.x, p.y), size), Triangle(false, Position(p.x - 1, p.y), size)};

                case Direction::SouthEast:
                    return {Triangle(true, Position(p.x, p.y), size), Triangle(false, Position(p.x, p.y), size)};

                case Direction::East:
                    return {Triangle(true, Position(p.x, p.y - 1), size), Triangle(false, Position(p.x, p.y), size)};

                case Direction::NorthEast:
                    return {Triangle(true, Position(p.x, p.y - 1), size),
                            Triangle(false, Position(p.x - 1, p.y - 1), size)};

                case Direction::NorthWest:
                    return {Triangle(true, Position(p.x - 1, p.y - 1), size),
                            Triangle(false, Position(p.x - 1, p.y - 1), size)};
            }
        }

        throw std::invalid_argument("direction not supported");
    }

    std::array<Triangle, 6> GetTriangles(const MapPoint& p, const MapExtent& size)
    {
        // Every node point on the grid has exactly 6 directly triangles.
        // 3 of those triangles are RSU triangles, the other 3 are LSD triangles.
        // Note that every 2nd row is shifted by half a triangle to the left,
        // therefore:
        //
        //   Y-value    EVEN        ODD
        //
        //   R          0/ 0        0/ 0
        //   S         -1/-1        0/-1
        //   U          0/-1        1/-1
        //
        //   L          0/ 0        0/ 0
        //   S         -1/ 0       -1/ 0
        //   D         -1/-1        0/-1

        Position rsu1(p.x - !(p.y & 1), p.y - 1);
        Position rsu2(p.x + (p.y & 1), p.y - 1);
        Position lsd1(p.x - 1, p.y);
        Position lsd2(p.x - !(p.y & 1), p.y - 1);

        return {Triangle(true, p),  Triangle(true, rsu1, size),  Triangle(true, rsu2, size),
                Triangle(false, p), Triangle(false, lsd1, size), Triangle(false, lsd2, size)};
    }

    std::array<Triangle, 3> GetTriangleNeighbors(const Triangle& t, const MapExtent& size)
    {
        // Every triangle has exactly 3 triangle neighbors on the map.
        // For an RSU triangle, each neighbor must be LSD and the other way around.
        // Note that every 2nd row is shifted by half a triangle to the left,
        // therefore:
        //
        //   Y-value    EVEN        ODD
        //
        //   R          0/ 0        0/ 0  (North/East)
        //   S         -1/ 0       -1/ 0  (North/West)
        //   U         -1/ 1        0/ 1  (South/East)
        //
        //   L          0/ 0        0/ 0  (South/West)
        //   S          1/ 0        1/ 0  (South/East)
        //   D          0/-1        1/-1  (North/West)

        const MapPoint& p = t.position;
        const bool odd = p.y & 1;
        const bool rsu = t.rsu;

        const int x1 = p.x + (rsu ? -1 : 1);
        const int y1 = p.y;

        const int x2 = p.x + (!odd ? -1 : 0) + (!rsu ? 1 : 0);
        const int y2 = p.y + (rsu ? 1 : -1);

        return {Triangle(!rsu, p), Triangle(!rsu, Position(x1, y1), size), Triangle(!rsu, Position(x2, y2), size)};
    }

    std::array<MapPoint, 3> GetTriangleEdges(const Triangle& t, const MapExtent& size)
    {
        // Every triangle has exactly 3 edge points on the map.
        // Note that every 2nd row is shifted by half a triangle to the left,
        // therefore:
        //
        //   Y-value    EVEN        ODD
        //
        //   R          0/ 0        0/ 0
        //   S         -1/ 1        1/ 1
        //   U          0/ 1        0/ 1
        //
        //   L          0/ 0        0/ 0
        //   S          1/ 0        1/ 0
        //   D          0/ 1        1/ 1

        const MapPoint& p = t.position;
        const bool rsu = t.rsu;
        const int odd = p.y & 1;

        int x1 = p.x + (rsu && !odd ? -1 : 1);
        int y1 = p.y + (rsu ? 1 : 0);

        int x2 = p.x + (!rsu && odd ? 1 : 0);
        int y2 = p.y + 1;

        return {p, MakeMapPoint(Position(x1, y1), size), MakeMapPoint(Position(x2, y2), size)};
    }

}} // namespace rttr::mapGenerator
