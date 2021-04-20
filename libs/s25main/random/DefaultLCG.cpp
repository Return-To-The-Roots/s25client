// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DefaultLCG.h"
#include "s25util/Serializer.h"
#include <iostream>

void DefaultLCG::discard(uint64_t j)
{
    for(uint64_t i = 0; i < j; i++)
        (*this)();
}

void DefaultLCG::deserialize(Serializer& ser)
{
    state_ = ser.PopUnsignedInt();
}

void DefaultLCG::serialize(Serializer& ser) const
{
    ser.PushUnsignedInt(state_);
}

std::ostream& operator<<(std::ostream& os, const DefaultLCG& obj)
{
    return os << obj.state_;
}

std::istream& operator>>(std::istream& is, DefaultLCG& obj)
{
    return is >> obj.state_;
}
