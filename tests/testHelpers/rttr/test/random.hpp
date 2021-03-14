// Copyright (c) 2016 - 2019 Settlers Freaks (sf-team at siedler25.org)
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

#pragma once

#include "helpers/random.h"
#include <limits>
#include <random>
#include <string>

namespace rttr { namespace test {
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
    inline bool randomBool() { return randomValue(0, 1) == 0; }
    template<typename T>
    auto randomPoint(typename T::ElementType min = std::numeric_limits<typename T::ElementType>::min(),
                     typename T::ElementType max = std::numeric_limits<typename T::ElementType>::max())
    {
        return T{randomValue(min, max), randomValue(min, max)};
    }
    std::string randString(int len = -1);
}} // namespace rttr::test
