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
const unsigned GROWING_WAITING_LENGTH = 1100;
/// Duration of grow phase
const unsigned GROWING_LENGTH = 16;

using namespace wineaddon;

noGrapefield::noGrapefield(const MapPoint pos)
    : noCoordBase(NodalObjectType::Grapefield, pos), type(RANDOM_RAND(2)), state(State::GrowingWaiting), size(0)
{
    event = GetEvMgr().AddEvent(this, GROWING_WAITING_LENGTH);
}

noGrapefield::~noGrapefield() = default;

void noGrapefield::Destroy()
{
    GetEvMgr().RemoveEvent(event);

    // recompute BQ around
    world->RecalcBQAroundPoint(pos);

    noCoordBase::Destroy();
}

void noGrapefield::Serialize(SerializedGameData& sgd) const
{
    noCoordBase::Serialize(sgd);

    sgd.PushUnsignedChar(type);
    sgd.PushEnum<uint8_t>(state);
    sgd.PushUnsignedChar(size);
    sgd.PushEvent(event);
}

noGrapefield::noGrapefield(SerializedGameData& sgd, const unsigned obj_id)
    : noCoordBase(sgd, obj_id), type(sgd.PopUnsignedChar()), state(sgd.Pop<State>()), size(sgd.PopUnsignedChar()),
      event(sgd.PopEvent())
{}

void noGrapefield::Draw(DrawPoint drawPt)
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
            unsigned alpha = GAMECLIENT.Interpolate(0xFF, event);

            // hiding old field
            grapefield_cache[type][size].draw(drawPt, SetAlpha(COLOR_WHITE, 0xFF - alpha));

            // showing new field
            grapefield_cache[type][size + 1].draw(drawPt, SetAlpha(COLOR_WHITE, alpha));
        }
        break;
        case State::Withering:
        {
            unsigned alpha = GAMECLIENT.Interpolate(0xFF, event);

            // hiding old field
            grapefield_cache[type][size].draw(drawPt, SetAlpha(COLOR_WHITE, 0xFF - alpha));
        }
        break;
    }
}

void noGrapefield::HandleEvent(const unsigned /*id*/)
{
    switch(state)
    {
        case State::GrowingWaiting:
        {
            // Field was waiting, so it is growing now
            event = GetEvMgr().AddEvent(this, GROWING_LENGTH);
            state = State::Growing;
        }
        break;
        case State::Growing:
        {
            // When not fully grown, add an additional wait event, so the field can grow further
            if(++size != 3)
            {
                event = GetEvMgr().AddEvent(this, GROWING_WAITING_LENGTH);
                // Wait until the next growing phase
                state = State::GrowingWaiting;
            } else
            {
                // Fully grown
                state = State::Normal;
                // wither after large time
                event = GetEvMgr().AddEvent(this, 3000 + RANDOM_RAND(1000));
            }
        }
        break;
        case State::Normal:
        {
            // Living to long --> hokus pokus verschwindibus!
            state = State::Withering;
            event = GetEvMgr().AddEvent(this, 20);
        }
        break;
        case State::Withering:
        {
            // Self destruction
            event = nullptr;
            world->SetNO(pos, nullptr);
            GetEvMgr().AddToKillList(this);
        }
        break;
    }
}

unsigned noGrapefield::GetHarvestID() const
{
    return bobIndex[BobTypes(rttr::enum_cast(BobTypes::WINEGROWER_GRAPEFIELDS_ONE) + type)] + 4;
}

void noGrapefield::BeginHarvesting()
{
    // Kill event, so we do not disappear when we are harvested
    GetEvMgr().RemoveEvent(event);
    event = nullptr;
    state = State::Normal;
}

void noGrapefield::EndHarvesting()
{
    // Wither after long time
    event = GetEvMgr().AddEvent(this, 3000 + RANDOM_RAND(1000));
}
