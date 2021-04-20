// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/filesystem/path.hpp>
#include <string>

/// Tries to start a game (map, savegame or replay) and returns whether this was successfull
bool QuickStartGame(const boost::filesystem::path& mapOrReplayPath, bool singlePlayer = false);
