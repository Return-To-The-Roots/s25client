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

#ifndef Direction_h__
#define Direction_h__

/// "Enum" to represent one of the 6 directions from each node
struct Direction
{
    enum Type
    {
        WEST,      // 0
        NORTHWEST, // 1
        NORTHEAST, // 2
        EAST,      // 3
        SOUTHEAST, // 4
        SOUTHWEST  // 5
    };
    static BOOST_CONSTEXPR_OR_CONST unsigned COUNT = SOUTHWEST + 1;

    Type t_;
    Direction(): t_(WEST){}
    Direction(Type t): t_(t) { RTTR_Assert(t_ >= WEST && static_cast<unsigned>(t_) < COUNT); }
    /// Convert an UInt safely to a Direction
    explicit Direction(unsigned t): t_(Type(t % COUNT)){ RTTR_Assert(t_ >= WEST && static_cast<unsigned>(t_) < COUNT); }
    /// Convert an UInt to a Direction without checking its value. Use only when this is actually a Direction
    static Direction fromInt(unsigned t){ return Type(t); }
    static Direction fromInt(int t){ return Type(t); }
    operator Type() const { return t_; }
    /// Return the Direction as an UInt
    unsigned toUInt() const { return t_; }
    Direction& operator+=(unsigned i);
    Direction& operator-=(unsigned i);
    Direction& operator++();
    Direction operator++(int);
    Direction& operator--();
    Direction operator--(int);
    // TODO: Add iterator to iterate over all values from a given value
private:
    //prevent automatic conversion for any other built-in types such as bool, int, etc
    template<typename T>
    operator T() const;
    // Disallow int operators
    Direction& operator+=(int i);
    Direction& operator-=(int i);
};
//-V:Direction:801 

//////////////////////////////////////////////////////////////////////////
// Implementation
//////////////////////////////////////////////////////////////////////////

inline Direction& Direction::operator+=(unsigned i)
{
    t_ = Direction(static_cast<unsigned>(t_) + i);
    return *this;
}

inline Direction& Direction::operator-=(unsigned i)
{
    t_ = Direction(static_cast<unsigned>(t_) + COUNT - (i % COUNT));
    return *this;
}

inline Direction operator+(Direction dir, unsigned i)
{
    return dir += i;
}

inline Direction operator-(Direction dir, unsigned i)
{
    return dir -= i;
}

inline Direction& Direction::operator++()
{
    return *this += 1u;
}

inline Direction Direction::operator++(int)
{
    Direction result(*this);
    ++(*this);
    return result;
}

inline Direction& Direction::operator--()
{
    return *this -= 1u;
}

inline Direction Direction::operator--(int)
{
    Direction result(*this);
    --(*this);
    return result;
}

inline bool operator==(const Direction& lhs, const Direction& rhs)
{
    return Direction::Type(lhs) == Direction::Type(rhs);
}

inline bool operator!=(const Direction& lhs, const Direction& rhs)
{
    return !(lhs == rhs);
}

// Comparison operators to avoid ambiguity
inline bool operator==(const Direction::Type& lhs, const Direction& rhs)
{
    return lhs == Direction::Type(rhs);
}
inline bool operator==(const Direction& lhs, const Direction::Type& rhs)
{
    return Direction::Type(lhs) == rhs;
}
inline bool operator!=(const Direction::Type& lhs, const Direction& rhs)
{
    return lhs != Direction::Type(rhs);
}
inline bool operator!=(const Direction& lhs, const Direction::Type& rhs)
{
    return Direction::Type(lhs) != rhs;
}

#endif // Direction_h__
