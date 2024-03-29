// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AnimationManager.h"
#include "Window.h"
#include "animation/Animation.h"
#include "helpers/containerUtils.h"
#include <boost/range/adaptor/map.hpp>

AnimationManager::AnimationManager(Window* parent) : parent_(parent)
{
    // Use a start id based on the parent id to detect calls to wrong animation managers
    nextId_ = parent->GetID() << 16;
}

AnimationManager::~AnimationManager()
{
    for(Animation* animation : animations_ | boost::adaptors::map_values)
        delete animation;
}

void AnimationManager::update(unsigned time)
{
    for(auto it = animations_.begin(); it != animations_.end(); /* no inc */)
    {
        it->second->update(time, parent_);
        if(it->second->isFinished())
        {
            delete it->second;
            it = animations_.erase(it);
        } else
            ++it;
    }
}

unsigned AnimationManager::addAnimation(Animation* animation)
{
    if(!animation)
        return 0u;
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
    if(!animId)
        return false;
    return helpers::contains(animations_, animId);
}

Animation* AnimationManager::getAnimation(unsigned animId)
{
    if(!animId)
        return nullptr;
    auto it = animations_.find(animId);
    return it == animations_.end() ? nullptr : it->second;
}

unsigned AnimationManager::getAnimationId(const Animation* animation) const
{
    for(const AnimationMap::value_type& val : animations_)
    {
        if(val.second == animation)
            return val.first;
    }
    return 0u;
}

std::vector<Animation*> AnimationManager::getElementAnimations(unsigned elementId) const
{
    std::vector<Animation*> result;
    for(Animation* animation : animations_ | boost::adaptors::map_values)
    {
        if(animation->getElementId() == elementId)
            result.push_back(animation);
    }
    return result;
}

void AnimationManager::removeElementAnimations(unsigned elementId)
{
    std::vector<Animation*> elAnims = getElementAnimations(elementId);
    for(Animation* anim : elAnims)
    {
        removeAnimation(getAnimationId(anim));
    }
}

void AnimationManager::finishElementAnimations(unsigned elementId, bool finishImmediately)
{
    std::vector<Animation*> elAnims = getElementAnimations(elementId);
    for(Animation* anim : elAnims)
    {
        finishAnimation(getAnimationId(anim), finishImmediately);
    }
}

void AnimationManager::removeAnimation(unsigned animId)
{
    auto it = animations_.find(animId);
    if(it != animations_.end())
    {
        delete it->second;
        animations_.erase(it);
    }
}

void AnimationManager::finishAnimation(unsigned animId, bool finishImmediately)
{
    Animation* anim = getAnimation(animId);
    if(!anim)
        return;
    anim->finish(parent_, finishImmediately);
    if(anim->isFinished())
        removeAnimation(animId);
}

unsigned AnimationManager::getNumActiveAnimations() const
{
    return animations_.size();
}

void AnimationManager::onRescale(const ScreenResizeEvent& rs)
{
    for(Animation* animation : animations_ | boost::adaptors::map_values)
        animation->onRescale(rs);
}
