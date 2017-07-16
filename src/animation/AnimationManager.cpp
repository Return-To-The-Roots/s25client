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

#include "defines.h" // IWYU pragma: keep
#include "AnimationManager.h"
#include "animation/Animation.h"
#include "Window.h"
#include "helpers/containerUtils.h"
#include "helpers/mapTraits.h"
#include <boost/foreach.hpp>
#include <boost/range/adaptor/map.hpp>

AnimationManager::AnimationManager(Window* parent): parent_(parent)
{
    // Use a start id based on the parent id to detect calls to wrong animation managers
    nextId_ = parent->GetID() << 16;
}

AnimationManager::~AnimationManager()
{
    BOOST_FOREACH(Animation* animation, animations_ | boost::adaptors::map_values)
        delete animation;
}

void AnimationManager::update(unsigned time)
{
    for(AnimationMap::iterator it = animations_.begin(); it != animations_.end(); /* no inc */)
    {
        it->second->update(time, parent_);
        if(it->second->isFinished())
        {
            delete it->second;
            it = helpers::erase(animations_, it);
        } else
            ++it;
    }
}

unsigned AnimationManager::addAnimation(Animation* animation)
{
    // Make sure we don't add an animation twice
    if(getAnimationId(animation) != 0u)
        return 0u;
    // The element must be inside the parernt
    RTTR_Assert(parent_->GetCtrl<Window>(animation->getElementId()));
    // Make the id non-zero
    if(!nextId_)
        nextId_ = 1;
    RTTR_Assert(!isAnimationActive(nextId_));
    animations_[nextId_] = animation;
    return nextId_++;
}

bool AnimationManager::isAnimationActive(unsigned animId) const
{
    return helpers::contains(animations_, animId);
}

Animation* AnimationManager::getAnimation(unsigned animId)
{
    AnimationMap::iterator it = animations_.find(animId);
    return it == animations_.end() ? NULL : it->second;
}

unsigned AnimationManager::getAnimationId(const Animation* animation) const
{
    BOOST_FOREACH(const AnimationMap::value_type& val, animations_)
    {
        if(val.second == animation)
            return val.first;
    }
    return 0u;
}

std::vector<Animation*> AnimationManager::getElementAnimations(unsigned elementId) const
{
    std::vector<Animation*> result;
    BOOST_FOREACH(Animation* animation, animations_ | boost::adaptors::map_values)
    {
        if(animation->getElementId() == elementId)
            result.push_back(animation);
    }
    return result;
}

void AnimationManager::removeElementAnimations(unsigned elementId)
{
    for(AnimationMap::iterator it = animations_.begin(); it != animations_.end(); /* no inc */)
    {
        if(it->second->getElementId() == elementId)
        {
            delete it->second;
            it = helpers::erase(animations_, it);
        } else
            ++it;
    }
}

void AnimationManager::removeAnimation(unsigned animId)
{
    animations_.erase(animId);
}

unsigned AnimationManager::getNumActiveAnimations() const
{
    return animations_.size();
}
