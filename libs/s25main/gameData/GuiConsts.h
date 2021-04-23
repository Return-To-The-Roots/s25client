// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>

const std::array<float, 7> SUPPRESS_UNUSED ZOOM_FACTORS = {{0.5f, 0.75f, 1.f, 1.25f, 1.5f, 2.f, 3.f}};
const size_t ZOOM_DEFAULT_INDEX = 2;
const float ZOOM_ACCELERATION = 0.001f;
const float ZOOM_WHEEL_INCREMENT = 0.03f;
