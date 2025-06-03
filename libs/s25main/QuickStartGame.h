// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/AIInfo.h"
#include <boost/filesystem/path.hpp>
#include <string>
#include <vector>

/// Parse --ai flags specified in the command line
std::vector<AI::Info> ParseAIOptions(const std::vector<std::string>& aiOptions);

/// Tries to start a game (map, savegame, replay, or ai-battle) and returns whether this was successfull
bool QuickStartGame(const boost::filesystem::path& mapOrReplayPath, const std::vector<std::string>& ais);
