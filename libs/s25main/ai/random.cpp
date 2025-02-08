// Copyright (C) 2005 - 2025 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ai/random.h"

namespace AI {

std::minstd_rand& getRandomGenerator()
{
    std::random_device rnd_dev;
    static std::minstd_rand rng(rnd_dev());
    return rng;
}

} // namespace AI
