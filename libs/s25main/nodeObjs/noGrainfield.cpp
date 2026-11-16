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

/// Länge des Wachs-Wartens
const unsigned GROWING_WAITING_LENGTH = 1100;
/// Länge des Wachsens
const unsigned GROWING_LENGTH = 16;

noGrainfield::noGrainfield(const MapPoint pos)
    : noCoordBase(NodalObjectType::Grainfield, pos), type(RANDOM_RAND(2)), state(State::GrowingWaiting), size(0)
{
    event = getEvMgr().addEvent(this, GROWING_WAITING_LENGTH);
}

noGrainfield::~noGrainfield() = default;

void noGrainfield::destroy()
{
    getEvMgr().removeEvent(event);

    // Bauplätze drumrum neu berechnen
    world->recalcBQAroundPoint(pos);

    noCoordBase::destroy();
}

void noGrainfield::Serialize(SerializedGameData& sgd) const
{
    noCoordBase::Serialize(sgd);

    sgd.PushUnsignedChar(type);
    sgd.pushEnum<uint8_t>(state);
    sgd.PushUnsignedChar(size);
    sgd.pushEvent(event);
}

noGrainfield::noGrainfield(SerializedGameData& sgd, const unsigned obj_id)
    : noCoordBase(sgd, obj_id), type(sgd.PopUnsignedChar()), state(sgd.pop<State>()), size(sgd.PopUnsignedChar()),
      event(sgd.popEvent())
{}

void noGrainfield::draw(DrawPoint drawPt)
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
            unsigned alpha = GAMECLIENT.interpolate(0xFF, event);

            // altes Feld ausblenden
            LOADER.grainfield_cache[type][size].draw(drawPt, SetAlpha(COLOR_WHITE, 0xFF - alpha));

            // neues Feld einblenden
            LOADER.grainfield_cache[type][size + 1].draw(drawPt, SetAlpha(COLOR_WHITE, alpha));
        }
        break;
        case State::Withering:
        {
            unsigned alpha = GAMECLIENT.interpolate(0xFF, event);

            // Feld ausblenden
            LOADER.grainfield_cache[type][size].draw(drawPt, SetAlpha(COLOR_WHITE, 0xFF - alpha));
        }
        break;
    }
}

void noGrainfield::handleEvent(const unsigned /*id*/)
{
    switch(state)
    {
        case State::GrowingWaiting:
        {
            // Feld hat gewartet, also wächst es jetzt
            event = getEvMgr().addEvent(this, GROWING_LENGTH);
            state = State::Growing;
        }
        break;
        case State::Growing:
        {
            // Wenn er ausgewachsen ist, dann nicht, ansonsten nochmal ein "Warteevent" anmelden, damit er noch weiter
            // wächst
            if(++size != 3)
            {
                event = getEvMgr().addEvent(this, GROWING_WAITING_LENGTH);
                // Erstmal wieder bis zum nächsten Wachsstumsschub warten
                state = State::GrowingWaiting;
            } else
            {
                // bin nun ausgewachsen
                state = State::Normal;
                // nach langer Zeit verdorren
                event = getEvMgr().addEvent(this, 3000 + RANDOM_RAND(1000));
            }
        }
        break;
        case State::Normal:
        {
            // Jetzt lebt es schon zu lange --> hokus pokus verschwindibus!
            state = State::Withering;
            event = getEvMgr().addEvent(this, 20);
        }
        break;
        case State::Withering:
        {
            // Selbst zerstören
            event = nullptr;
            world->setNO(pos, nullptr);
            getEvMgr().addToKillList(this);
        }
        break;
    }
}

void noGrainfield::beginHarvesting()
{
    // Event killen, damit wir nicht plötzlich verschwinden, wenn er uns aberntet
    getEvMgr().removeEvent(event);
    event = nullptr;
    state = State::Normal;
}

void noGrainfield::endHarvesting()
{
    // nach langer Zeit verdorren (von neuem)
    event = getEvMgr().addEvent(this, 3000 + RANDOM_RAND(1000));
}
