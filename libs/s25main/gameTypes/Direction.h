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

#ifndef Direction_h__
#define Direction_h__

#include <iterator>

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
    static constexpr unsigned COUNT = SOUTHWEST + 1;

    Type t_;
    Direction() : t_(WEST) {}
    Direction(Type t) : t_(t) { RTTR_Assert(t_ >= WEST && static_cast<unsigned>(t_) < COUNT); }
    /// Convert an UInt safely to a Direction
    explicit Direction(unsigned t) : t_(Type(t % COUNT)) { RTTR_Assert(t_ >= WEST && static_cast<unsigned>(t_) < COUNT); }
    /// Convert an UInt to a Direction without checking its value. Use only when this is actually a Direction
    static Direction fromInt(unsigned t) { return Type(t); }
    static Direction fromInt(int t) { return Type(t); }
    /// Use this for use in switches
    Type native_value() const { return t_; }
    /// Return the Direction as an UInt
    unsigned toUInt() const { return t_; }
    Direction& operator+=(unsigned i);
    Direction& operator-=(unsigned i);
    Direction& operator++();
    Direction operator++(int);
    Direction& operator--();
    Direction operator--(int);

    struct iterator;
    typedef iterator const_iterator;
    const_iterator begin() const;
    const_iterator end() const;

private:
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
    t_ = Direction(static_cast<unsigned>(t_) + i).t_;
    return *this;
}

inline Direction& Direction::operator-=(unsigned i)
{
    t_ = Direction(static_cast<unsigned>(t_) + COUNT - (i % COUNT)).t_;
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
    return lhs.t_ == rhs.t_;
}

inline bool operator!=(const Direction& lhs, const Direction& rhs)
{
    return !(lhs == rhs);
}

// Comparison operators to avoid ambiguity
inline bool operator==(const Direction::Type& lhs, const Direction& rhs)
{
    return lhs == rhs.t_;
}
inline bool operator==(const Direction& lhs, const Direction::Type& rhs)
{
    return lhs.t_ == rhs;
}
inline bool operator!=(const Direction::Type& lhs, const Direction& rhs)
{
    return lhs != rhs.t_;
}
inline bool operator!=(const Direction& lhs, const Direction::Type& rhs)
{
    return lhs.t_ != rhs;
}

struct Direction::iterator
{
    typedef std::forward_iterator_tag iterator_category;
    typedef Direction value_type;
    typedef Direction reference;
    typedef const Direction* pointer;
    typedef std::ptrdiff_t difference_type;

    explicit iterator(unsigned value) : value_(value) {}
    iterator& operator++()
    {
        ++value_;
        return *this;
    }
    iterator operator++(int)
    {
        iterator retval = *this;
        ++(*this);
        return retval;
    }
    bool operator==(iterator other) const { return value_ == other.value_; }
    bool operator!=(iterator other) const { return !(*this == other); }
    Direction operator*() const { return Direction::fromInt(value_ < COUNT ? value_ : value_ - COUNT); }

private:
    unsigned value_;
};

inline Direction::const_iterator Direction::begin() const
{
    return const_iterator(t_);
}
inline Direction::const_iterator Direction::end() const
{
    return const_iterator(t_ + COUNT);
}

#endif // Direction_h__
