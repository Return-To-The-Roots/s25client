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

#ifndef AnimationManager_h__
#define AnimationManager_h__

#include <boost/noncopyable.hpp>
#include <map>
#include <vector>

class Animation;
struct ScreenResizeEvent;
class Window;

/// Class to handle the animations for all controls of a window
/// The parent must be alive during the lifetime of this instance
/// The time can have any reference but is expected to increase monotonically in steps of 1ms
class AnimationManager: private boost::noncopyable
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
    /// Return the animation with the given id or NULL if it doesn't exist
    Animation* getAnimation(unsigned animId);
    /// Return the id of a given animation
    unsigned getAnimationId(const Animation* animation) const;
    std::vector<Animation*> getElementAnimations(unsigned elementId) const;
    void removeElementAnimations(unsigned elementId);
    /// Remove the animation with the given id
    void removeAnimation(unsigned animId);
    /// Return the number of active animations
    unsigned getNumActiveAnimations() const;
    /// React when the parents elements have been rescaled
    void onRescale(const ScreenResizeEvent& rs);
private:
    typedef std::map<unsigned, Animation*> AnimationMap;
    Window* parent_;
    unsigned nextId_;
    AnimationMap animations_;
};

#endif // AnimationManager_h__
