// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
