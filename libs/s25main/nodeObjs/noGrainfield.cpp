// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "noGrainfield.h"

#include "EventManager.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "network/GameClient.h"
#include "ogl/glSmartBitmap.h"
#include "random/Random.h"
#include "world/GameWorldGame.h"
#include "s25util/colors.h"

/// Länge des Wachs-Wartens
const unsigned GROWING_WAITING_LENGTH = 1100;
/// Länge des Wachsens
const unsigned GROWING_LENGTH = 16;

noGrainfield::noGrainfield(const MapPoint pos)
    : noCoordBase(NodalObjectType::Grainfield, pos), type(RANDOM.Rand(__FILE__, __LINE__, GetObjId(), 2)),
      state(State::GrowingWaiting), size(0)
{
    event = GetEvMgr().AddEvent(this, GROWING_WAITING_LENGTH);
}

noGrainfield::~noGrainfield() = default;

void noGrainfield::Destroy_noGrainfield()
{
    GetEvMgr().RemoveEvent(event);

    // Bauplätze drumrum neu berechnen
    gwg->RecalcBQAroundPoint(pos);

    Destroy_noCoordBase();
}

void noGrainfield::Serialize_noGrainfield(SerializedGameData& sgd) const
{
    Serialize_noCoordBase(sgd);

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

            // altes Feld ausblenden
            LOADER.grainfield_cache[type][size].draw(drawPt, SetAlpha(COLOR_WHITE, 0xFF - alpha));

            // neues Feld einblenden
            LOADER.grainfield_cache[type][size + 1].draw(drawPt, SetAlpha(COLOR_WHITE, alpha));
        }
        break;
        case State::Withering:
        {
            unsigned alpha = GAMECLIENT.Interpolate(0xFF, event);

            // Feld ausblenden
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
                event = GetEvMgr().AddEvent(this, 3000 + RANDOM.Rand(__FILE__, __LINE__, GetObjId(), 1000));
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
            gwg->SetNO(pos, nullptr);
            GetEvMgr().AddToKillList(this);
        }
        break;
    }
}

void noGrainfield::BeginHarvesting()
{
    // Event killen, damit wir nicht plötzlich verschwinden, wenn er uns aberntet
    GetEvMgr().RemoveEvent(event);
    event = nullptr;
    state = State::Normal;
}

void noGrainfield::EndHarvesting()
{
    // nach langer Zeit verdorren (von neuem)
    event = GetEvMgr().AddEvent(this, 3000 + RANDOM.Rand(__FILE__, __LINE__, GetObjId(), 1000));
}
