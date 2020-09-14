// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include <array>

const std::array<float, 7> SUPPRESS_UNUSED ZOOM_FACTORS = {{0.5f, 0.75f, 1.f, 1.25f, 1.5f, 2.f, 3.f}};
const size_t ZOOM_DEFAULT_INDEX = 2;
const float ZOOM_ACCELERATION = 0.001f;
const float ZOOM_WHEEL_INCREMENT = 0.03f;
