// Copyright (c) 2017 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include <vector>

namespace rttr {
namespace mapGenerator {

Triangle::Triangle(bool rsu, const MapPoint& position) : rsu(rsu), position(position)
{
    
}

Triangle Triangle::Inverse() const
{
    return Triangle(!rsu, position);
}

Triangle Triangle::Inverse(const MapPoint& position) const
{
    return Triangle(!rsu, position);
}

std::vector<Triangle> GetTriangles(const MapPoint& p, const MapExtent& size)
{
    // Every node point on the grid has exactly 6 directly triangles.
    // 3 of those triangles are RSU triangles, the other 3 are LSD triangles.
    // Note that every 2nd row is shifted by half a triangle to the left,
    // therefore:
    //
    //   Y-value    EVEN        ODD
    //   R          0/ 0        0/ 0
    //   S         -2/-1        0/-1
    //   U         -1/-1        1/-1
    //
    //   L          0/ 0        0/ 0
    //   S         -1/ 0       -1/ 0
    //   D         -2/-1        0/-1
    
    MapPoint rsu1 = MakeMapPoint(Position(p.x - !(p.y & 1) * 2, p.y - 1), size);
    MapPoint rsu2 = MakeMapPoint(Position(p.x - !(p.y & 1) + (p.y & 1), p.y - 1), size);
    MapPoint lsd1 = MakeMapPoint(Position(p.x - 1, p.y), size);
    MapPoint lsd2 = MakeMapPoint(Position(p.x - !(p.y & 1) * 2, p.y - 1), size);

    return {
        Triangle(true,  p), Triangle(true,  rsu1), Triangle(true,  rsu2),
        Triangle(false, p), Triangle(false, lsd1), Triangle(false, lsd2)
    };
}

std::vector<Triangle> GetTriangleNeighbors(const Triangle& t, const MapExtent& size)
{
    // y    EVEN   EVEN   ODD   ODD
    //      LSD    RSU    LSD   RSU
    // x0    0      0      0     0
    // y0    0      0      0     0
    // x1   +1     -1     +1    -1
    // y1    0      0      0     0
    // x2   -1     -1     +1    -1
    // y2   -1     +1     -1    -1

    bool odd = t.position.y & 1;
    
    int x1 = t.position.x + (t.rsu ? -1 : 1);
    int y1 = t.position.y;
    
    int x2 = t.position.x + (t.rsu || !odd ? -1 : 1);
    int y2 = t.position.y + (!t.rsu || odd ? -1 : 1);

    MapPoint p1 = MakeMapPoint(Position(x1, y1), size);
    MapPoint p2 = MakeMapPoint(Position(x2, y2), size);
    
    return { t.Inverse(), t.Inverse(p1), t.Inverse(p2) };
}

std::vector<MapPoint> GetTriangleEdges(const Triangle& t, const MapExtent& size)
{
    int odd = t.position.y & 1;
    
    int x1 = t.position.x + (t.rsu && !odd ? -1 : 1);
    int y1 = t.position.y + (t.rsu ? 1 : 0);

    int x2 = t.position.x + (odd ? 2 : 0);
    int y2 = t.position.y + 1;

    return {
        t.position,
        MakeMapPoint(Position(x1, y1), size),
        MakeMapPoint(Position(x2, y2), size)
    };
}

}}
