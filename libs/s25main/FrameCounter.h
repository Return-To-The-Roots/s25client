// Copyright (c) 2005 - 2018 Settlers Freaks (sf-team at siedler25.org)
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

/// Class for keeping track of the number of frames passed
/// Updates frame rate in specified intervalls (e.g. once per second),
/// so except for the first intervall there is always the frame rate (frames per second) of the last intervall
/// and the current intervall
class FrameCounter
{
public:
    using clock = std::chrono::steady_clock;

private:
    clock::duration updateInverval_; /// How often the FPS are updated
    unsigned framerate_;             /// Current FPS
    unsigned curNumFrames_;
    clock::time_point curStartTime_, lastUpdateTime_;

public:
    FrameCounter(clock::duration updateInverval = std::chrono::seconds(1));
    /// To be called after a frame has passed
    void update(clock::time_point curTime = clock::now());
    /// Get frame rate (FPS) as calculated in the last period
    unsigned getFrameRate() const { return framerate_; }
    /// Get frame rate (FPS) of the current interval
    unsigned getCurNumFrames() const { return curNumFrames_; }
    /// Get frame rate (FPS) of the current interval
    unsigned getCurFrameRate() const;
    clock::time_point getCurStartTime() const { return curStartTime_; }
    /// Return length of current intervall (start of the intervall until the last update call)
    clock::duration getCurIntervalLength() const { return lastUpdateTime_ - curStartTime_; }
    clock::duration getUpdateInterval() const { return updateInverval_; }
};

class FrameTimer
{
public:
    /// Clock used. Same as FrameCounter
    using clock = FrameCounter::clock;
    /// Resolution of the time between frames. Uses Clocks duration
    using duration_t = clock::duration;

private:
    /// How long a frame should be
    duration_t targetFrameDuration_;
    /// Time when the next frame should be drawn
    clock::time_point nextFrameTime_;
    /// How many frames to lag behind before adjusting
    unsigned maxLagFrames_;

public:
    FrameTimer(int targetFramerate = 60, unsigned maxLagFrames = 60, clock::time_point curTime = clock::now());
    void setTargetFramerate(int targetFramerate);
    /// Return time till next frame should be drawn
    duration_t calcTimeToNextFrame(clock::time_point curTime = clock::now()) const;
    /// Update state when frame is drawn
    void update(clock::time_point curTime = clock::now());
};

class FrameLimiter
{
    FrameTimer frameTimer_;

public:
    using clock = FrameTimer::clock;
    FrameLimiter();
    explicit FrameLimiter(FrameTimer frameTimer);
    void setTargetFramerate(int targetFramerate);
    /// Update state when frame is drawn
    void update(clock::time_point curTime = clock::now());
    void sleepTillNextFrame(clock::time_point curTime);
};
