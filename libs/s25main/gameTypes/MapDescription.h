// Copyright (C) 2005 - 2023 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/MapType.h"
#include <boost/filesystem/path.hpp>
#include <optional>

struct MapDescription
{
    boost::filesystem::path map_path;
    MapType map_type;
    std::optional<boost::filesystem::path> lua_path;

    MapDescription(boost::filesystem::path map_path, MapType map_type,
                   std::optional<boost::filesystem::path> lua_path = std::nullopt);
};
