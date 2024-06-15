// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Replay.h"
#include <boost/filesystem/path.hpp>
#include <optional>
#include <string>

struct ReplayInfo
{
    /// Replay file
    Replay replay;
    boost::filesystem::path filename;
    /// Number of async GFs
    int async = 0;
    // GF for the next replay command if any
    std::optional<unsigned> next_gf;
    /// FoW deactivated?
    bool all_visible;
};
