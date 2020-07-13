// Copyright (c) 2016 - 2018 Settlers Freaks (sf-team at siedler25.org)
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

#include "initGameRNG.hpp"
#include "random/Random.h"
#include <rttr/test/random.hpp>
#include <iostream>

void initGameRNG(unsigned defaultValue /*= 1337*/)
{
#ifdef RTTR_RAND_TEST
    defaultValue += rttr::test::randomValue<unsigned>();
#endif
    RANDOM.Init(defaultValue);
}
