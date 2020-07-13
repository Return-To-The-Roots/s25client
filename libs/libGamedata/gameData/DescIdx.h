// Copyright (c) 2017 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef DescIdx_h__
#define DescIdx_h__

#include <cstdint>

/// Type safe index for a description
//-V:DescIdx:801
template<class T>
struct DescIdx
{
    /// Invalid index
    static constexpr uint8_t INVALID = 0xFF;
    uint8_t value;
    explicit DescIdx(uint8_t value = INVALID) : value(value) {}
    bool operator!() const { return value == INVALID; }
    bool operator==(DescIdx rhs) const { return value == rhs.value; }
    bool operator!=(DescIdx rhs) const { return value != rhs.value; }
    bool operator<(DescIdx rhs) const { return value < rhs.value; }
    bool operator>(DescIdx rhs) const { return value > rhs.value; }
    bool operator<=(DescIdx rhs) const { return value <= rhs.value; }
    bool operator>=(DescIdx rhs) const { return value >= rhs.value; }
};

#endif // DescIdx_h__
