// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/noncopyable.hpp>
#include <map>
#include <vector>

class Animation;
struct ScreenResizeEvent;
class Window;

/// Class to handle the animations for all controls of a window
/// The parent must be alive during the lifetime of this instance
/// The time can have any reference but is expected to increase monotonically in steps of 1ms
class AnimationManager : private boost::noncopyable
{
public:
    AnimationManager(Window* parent);
    ~AnimationManager();
    /// Update the animations to the given time
    void update(unsigned time);
    /// Add an animation and return a non-null identifier
    unsigned addAnimation(Animation* animation);
    /// Return true, if the given animation still exists
    bool isAnimationActive(unsigned animId) const;
    /// Return the animation with the given id or nullptr if it doesn't exist
    Animation* getAnimation(unsigned animId);
    /// Return the id of a given animation
    unsigned getAnimationId(const Animation* animation) const;
    std::vector<Animation*> getElementAnimations(unsigned elementId) const;
    /// Remove all animations for this element (stopping them where they are)
    void removeElementAnimations(unsigned elementId);
    /// Finish the elements animations
    /// If finishImmediately is true, then execute their last frame (skipping all frames in between)
    /// else just play them to the end (setting repeat to oscillateOnce for oscillate or none for repeat)
    void finishElementAnimations(unsigned elementId, bool finishImmediately);
    /// Remove the animation with the given id
    void removeAnimation(unsigned animId);
    /// Finish the animation
    /// If finishImmediately is true, then execute the last frame (skipping all frames in between)
    /// else just play it to the end (setting repeat to oscillateOnce for oscillate or none for repeat)
    void finishAnimation(unsigned animId, bool finishImmediately);
    /// Return the number of active animations
    unsigned getNumActiveAnimations() const;
    /// React when the parents elements have been rescaled
    void onRescale(const ScreenResizeEvent& rs);

private:
    using AnimationMap = std::map<unsigned, Animation*>;
    Window* parent_;
    unsigned nextId_;
    AnimationMap animations_;
};
