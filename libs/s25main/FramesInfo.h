// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <chrono>

/// Struct that stores information about the frames, like GF status...
struct FramesInfo
{
    using UsedClock = std::chrono::steady_clock;

    FramesInfo();
    void Clear();

    /// Length of one GF in ms (~ 1/speed of the game)
    std::chrono::milliseconds gf_length;
    /// Requested length of GF (for multiple changes between a NWF)
    std::chrono::milliseconds gfLengthReq;
    /// Length of a NWF (network frame) in GFs
    unsigned nwf_length;
    /// Time since last GF in ms (valid range: [0, gfLength) )
    std::chrono::milliseconds frameTime;
    /// Timestamp of the last processed GF (--> FrameTime = CurrentTime - LastTime (except for lags) )
    UsedClock::time_point lastTime;
    /// True if the game is paused (no processing of GFs)
    bool isPaused;
};

/// Same as FramesInfo but with additional data that is only meaningfull for the client
struct FramesInfoClient : public FramesInfo
{
    FramesInfoClient();
    void Clear();

    /// Force pause the game (start TS and length) e.g. to compensate for lags
    UsedClock::time_point forcePauseStart;
    std::chrono::milliseconds forcePauseLen;
};
