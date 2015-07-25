// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef Point_h__
#define Point_h__

#include <limits>

// 2D-Punkt
template <typename T>
struct Point
{
    T x, y;
    Point() {}
    Point(const T x, const T y): x(x), y(y) {}
    template<typename U>
    explicit Point(const Point<U>& pt): x(pt.x), y(pt.y) {}
    bool operator==(const Point second) const
    { return (x == second.x && y == second.y); }
    bool operator!=(const Point second) const
    { return !(*this == second); }

    static const Point Invalid()
    { return Point(std::numeric_limits<T>::max(), std::numeric_limits<T>::max()); }

    bool isValid() const
    {
        return *this != Invalid();
    }
};

#endif // Point_h__