// Copyright (C) 2017 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

class Window;
struct ScreenResizeEvent;

class Animation
{
public:
    enum class RepeatType
    {
        None,
        Repeat,
        Oscillate,
        OscillateOnce
    };
    enum class SkipType
    {
        /// Skip on time, play every frame
        Time,
        /// Ensure timing, skip frames (last frame is always played before anim is finished)
        Frames
    };
    Animation(Window* element, unsigned numFrames, unsigned frameRate, RepeatType repeat);
    virtual ~Animation();

    unsigned getElementId() const { return elementId_; }
    unsigned getFrameRate() const { return frameRate_; }
    void setFrameRate(unsigned frameRate);
    unsigned getNumFrames() const { return numFrames_; }
    void setNumFrames(unsigned numFrames);
    RepeatType getRepeat() const { return repeat_; }
    void setRepeat(RepeatType repeat) { repeat_ = repeat; };
    unsigned getCurFrame() const { return curFrame_; }
    bool isLastFrame() const;
    SkipType getSkipType() const { return skipType_; }
    void setSkipType(SkipType skipType) { skipType_ = skipType; }

    void update(unsigned time, Window* parent);
    /// Finish the animations
    /// If finishImmediatelly is true, then execute the last frame (skipping all frames in between)
    /// else just let them play them to the end (setting repeat to oscillateOnce for oscillate or none for repeat)
    void finish(Window* parent, bool finishImmediately);

    /// Return true when the animation is done
    virtual bool isFinished() const;
    /// React when the parents elements have been rescaled
    virtual void onRescale(const ScreenResizeEvent&) {}

protected:
    /// Function for subclasses to override to do the actual animation
    /// Gets the element and the time into the next frame as a fraction of the frameRate [0..1)
    /// nextFramepartTime will be zero for the last frame
    virtual void doUpdate(Window* element, double nextFramepartTime) = 0;
    /// Return the current factor [0, 1] for linearly interpolating over the duration if the animation
    double getCurLinearInterpolationFactor(double nextFramepartTime) const;

private:
    /// Executes the current frame
    void execFrame(Window* parent, unsigned remainingTime);
    /// Advance to frame according to passed time
    void advanceFrames(unsigned passedTime);
    /// Advance to next frame
    void advanceFrame();

    /// ID of animated element
    unsigned elementId_;
    /// number of frames
    unsigned numFrames_;
    /// milliseconds per frame
    unsigned frameRate_;
    /// what to do after the end of the cycle
    RepeatType repeat_;
    /// last update time
    unsigned lastTime_;
    /// current frame number
    unsigned curFrame_;
    /// are we counting up or down (oscillate)
    bool countUp_;
    /// What to do when the passed time exceeds more than one frame
    SkipType skipType_;
    /// Whether the animation has started (frame 0 called, time set)
    bool hasStarted_;
};
