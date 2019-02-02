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

#include "rttrDefines.h" // IWYU pragma: keep
#include "FrameCounter.h"
#include "helpers/win32_nanosleep.h" // IWYU pragma: keep
#include <algorithm>
#include <cmath>
#include <cstdlib>

//-V:clock::time_point:813

FrameCounter::FrameCounter(clock::duration updateInverval) : updateInverval_(updateInverval), framerate_(0), curNumFrames_(0) {}

void FrameCounter::update(clock::time_point curTime)
{
    lastUpdateTime_ = curTime;
    if(curNumFrames_ == 0)
        curStartTime_ = curTime;
    else
    {
        if(lastUpdateTime_ - curStartTime_ >= updateInverval_)
        {
            framerate_ = getCurFrameRate();
            curStartTime_ = curTime;
            curNumFrames_ = 0;
        }
    }
    curNumFrames_++;
}

unsigned FrameCounter::getCurFrameRate() const
{
    clock::duration timeDiff = lastUpdateTime_ - curStartTime_;
    if(timeDiff == clock::duration::zero())
        return 0;
    typedef std::chrono::duration<double> dSeconds;
    return std::lround(curNumFrames_ / std::chrono::duration_cast<dSeconds>(timeDiff).count());
}

FrameTimer::FrameTimer(int targetFramerate, unsigned maxLagFrames, clock::time_point curTime)
    : targetFrameDuration_(duration_t::zero()), nextFrameTime_(curTime), maxLagFrames_(maxLagFrames)
{
    setTargetFramerate(targetFramerate);
}

void FrameTimer::setTargetFramerate(int targetFramerate)
{
    using namespace std::chrono;
    nextFrameTime_ -= targetFrameDuration_;
    if(targetFramerate <= 0)
        targetFrameDuration_ = duration_t::zero(); // Disabled
    else
        targetFrameDuration_ = duration_cast<duration_t>(seconds(1)) / std::min(targetFramerate, 200); // Max 200FPS
    nextFrameTime_ += targetFrameDuration_;
}

FrameTimer::duration_t FrameTimer::calcTimeToNextFrame(clock::time_point curTime) const
{
    using namespace std::chrono;
    if(targetFrameDuration_ == duration_t::zero())
        return clock::duration::zero();

    if(curTime < nextFrameTime_)
        return nextFrameTime_ - curTime;
    else
        return duration_t::zero();
}

void FrameTimer::update(clock::time_point curTime)
{
    // Ideal: nextFrameTime == curTime -> Current frame is punctual
    // Normal: nextFrameTime + x == curTime; -targetFrameDuration < x < targetFrameDuration (1 frame early to 1 frame late)
    // Problem: The calculations can take long so every frame is late making nextFrameTime_ be further and further behind current time
    // So even when changing the target we will never catch up -> Limit the time difference
    if(nextFrameTime_ + maxLagFrames_ * targetFrameDuration_ < curTime)
        nextFrameTime_ = curTime;
    else
    {
        // Set the time for the next frame exactly 1 frame after the last
        // This way if the current frame was late, the next will be early and vice versa
        nextFrameTime_ += targetFrameDuration_;
    }
}

FrameLimiter::FrameLimiter() {}

FrameLimiter::FrameLimiter(FrameTimer frameTimer) : frameTimer_(frameTimer) {}

void FrameLimiter::setTargetFramerate(int targetFramerate)
{
    frameTimer_.setTargetFramerate(targetFramerate);
}

void FrameLimiter::update(clock::time_point curTime)
{
    frameTimer_.update(curTime);
}

void FrameLimiter::sleepTillNextFrame(clock::time_point curTime)
{
    using namespace std::chrono;
    nanoseconds waitTime = duration_cast<nanoseconds>(frameTimer_.calcTimeToNextFrame(curTime));
    // No time to waste?
    if(waitTime <= nanoseconds::zero())
        return;
#ifdef _WIN32
    if(waitTime < milliseconds(13)) // timer resolutions < 13ms do not work for windows correctly. TODO: Still true?
        return;
#endif

    timespec req;
    req.tv_sec = static_cast<time_t>(duration_cast<seconds>(waitTime).count());
    req.tv_nsec = static_cast<long>((waitTime - seconds(req.tv_sec)).count());

    while(nanosleep(&req, &req) == -1)
        continue;
}
