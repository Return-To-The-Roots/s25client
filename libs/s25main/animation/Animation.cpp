// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Animation.h"
#include "Window.h"

Animation::Animation(Window* element, unsigned numFrames, unsigned frameRate, RepeatType repeat)
    : elementId_(element->GetID()), numFrames_(numFrames), frameRate_(frameRate), repeat_(repeat), lastTime_(0),
      curFrame_(0), countUp_(true), skipType_(SkipType::Frames), hasStarted_(false)
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
        repeat_ = RepeatType::None;
        countUp_ = true;
        curFrame_ = numFrames_ - 1u;
    }
    // Do not overshoot on last frame unless we are in oscillate mode
    if(isLastFrame() && repeat_ != RepeatType::Oscillate && (repeat_ != RepeatType::OscillateOnce || !countUp_))
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
        if(skipType_ == SkipType::Time)
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
            if(repeat_ == RepeatType::None)
            {
                // Maximum number of frames passed are the number of remaining frames
                numFramesPassed = std::min(numFramesPassed, numFrames_ - 1 - curFrame_);
            } else if(repeat_ == RepeatType::Repeat)
            {
                // We can reduce the number of passed frames by full cycles
                // So passing 6 frames on an animation with 5 frames is the same as just passing 1 frame
                numFramesPassed = numFramesPassed % numFrames_;
            } else if(repeat_ == RepeatType::Oscillate)
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
            RTTR_Assert(repeat_ != RepeatType::None);
            if(repeat_ == RepeatType::Oscillate || repeat_ == RepeatType::OscillateOnce)
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
            RTTR_Assert(repeat_ != RepeatType::None && repeat_ != RepeatType::OscillateOnce);
            if(repeat_ == RepeatType::Oscillate)
            {
                countUp_ = true;
                curFrame_ = 1u;
            } else
                curFrame_ = numFrames_ - 1u;
        } else
            curFrame_--;
    }
    if((repeat_ == RepeatType::Oscillate && isLastFrame())
       || (repeat_ == RepeatType::OscillateOnce && curFrame_ + 1u >= numFrames_))
        countUp_ = !countUp_;
}

void Animation::finish(Window* parent, bool finishImmediately)
{
    if(repeat_ == RepeatType::Repeat)
        repeat_ = RepeatType::None;
    else if(repeat_ == RepeatType::Oscillate)
        repeat_ = RepeatType::OscillateOnce;
    if(!hasStarted_)
    {
        // When we haven't started yet, just execute the last frame immediately
        if(repeat_ == RepeatType::OscillateOnce)
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
    if(repeat_ == RepeatType::OscillateOnce && curFrame_ == 0u)
        countUp_ = false;
    if(!finishImmediately || isFinished())
        return;
    if(repeat_ == RepeatType::None)
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
    if(repeat_ == RepeatType::None)
        return isLastFrame();
    else if(repeat_ == RepeatType::OscillateOnce)
        return !countUp_ && curFrame_ == 0;
    else
        return false;
}
