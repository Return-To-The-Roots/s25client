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

#ifndef ReplayInfo_h__
#define ReplayInfo_h__

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

#endif // ReplayInfo_h__
