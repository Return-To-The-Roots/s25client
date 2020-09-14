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

#include <chrono>

/// Struct that stores information about the frames, like GF status...
struct FramesInfo
{
    using milliseconds32_t = std::chrono::duration<uint32_t, std::milli>; //-V:milliseconds32_t:813
    using UsedClock = std::chrono::steady_clock;

    FramesInfo();
    void Clear();

    /// Length of one GF in ms (~ 1/speed of the game)
    milliseconds32_t gf_length;
    /// Requested length of GF (for multiple changes between a NWF)
    milliseconds32_t gfLengthReq;
    /// Length of a NWF (network frame) in GFs
    unsigned nwf_length;
    /// Time since last GF in ms (valid range: [0, gfLength) )
    milliseconds32_t frameTime;
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
    milliseconds32_t forcePauseLen;
};
