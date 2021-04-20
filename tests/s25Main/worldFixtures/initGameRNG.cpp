// Copyright (C) 2016 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
