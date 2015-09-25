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
        NORTH,     // 0
        NORTHEAST, // 1
        SOUTHEAST, // 2
        SOUTH,     // 3
        SOUTHWEST, // 4
        NORTHWEST  // 5
    };
    static const int COUNT = NORTHWEST + 1;

    Type t_;
    Direction(Type t) : t_(t) { assert(t_ >= NORTH && t_ < COUNT); }
    /// Converts an UInt safely to a Direction
    explicit Direction(unsigned t): t_(Type(t % COUNT)){ assert(t_ >= NORTH && t_ < COUNT); }
    /// Converts an UInt to a Direction without checking its value. Use only when this is actually a Direction
    static Direction fromInt(unsigned t){ return Type(t); }
    static Direction fromInt(int t){ return Type(t); }
    operator Type() const { return t_; }
    /// Returns the Direction as an UInt (for legacy code)
    unsigned toUInt(){ return t_; }
    Direction operator+(unsigned i) const { return Direction(t_ + i); }
    // TODO: Add iterator to iterate over all values from a given value
private:
    //prevent automatic conversion for any other built-in types such as bool, int, etc
    template<typename T>
    operator T() const;
};

#endif // Direction_h__