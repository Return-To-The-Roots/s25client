// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Replay.h"
#include <boost/filesystem/path.hpp>
#include <string>

struct ReplayInfo
{
    ReplayInfo() : async(0), end(false), next_gf(0), all_visible(false) {}

    /// Replaydatei
    Replay replay;
    boost::filesystem::path filename;
    /// Replay asynchron (Meldung nur einmal ausgeben!)
    int async;
    bool end;
    // Nächster Replay-Command-Zeitpunkt (in GF)
    unsigned next_gf;
    /// Alles sichtbar (FoW deaktiviert)
    bool all_visible;
};
