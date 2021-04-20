// Copyright (c) 2005 - 2018 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
