// Copyright (C) 2005 - 2023 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "MapDescription.h"

MapDescription::MapDescription(boost::filesystem::path map_path, MapType map_type,
                               boost::optional<boost::filesystem::path> lua_path)
    : map_path(std::move(map_path)), map_type(map_type), lua_path(std::move(lua_path))
{}
