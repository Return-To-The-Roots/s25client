// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/filesystem/path.hpp>
#include <string>

/// Tries to start a game (map, savegame, replay, or ai-battle) and returns whether this was successfull
bool QuickStartGame(const boost::filesystem::path& mapOrReplayPath, const std::vector<std::string>& ais = std::vector<std::string>(), bool singlePlayer = false);
