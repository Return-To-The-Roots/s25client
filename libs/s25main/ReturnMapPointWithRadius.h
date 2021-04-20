// Copyright (c) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <utility>

struct ReturnMapPointWithRadius
{
    constexpr std::pair<MapPoint, unsigned> operator()(const MapPoint pt, unsigned r) const noexcept { return {pt, r}; }
};
