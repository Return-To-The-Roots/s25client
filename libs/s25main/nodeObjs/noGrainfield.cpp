// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "noGrainfield.h"

#include "EventManager.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "network/GameClient.h"
#include "ogl/glSmartBitmap.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include "s25util/colors.h"

/// Length of growth waiting phase
const unsigned GROWING_WAITING_LENGTH = 1100;
/// Length of growth phase
const unsigned GROWING_LENGTH = 16;

noGrainfield::noGrainfield(const MapPoint pos)
    : noCoordBase(NodalObjectType::Grainfield, pos), type(RANDOM_RAND(2)), state(State::GrowingWaiting), size(0)
{
    event = GetEvMgr().AddEvent(this, GROWING_WAITING_LENGTH);
}

noGrainfield::~noGrainfield() = default;

void noGrainfield::Destroy()
{
    GetEvMgr().RemoveEvent(event);

    // Recalculate surrounding build-quality tiles
    world->RecalcBQAroundPoint(pos);

    noCoordBase::Destroy();
}

void noGrainfield::Serialize(SerializedGameData& sgd) const
{
    noCoordBase::Serialize(sgd);

    sgd.PushUnsignedChar(type);
    sgd.PushEnum<uint8_t>(state);
    sgd.PushUnsignedChar(size);
    sgd.PushEvent(event);
}

noGrainfield::noGrainfield(SerializedGameData& sgd, const unsigned obj_id)
    : noCoordBase(sgd, obj_id), type(sgd.PopUnsignedChar()), state(sgd.Pop<State>()), size(sgd.PopUnsignedChar()),
      event(sgd.PopEvent())
{}

void noGrainfield::Draw(DrawPoint drawPt)
{
    switch(state)
    {
        case State::GrowingWaiting:
        case State::Normal:
        {
            LOADER.grainfield_cache[type][size].draw(drawPt);
        }
        break;
        case State::Growing:
        {
            unsigned alpha = GAMECLIENT.Interpolate(0xFF, event);

            // Fade out old field
            LOADER.grainfield_cache[type][size].draw(drawPt, SetAlpha(COLOR_WHITE, 0xFF - alpha));

            // Fade in new field
            LOADER.grainfield_cache[type][size + 1].draw(drawPt, SetAlpha(COLOR_WHITE, alpha));
        }
        break;
        case State::Withering:
        {
            unsigned alpha = GAMECLIENT.Interpolate(0xFF, event);

            // Fade out field
            LOADER.grainfield_cache[type][size].draw(drawPt, SetAlpha(COLOR_WHITE, 0xFF - alpha));
        }
        break;
    }
}

void noGrainfield::HandleEvent(const unsigned /*id*/)
{
    switch(state)
    {
        case State::GrowingWaiting:
        {
            // Waiting phase ended, now it grows
            event = GetEvMgr().AddEvent(this, GROWING_LENGTH);
            state = State::Growing;
        }
        break;
        case State::Growing:
        {
            // If fully grown, stop here; otherwise schedule another waiting event to continue growth
            if(++size != 3)
            {
                event = GetEvMgr().AddEvent(this, GROWING_WAITING_LENGTH);
                // Wait for the next growth step
                state = State::GrowingWaiting;
            } else
            {
                // Fully grown now
                state = State::Normal;
                // Wither after a long time
                event = GetEvMgr().AddEvent(this, 3000 + RANDOM_RAND(1000));
            }
        }
        break;
        case State::Normal:
        {
            // It has existed too long now -> start disappearing
            state = State::Withering;
            event = GetEvMgr().AddEvent(this, 20);
        }
        break;
        case State::Withering:
        {
            // Destroy itself
            event = nullptr;
            world->SetNO(pos, nullptr);
            GetEvMgr().AddToKillList(this);
        }
        break;
    }
}

void noGrainfield::BeginHarvesting()
{
    // Cancel event so it does not disappear while being harvested
    GetEvMgr().RemoveEvent(event);
    event = nullptr;
    state = State::Normal;
}

void noGrainfield::EndHarvesting()
{
    // Wither after a long time (again)
    event = GetEvMgr().AddEvent(this, 3000 + RANDOM_RAND(1000));
}
