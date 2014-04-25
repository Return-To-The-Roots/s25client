// $Id: noGrainfield.cpp 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "main.h"
#include "noGrainfield.h"

#include "Loader.h"
#include "GameClient.h"
#include "Random.h"
#include "GameWorld.h"
#include "SerializedGameData.h"

#include "glSmartBitmap.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/// Länge des Wachs-Wartens
const unsigned GROWING_WAITING_LENGTH = 1100;
/// Länge des Wachsens
const unsigned GROWING_LENGTH = 16;

noGrainfield::noGrainfield(const unsigned short x, const unsigned short y) : noCoordBase(NOP_GRAINFIELD, x, y),
    type(RANDOM.Rand(__FILE__, __LINE__, obj_id, 2)), state(STATE_GROWING_WAITING), size(0), event(em->AddEvent(this, GROWING_WAITING_LENGTH))
{
}

noGrainfield::~noGrainfield()
{

}

void noGrainfield::Destroy_noGrainfield()
{
    em->RemoveEvent(event);

    // Bauplätze drumrum neu berechnen
    gwg->RecalcBQAroundPoint(x, y);

    Destroy_noCoordBase();
}

void noGrainfield::Serialize_noGrainfield(SerializedGameData* sgd) const
{
    Serialize_noCoordBase(sgd);

    sgd->PushUnsignedChar(type);
    sgd->PushUnsignedChar(static_cast<unsigned char>(state));
    sgd->PushUnsignedChar(size);
    sgd->PushObject(event, true);
}

noGrainfield::noGrainfield(SerializedGameData* sgd, const unsigned obj_id) : noCoordBase(sgd, obj_id),
    type(sgd->PopUnsignedChar()),
    state(State(sgd->PopUnsignedChar())),
    size(sgd->PopUnsignedChar()),
    event(sgd->PopObject<EventManager::Event>(GOT_EVENT))
{
}

void noGrainfield::Draw( int x, int y)
{
    switch(state)
    {
        case STATE_GROWING_WAITING:
        case STATE_NORMAL:
        {
            Loader::grainfield_cache[type][size].draw(x, y);
        } break;
        case STATE_GROWING:
        {
            unsigned alpha = GAMECLIENT.Interpolate(0xFF, event);

            // altes Feld ausblenden
            Loader::grainfield_cache[type][size].draw(x, y, SetAlpha(COLOR_WHITE, 0xFF - alpha));

            // neues Feld einblenden
            Loader::grainfield_cache[type][size + 1].draw(x, y, SetAlpha(COLOR_WHITE, alpha));
        } break;
        case STATE_WITHERING:
        {
            unsigned alpha = GAMECLIENT.Interpolate(0xFF, event);

            // Feld ausblenden
            Loader::grainfield_cache[type][size].draw(x, y, SetAlpha(COLOR_WHITE, 0xFF - alpha));
        } break;
    }

}

void noGrainfield::HandleEvent(const unsigned int id)
{
    switch(state)
    {
        case STATE_GROWING_WAITING:
        {
            // Feld hat gewartet, also wächst es jetzt
            event = em->AddEvent(this, GROWING_LENGTH);
            state = STATE_GROWING;
        } break;
        case STATE_GROWING:
        {
            // Wenn er ausgewachsen ist, dann nicht, ansonsten nochmal ein "Warteevent" anmelden, damit er noch weiter wächst
            if(++size != 3)
            {
                event = em->AddEvent(this, GROWING_WAITING_LENGTH);
                // Erstmal wieder bis zum nächsten Wachsstumsschub warten
                state = STATE_GROWING_WAITING;
            }
            else
            {
                // bin nun ausgewachsen
                state = STATE_NORMAL;
                // nach langer Zeit verdorren
                event = em->AddEvent(this, 3000 + RANDOM.Rand(__FILE__, __LINE__, obj_id, 1000));
            }

        } break;
        case STATE_NORMAL:
        {
            // Jetzt lebt es schon zu lange --> hokus pokus verschwindibus!
            state = STATE_WITHERING;
            event = em->AddEvent(this, 20);
        } break;
        case STATE_WITHERING:
        {
            // Selbst zerstören
            event = 0;
            gwg->SetNO(0, x, y);
            em->AddToKillList(this);
        } break;
    }
}


void noGrainfield::BeginHarvesting()
{
    // Event killen, damit wir nicht plötzlich verschwinden, wenn er uns aberntet
    em->RemoveEvent(event);
    event = 0;
    state = STATE_NORMAL;
}

void noGrainfield::EndHarvesting()
{
    // nach langer Zeit verdorren (von neuem)
    event = em->AddEvent(this, 3000 + RANDOM.Rand(__FILE__, __LINE__, obj_id, 1000));
}

