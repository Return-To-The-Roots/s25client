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

#include "XorShift.h"
#include "s25util/Serializer.h"
#include <iostream>

void XorShift::discard(uint64_t j)
{
    for(uint64_t i = 0; i < j; i++)
        (*this)();
}

void XorShift::deserialize(Serializer& ser)
{
    uint64_t high = ser.PopUnsignedInt();
    uint64_t low = ser.PopUnsignedInt();
    state_ = (high << 32) | low;
}

void XorShift::serialize(Serializer& ser) const
{
    auto high = static_cast<unsigned>(state_ >> 32);
    auto low = static_cast<unsigned>(state_);
    ser.PushUnsignedInt(high);
    ser.PushUnsignedInt(low);
}

std::ostream& operator<<(std::ostream& os, const XorShift& obj)
{
    return os << obj.state_;
}

std::istream& operator>>(std::istream& is, XorShift& obj)
{
    return is >> obj.state_;
}

void XorShift::seed(uint64_t newSeed)
{
    // Zero is not allowed as it leads to only zeros as output
    state_ = (newSeed) ? newSeed : 1;
}
