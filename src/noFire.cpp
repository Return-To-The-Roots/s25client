// $Id: noFire.cpp 9510 2014-11-29 10:51:36Z marcus $
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
#include "noFire.h"

#include "EventManager.h"
#include "Loader.h"
#include "macros.h"
#include "GameClient.h"
#include "GameWorld.h"
#include "VideoDriverWrapper.h"
#include "SoundManager.h"
#include "SerializedGameData.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

noFire::noFire(const unsigned short x, const unsigned short y, const unsigned char size)
    : noCoordBase(NOP_FIRE, x, y), size(size), was_sounding(false), last_sound(0), next_interval(0)
{
    // Bestimmte Zeit lang brennen
	const unsigned FIREDURATION[] = {3700, 2775, 1850, 925, 370, 5550, 7400};
    dead_event = em->AddEvent(this, FIREDURATION[GAMECLIENT.GetGGS().getSelection(ADDON_BURN_DURATION)]);
}
noFire::~noFire()
{
}

void noFire::Destroy_noFire()
{
    // nix mehr hier
    gwg->SetNO(0, x, y);
    // Bauplätze drumrum neu berechnen
    gwg->RecalcBQAroundPoint(x, y);

    // Evtl Sounds vernichten
    SoundManager::inst().WorkingFinished(this);

    Destroy_noCoordBase();
}

void noFire::Serialize_noFire(SerializedGameData* sgd) const
{
    Serialize_noCoordBase(sgd);

    sgd->PushUnsignedChar(size);
    sgd->PushObject(dead_event, true);
}

noFire::noFire(SerializedGameData* sgd, const unsigned obj_id) : noCoordBase(sgd, obj_id),
    size(sgd->PopUnsignedChar()),
    dead_event(sgd->PopObject<EventManager::Event>(GOT_EVENT)),
    was_sounding(false),
    last_sound(0),
    next_interval(0)
{
}

void noFire::Draw(int x, int y)
{
    //// Die ersten 2 Drittel (zeitlich) brennen, das 3. Drittel Schutt daliegen lassen
	const unsigned FIREANIMATIONDURATION[] = {1000, 750, 500, 250, 100, 1500, 2000};
    unsigned id = GAMECLIENT.Interpolate(FIREANIMATIONDURATION[GAMECLIENT.GetGGS().getSelection(ADDON_BURN_DURATION)], dead_event);

    if(id < FIREANIMATIONDURATION[GAMECLIENT.GetGGS().getSelection(ADDON_BURN_DURATION)]*2/3)
    {
        // Loderndes Feuer
        LOADER.GetMapImageN(2500 + size * 8 + id % 8)->Draw(x, y, 0, 0, 0, 0, 0, 0);
        LOADER.GetMapImageN(2530 + size * 8 + id % 8)->Draw(x, y, 0, 0, 0, 0, 0, 0, 0xC0101010);

        // Feuersound abspielen in zufälligen Intervallen
        if(VideoDriverWrapper::inst().GetTickCount() - last_sound > next_interval)
        {
            SoundManager::inst().PlayNOSound(96, this, id);
            was_sounding = true;

            last_sound = VideoDriverWrapper::inst().GetTickCount();
            next_interval = 500 + rand() % 1400;
        }
    }
    else
    {
        // Schutt
        LOADER.GetMapImageN(2524 + size)->Draw(x, y, 0, 0, 0, 0, 0, 0);
    }
}

/// Benachrichtigen, wenn neuer gf erreicht wurde
void noFire::HandleEvent(const unsigned int id)
{
    // Todesevent --> uns vernichten
    dead_event = 0;
    em->AddToKillList(this);
}
