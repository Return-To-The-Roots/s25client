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
    typedef T ElementType;

    T x, y;
    Point() {} //-V730
    Point(const T x, const T y): x(x), y(y) {}
    Point(const Point& other): x(other.x), y(other.y){}
    template<typename U>
    explicit Point(const Point<U>& pt): x(static_cast<T>(pt.x)), y(static_cast<T>(pt.y)) {}

    static const Point Invalid();
    inline bool isValid() const;

    inline bool operator==(const Point& second) const;
    inline bool operator!=(const Point& second) const;
    inline Point& operator+=(const Point& right);
    inline Point& operator-=(const Point& right);
    inline friend Point operator+(Point left, const Point& right) { return (left+=right); }
    inline friend Point operator-(Point left, const Point& right) { return (left-=right); }
    inline Point operator*(const T div) const;
    inline Point operator/(const T div) const;
};

template <typename T>
const Point<T> Point<T>::Invalid()
{
    return Point(std::numeric_limits<T>::max(), std::numeric_limits<T>::max());
}

template <typename T>
bool Point<T>::isValid() const
{
    return *this != Invalid();
}

template <typename T>
bool Point<T>::operator==(const Point<T>& second) const
{
    return (x == second.x && y == second.y);
}

template <typename T>
bool Point<T>::operator!=(const Point<T>& second) const
{
    return !(*this == second);
}

template <typename T>
Point<T>& Point<T>::operator+=(const Point<T>& right)
{
    x += right.x;
    y += right.y;
    return *this;
}

template <typename T>
Point<T>& Point<T>::operator-=(const Point<T>& right)
{
    x -= right.x;
    y -= right.y;
    return *this;
}

template <typename T>
Point<T> Point<T>::operator/(const T div) const
{
    return Point(x / div, y / div);
}

template <typename T>
Point<T> Point<T>::operator*(const T div) const
{
    return Point(x * div, y * div);
}

#endif // Point_h__
