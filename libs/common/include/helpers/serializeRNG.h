// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "s25util/StringConversion.h"
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace helpers {

template<typename RandomT>
std::string serializeRng(const RandomT& rng)
{
    s25util::ClassicImbuedStream<std::ostringstream> s;
    s << rng;
    return s.str();
}

template<typename RandomT>
bool deserializeRng(RandomT& rng, const std::string& data)
{
    s25util::ClassicImbuedStream<std::istringstream> s(data);
    return (s >> rng) && s.eof();
}

template<typename RandomT>
void pushRng(Serializer& ser, const RandomT& rng)
{
    ser.PushString(serializeRng(rng));
}

template<typename RandomT>
auto popRng(Serializer& ser)
{
    std::remove_reference_t<RandomT> rng;
    if(!deserializeRng(rng, ser.PopString()))
        throw std::invalid_argument("Invalid RNG state");
    return rng;
}

} // namespace helpers
