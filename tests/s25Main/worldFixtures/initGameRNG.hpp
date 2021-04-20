// Copyright (c) 2016 - 2018 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

/// Initialize the ingame-Random Number Generator with the given value
/// unless RTTR_RAND_TEST is defined in which case a random value is used
void initGameRNG(unsigned defaultValue = 1337);
