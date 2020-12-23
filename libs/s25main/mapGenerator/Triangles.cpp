// Copyright (c) 2017 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "mapGenerator/Triangles.h"

#include <stdexcept>
#include <vector>

namespace rttr { namespace mapGenerator {

    Triangle::Triangle(bool rsu, const MapPoint& position) : rsu(rsu), position(position) {}

    Triangle::Triangle(bool rsu, const Position& position, const MapExtent& size)
        : rsu(rsu), position(MakeMapPoint(position, size))
    {}

    Triangle Triangle::Inverse() const { return Triangle(!rsu, position); }

    Triangle Triangle::Inverse(const MapPoint& position) const { return Triangle(!rsu, position); }

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

            switch(direction.native_value())
            {
                case Direction::WEST:
                    return {Triangle(true, Position(p.x, p.y - 1), size),
                            Triangle(false, Position(p.x - 1, p.y), size)};

                case Direction::SOUTHWEST:
                    return {Triangle(true, Position(p.x, p.y), size), Triangle(false, Position(p.x - 1, p.y), size)};

                case Direction::SOUTHEAST:
                    return {Triangle(true, Position(p.x, p.y), size), Triangle(false, Position(p.x, p.y), size)};

                case Direction::EAST:
                    return {Triangle(true, Position(p.x + 1, p.y - 1), size),
                            Triangle(false, Position(p.x, p.y), size)};

                case Direction::NORTHEAST:
                    return {Triangle(true, Position(p.x + 1, p.y - 1), size),
                            Triangle(false, Position(p.x, p.y - 1), size)};

                case Direction::NORTHWEST:
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

            switch(direction.native_value())
            {
                case Direction::WEST:
                    return {Triangle(true, Position(p.x - 1, p.y - 1), size),
                            Triangle(false, Position(p.x - 1, p.y), size)};

                case Direction::SOUTHWEST:
                    return {Triangle(true, Position(p.x, p.y), size), Triangle(false, Position(p.x - 1, p.y), size)};

                case Direction::SOUTHEAST:
                    return {Triangle(true, Position(p.x, p.y), size), Triangle(false, Position(p.x, p.y), size)};

                case Direction::EAST:
                    return {Triangle(true, Position(p.x, p.y - 1), size), Triangle(false, Position(p.x, p.y), size)};

                case Direction::NORTHEAST:
                    return {Triangle(true, Position(p.x, p.y - 1), size),
                            Triangle(false, Position(p.x - 1, p.y - 1), size)};

                case Direction::NORTHWEST:
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

        const auto& rsu1 = MakeMapPoint(Position(p.x - !(p.y & 1), p.y - 1), size);
        const auto& rsu2 = MakeMapPoint(Position(p.x + (p.y & 1), p.y - 1), size);
        const auto& lsd1 = MakeMapPoint(Position(p.x - 1, p.y), size);
        const auto& lsd2 = MakeMapPoint(Position(p.x - !(p.y & 1), p.y - 1), size);

        return {Triangle(true, p),  Triangle(true, rsu1),  Triangle(true, rsu2),
                Triangle(false, p), Triangle(false, lsd1), Triangle(false, lsd2)};
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

        int x1 = p.x + (rsu ? -1 : 1);
        int y1 = p.y;

        int x2 = p.x + (!odd ? -1 : 0) + (!rsu ? 1 : 0);
        int y2 = p.y + (rsu ? 1 : -1);

        const MapPoint& p1 = MakeMapPoint(Position(x1, y1), size);
        const MapPoint& p2 = MakeMapPoint(Position(x2, y2), size);

        return {t.Inverse(), t.Inverse(p1), t.Inverse(p2)};
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
