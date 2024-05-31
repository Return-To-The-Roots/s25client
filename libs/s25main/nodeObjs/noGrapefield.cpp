// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "noGrapefield.h"

#include "EventManager.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "network/GameClient.h"
#include "ogl/glSmartBitmap.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include "s25util/colors.h"
#include "ogl/glArchivItem_Bitmap.h"

/// Länge des Wachs-Wartens
const unsigned GROWING_WAITING_LENGTH = 1100;
/// Länge des Wachsens
const unsigned GROWING_LENGTH = 16;

noGrapefield::noGrapefield(const MapPoint pos)
    : noCoordBase(NodalObjectType::Grapefield, pos), type(RANDOM_RAND(2)), state(State::GrowingWaiting), size(0)
{
    event = GetEvMgr().AddEvent(this, GROWING_WAITING_LENGTH);
}

noGrapefield::~noGrapefield() = default;

void noGrapefield::Destroy()
{
    GetEvMgr().RemoveEvent(event);

    // Bauplätze drumrum neu berechnen
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
            LOADER.GetImageN("wine_bobs", 27 + 5 * type + size)->DrawFull(drawPt);
        }
        break;
        case State::Growing:
        {
            unsigned alpha = GAMECLIENT.Interpolate(0xFF, event);

            // altes Feld ausblenden
            LOADER.GetImageN("wine_bobs", 27 + 5 * type + size)->DrawFull(drawPt, SetAlpha(COLOR_WHITE, 0xFF - alpha));

            // neues Feld einblenden
            LOADER.GetImageN("wine_bobs", 27 + 5 * type + size + 1)->DrawFull(drawPt, SetAlpha(COLOR_WHITE, alpha));
        }
        break;
        case State::Withering:
        {
            unsigned alpha = GAMECLIENT.Interpolate(0xFF, event);

            // Feld ausblenden
            LOADER.GetImageN("wine_bobs", 27 + 5 * type)->DrawFull(drawPt, SetAlpha(COLOR_WHITE, 0xFF - alpha));
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
            // Feld hat gewartet, also wächst es jetzt
            event = GetEvMgr().AddEvent(this, GROWING_LENGTH);
            state = State::Growing;
        }
        break;
        case State::Growing:
        {
            // Wenn er ausgewachsen ist, dann nicht, ansonsten nochmal ein "Warteevent" anmelden, damit er noch weiter
            // wächst
            if(++size != 3)
            {
                event = GetEvMgr().AddEvent(this, GROWING_WAITING_LENGTH);
                // Erstmal wieder bis zum nächsten Wachsstumsschub warten
                state = State::GrowingWaiting;
            } else
            {
                // bin nun ausgewachsen
                state = State::Normal;
                // nach langer Zeit verdorren
                event = GetEvMgr().AddEvent(this, 3000 + RANDOM_RAND(1000));
            }
        }
        break;
        case State::Normal:
        {
            // Jetzt lebt es schon zu lange --> hokus pokus verschwindibus!
            state = State::Withering;
            event = GetEvMgr().AddEvent(this, 20);
        }
        break;
        case State::Withering:
        {
            // Selbst zerstören
            event = nullptr;
            world->SetNO(pos, nullptr);
            GetEvMgr().AddToKillList(this);
        }
        break;
    }
}

void noGrapefield::BeginHarvesting()
{
    // Event killen, damit wir nicht plötzlich verschwinden, wenn er uns aberntet
    GetEvMgr().RemoveEvent(event);
    event = nullptr;
    state = State::Normal;
}

void noGrapefield::EndHarvesting()
{
    // nach langer Zeit verdorren (von neuem)
    event = GetEvMgr().AddEvent(this, 3000 + RANDOM_RAND(1000));
}
