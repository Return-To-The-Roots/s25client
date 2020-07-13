// Copyright (c) 2017 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "Animation.h"
#include "Window.h"

Animation::Animation(Window* element, unsigned numFrames, unsigned frameRate, RepeatType repeat)
    : elementId_(element->GetID()), numFrames_(numFrames), frameRate_(frameRate), repeat_(repeat), lastTime_(0), curFrame_(0),
      countUp_(true), skipType_(SKIP_FRAMES), hasStarted_(false)
{
    // We need at least 2 frames: current state and next state
    RTTR_Assert(numFrames_ > 1);
    RTTR_Assert(frameRate_ > 0);
}

Animation::~Animation() = default;

void Animation::setFrameRate(unsigned frameRate)
{
    RTTR_Assert(frameRate > 0u);
    frameRate_ = frameRate;
}

void Animation::setNumFrames(unsigned numFrames)
{
    RTTR_Assert(numFrames > 1u);
    numFrames_ = numFrames;
    if(curFrame_ >= numFrames_)
        curFrame_ = numFrames_ - 1u;
}

void Animation::update(unsigned time, Window* parent)
{
    RTTR_Assert(!isFinished());
    // First update: Just set time
    if(!hasStarted_)
    {
        // Initialize and start
        curFrame_ = 0u;
        lastTime_ = time;
        countUp_ = true;
        hasStarted_ = true;
    } else
    {
        unsigned passedTime = time - lastTime_;
        // Next frame not there -> Out
        if(passedTime < frameRate_)
            return;
        advanceFrames(passedTime);
    }

    unsigned remainingTime = time - lastTime_;
    execFrame(parent, remainingTime);
}

void Animation::execFrame(Window* parent, unsigned remainingTime)
{
    RTTR_Assert(curFrame_ < numFrames_);
    RTTR_Assert(remainingTime < frameRate_);
    auto* element = parent->GetCtrl<Window>(elementId_);
    // Element missing -> Done
    if(!element)
    {
        repeat_ = RPT_None;
        countUp_ = true;
        curFrame_ = numFrames_ - 1u;
    }
    // Do not overshoot on last frame unless we are in oscillate mode
    if(isLastFrame() && repeat_ != RPT_Oscillate && (repeat_ != RPT_OscillateOnce || !countUp_))
        remainingTime = 0;
    doUpdate(element, static_cast<double>(remainingTime) / static_cast<double>(frameRate_));
}

bool Animation::isLastFrame() const
{
    return (countUp_ && curFrame_ + 1 >= numFrames_) || (!countUp_ && curFrame_ == 0);
}

double Animation::getCurLinearInterpolationFactor(double nextFramepartTime) const
{
    if(!countUp_)
        nextFramepartTime = -nextFramepartTime;
    double result = (getCurFrame() + nextFramepartTime) / (getNumFrames() - 1u);
    RTTR_Assert(result >= 0. && result <= 1.);
    return result;
}

void Animation::advanceFrames(unsigned passedTime)
{
    unsigned numFramesPassed = passedTime / frameRate_;
    if(numFramesPassed > 1u)
    {
        if(skipType_ == SKIP_TIME)
        {
            // Advance time by the full passed time less 1 frame
            // Add 1ms so the next frame is just not due yet
            lastTime_ += passedTime - frameRate_ + 1;
            // Play one frame
            numFramesPassed = 1u;
        } else
        {
            // Advance time by full frames passed
            lastTime_ += numFramesPassed * frameRate_;
            if(repeat_ == RPT_None)
            {
                // Maximum number of frames passed are the number of remaining frames
                numFramesPassed = std::min(numFramesPassed, numFrames_ - 1 - curFrame_);
            } else if(repeat_ == RPT_Repeat)
            {
                // We can reduce the number of passed frames by full cycles
                // So passing 6 frames on an animation with 5 frames is the same as just passing 1 frame
                numFramesPassed = numFramesPassed % numFrames_;
            } else if(repeat_ == RPT_Oscillate)
            {
                // Oscillate is similar to repeat but we have twice as many frames less one (last is not played twice)
                numFramesPassed = numFramesPassed % (numFrames_ * 2u - 2u);
            } else
            {
                // Maximum number of frames passed are the number of remaining frames
                unsigned numFramesLeft = (countUp_) ? numFrames_ * 2u - 2u - curFrame_ : curFrame_;
                numFramesPassed = std::min(numFramesPassed, numFramesLeft);
            }
        }
    } else
    {
        // Advance time by one frame
        lastTime_ += frameRate_;
    }

    for(unsigned i = 0; i < numFramesPassed; i++)
        advanceFrame();
}

void Animation::advanceFrame()
{
    if(countUp_)
    {
        curFrame_++;
        if(curFrame_ >= numFrames_)
        {
            RTTR_Assert(repeat_ != RPT_None);
            if(repeat_ == RPT_Oscillate || repeat_ == RPT_OscillateOnce)
            {
                countUp_ = false;
                curFrame_ = numFrames_ - 2u;
            } else
                curFrame_ = 0u;
        }
    } else
    {
        if(curFrame_ == 0)
        {
            RTTR_Assert(repeat_ != RPT_None && repeat_ != RPT_OscillateOnce);
            if(repeat_ == RPT_Oscillate)
            {
                countUp_ = true;
                curFrame_ = 1u;
            } else
                curFrame_ = numFrames_ - 1u;
        } else
            curFrame_--;
    }
    if((repeat_ == RPT_Oscillate && isLastFrame()) || (repeat_ == RPT_OscillateOnce && curFrame_ + 1u >= numFrames_))
        countUp_ = !countUp_;
}

void Animation::finish(Window* parent, bool finishImmediately)
{
    if(repeat_ == RPT_Repeat)
        repeat_ = RPT_None;
    else if(repeat_ == RPT_Oscillate)
        repeat_ = RPT_OscillateOnce;
    if(!hasStarted_)
    {
        // When we haven't started yet, just execute the last frame immediately
        if(repeat_ == RPT_OscillateOnce)
        {
            curFrame_ = 0u;
            countUp_ = false;
        } else
            curFrame_ = numFrames_ - 1u;
        hasStarted_ = true;
        execFrame(parent, 0u);
        return;
    }
    // Finish immediately if we are at the start/end
    if(repeat_ == RPT_OscillateOnce && curFrame_ == 0u)
        countUp_ = false;
    if(!finishImmediately || isFinished())
        return;
    if(repeat_ == RPT_None)
        curFrame_ = numFrames_ - 1u;
    else
    {
        curFrame_ = 0u;
        countUp_ = false;
    }
    execFrame(parent, 0u);
}

bool Animation::isFinished() const
{
    if(repeat_ == RPT_None)
        return isLastFrame();
    else if(repeat_ == RPT_OscillateOnce)
        return !countUp_ && curFrame_ == 0;
    else
        return false;
}
