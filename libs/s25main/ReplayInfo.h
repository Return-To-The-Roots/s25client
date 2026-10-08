// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
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
    /// GF the first desync was detected at, if any
    std::optional<unsigned> desyncGF;
    // GF for the next replay command if any
    std::optional<unsigned> next_gf;
    /// FoW deactivated?
    bool all_visible = false;
};
