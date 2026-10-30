// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AsyncChecksum.h"
#include "Replay.h"
#include <functional>
#include <optional>

class Game;

/// The game state recorded in the replay did not match the one the playback produced.
struct ReplayDesync
{
    unsigned gf;
    AsyncChecksum expected;
    AsyncChecksum actual;
};

struct ReplayCommandsResult
{
    bool executedGameCommands = false;
    std::optional<ReplayDesync> desync;
};

/// Execute the commands the replay recorded for the game's current GF, handing chat messages to onChat.
/// nextGF is the GF of the replay's next command and is advanced past the executed ones.
ReplayCommandsResult ExecuteReplayCommands(Replay& replay, std::optional<unsigned>& nextGF, Game& game,
                                           const std::function<void(const Replay::ChatCommand&)>& onChat = {});
