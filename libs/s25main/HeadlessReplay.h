// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AsyncChecksum.h"
#include "ILocalGameState.h"
#include "Replay.h"
#include "gameTypes/MapInfo.h"
#include <boost/filesystem/path.hpp>
#include <functional>
#include <memory>
#include <optional>

class Game;
class GameWorld;

/// The game state recorded in the replay did not match the one the playback produced.
struct ReplayDesync
{
    unsigned gf;
    AsyncChecksum expected;
    AsyncChecksum actual;
};

class HeadlessReplay
{
public:
    /// Reinitializes the global RNG.
    /// Throws std::runtime_error if the replay cannot be loaded.
    explicit HeadlessReplay(const boost::filesystem::path& replayPath);
    ~HeadlessReplay();

    const Replay& getReplay() const { return replay_; }
    const Game& getGame() const { return *game_; }
    const GameWorld& getWorld() const;
    bool isFromSavegame() const { return mapInfo_.savegame != nullptr; }
    /// GF the replay starts at: 0, or where the savegame it was started from left off
    unsigned getStartGF() const { return startGF_; }
    unsigned getCurrentGF() const;
    const std::optional<ReplayDesync>& getDesync() const { return desync_; }

    /// Execute the commands recorded for the current GF and advance the game by one frame.
    /// Returns false after the frame of the replay's last GF, where the recording ended, and at a desync.
    /// The game then stays at the desynced GF: every following frame would desync too, so there is
    /// nothing left to learn from continuing.
    /// Throws std::runtime_error if the replay is corrupt, i.e. holds a command for a GF that already passed.
    bool RunGF();
    /// Play back the whole replay, calling onGameFrame after each frame.
    /// Returns false if a desync was found.
    bool Run(const std::function<void(const HeadlessReplay&)>& onGameFrame = {});

private:
    NullLocalGameState localGameState_;
    Replay replay_;
    MapInfo mapInfo_;
    std::unique_ptr<Game> game_;
    unsigned startGF_ = 0;
    /// GF the next recorded command applies to, empty once the replay is exhausted
    std::optional<unsigned> nextGF_;
    std::optional<ReplayDesync> desync_;
};
