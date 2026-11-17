// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "noGrapefield.h"

#include "EventManager.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "WineLoader.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glSmartBitmap.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include "s25util/colors.h"

/// Duration of the grow waiting phase
constexpr unsigned GROWING_WAITING_LENGTH = 1100;
/// Duration of grow phase
constexpr unsigned GROWING_LENGTH = 16;

using namespace wineaddon;

noGrapefield::noGrapefield(const MapPoint pos)
    : noCoordBase(NodalObjectType::Grapefield, pos), type(RANDOM_RAND(2)), state(State::GrowingWaiting), size(0)
{
    event = getEvMgr().addEvent(this, GROWING_WAITING_LENGTH);
}

noGrapefield::~noGrapefield() = default;

void noGrapefield::destroy()
{
    getEvMgr().removeEvent(event);

    // recompute BQ around
    world->recalcBQAroundPoint(pos);

    noCoordBase::destroy();
}

void noGrapefield::serialize(SerializedGameData& sgd) const
{
    noCoordBase::serialize(sgd);

    sgd.PushUnsignedChar(type);
    sgd.pushEnum<uint8_t>(state);
    sgd.PushUnsignedChar(size);
    sgd.pushEvent(event);
}

noGrapefield::noGrapefield(SerializedGameData& sgd, const unsigned obj_id)
    : noCoordBase(sgd, obj_id), type(sgd.PopUnsignedChar()), state(sgd.pop<State>()), size(sgd.PopUnsignedChar()),
      event(sgd.popEvent())
{}

void noGrapefield::draw(DrawPoint drawPt)
{
    switch(state)
    {
        case State::GrowingWaiting:
        case State::Normal:
        {
            grapefield_cache[type][size].draw(drawPt);
        }
        break;
        case State::Growing:
        {
            unsigned alpha = GAMECLIENT.interpolate(0xFF, event);

            // hiding old field
            grapefield_cache[type][size].draw(drawPt, SetAlpha(COLOR_WHITE, 0xFF - alpha));

            // showing new field
            grapefield_cache[type][size + 1].draw(drawPt, SetAlpha(COLOR_WHITE, alpha));
        }
        break;
        case State::Withering:
        {
            unsigned alpha = GAMECLIENT.interpolate(0xFF, event);

            // hiding old field
            grapefield_cache[type][size].draw(drawPt, SetAlpha(COLOR_WHITE, 0xFF - alpha));
        }
        break;
    }
}

void noGrapefield::handleEvent(const unsigned /*id*/)
{
    switch(state)
    {
        case State::GrowingWaiting:
        {
            // Field was waiting, so it is growing now
            event = getEvMgr().addEvent(this, GROWING_LENGTH);
            state = State::Growing;
        }
        break;
        case State::Growing:
        {
            // When not fully grown, add an additional wait event, so the field can grow further
            if(++size != 3)
            {
                event = getEvMgr().addEvent(this, GROWING_WAITING_LENGTH);
                // Wait until the next growing phase
                state = State::GrowingWaiting;
            } else
            {
                // Fully grown
                state = State::Normal;
                // wither after large time
                event = getEvMgr().addEvent(this, 3000 + RANDOM_RAND(1000));
            }
        }
        break;
        case State::Normal:
        {
            // Living to long --> start vanishing
            state = State::Withering;
            event = getEvMgr().addEvent(this, 20);
        }
        break;
        case State::Withering:
        {
            // Self destruction
            event = nullptr;
            world->setNO(pos, nullptr);
            getEvMgr().addToKillList(this);
        }
        break;
    }
}

unsigned noGrapefield::getHarvestID() const
{
    return bobIndex[BobTypes(rttr::enumCast(BobTypes::WINEGROWER_GRAPEFIELDS_ONE) + type)] + 4;
}

void noGrapefield::beginHarvesting()
{
    // Kill event, so we do not disappear when we are harvested
    getEvMgr().removeEvent(event);
    event = nullptr;
    state = State::Normal;
}

void noGrapefield::abortHarvesting()
{
    // Wither after long time
    event = getEvMgr().addEvent(this, 3000 + RANDOM_RAND(1000));
}
