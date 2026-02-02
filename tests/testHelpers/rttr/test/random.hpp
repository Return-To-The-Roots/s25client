// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "helpers/random.h"
#include <limits>
#include <random>
#include <string>

namespace rttr::test {
/// Decorate a test with , *boost::unit_test::label("seed=42") to enforce a seed
std::mt19937& getRandState();

template<typename T>
T randomValue(T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max())
{
    return helpers::randomValue(getRandState(), min, max);
}
template<typename T>
T randomEnum()
{
    return helpers::randomEnum<T>(getRandState());
}
inline bool randomBool()
{
    return randomValue(0, 1) == 0;
}
template<typename T>
auto randomPoint(typename T::ElementType min = std::numeric_limits<typename T::ElementType>::min(),
                 // Avoid overflow in some calculations by limiting max
                 typename T::ElementType max = std::numeric_limits<typename T::ElementType>::max() / 32)
{
    return T{randomValue(min, max), randomValue(min, max)};
}
std::string randString(int len = -1);
} // namespace rttr::test
