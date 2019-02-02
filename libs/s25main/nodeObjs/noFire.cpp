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

#include "rttrDefines.h" // IWYU pragma: keep
#include "noFire.h"

#include "EventManager.h"
#include "GlobalGameSettings.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "SoundManager.h"
#include "addons/const_addons.h"
#include "drivers/VideoDriverWrapper.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "world/GameWorldGame.h"

noFire::noFire(const MapPoint pos, bool isBig)
    : noCoordBase(NOP_FIRE, pos), isBig(isBig), was_sounding(false), last_sound(0), next_interval(0)
{
    // Bestimmte Zeit lang brennen
    const unsigned FIREDURATION[] = {3700, 2775, 1850, 925, 370, 5550, 7400};
    dead_event = GetEvMgr().AddEvent(this, FIREDURATION[gwg->GetGGS().getSelection(AddonId::BURN_DURATION)]);
}
noFire::~noFire() {}

void noFire::Destroy_noFire()
{
    // Just in case
    GetEvMgr().RemoveEvent(dead_event);

    // nix mehr hier
    gwg->SetNO(pos, nullptr);
    // Bauplätze drumrum neu berechnen
    gwg->RecalcBQAroundPoint(pos);

    // Evtl Sounds vernichten
    SOUNDMANAGER.WorkingFinished(this);

    Destroy_noCoordBase();
}

void noFire::Serialize_noFire(SerializedGameData& sgd) const
{
    Serialize_noCoordBase(sgd);

    sgd.PushBool(isBig);
    sgd.PushEvent(dead_event);
}

noFire::noFire(SerializedGameData& sgd, const unsigned obj_id)
    : noCoordBase(sgd, obj_id), isBig(sgd.PopBool()), dead_event(sgd.PopEvent()), was_sounding(false), last_sound(0), next_interval(0)
{}

void noFire::Draw(DrawPoint drawPt)
{
    //// Die ersten 2 Drittel (zeitlich) brennen, das 3. Drittel Schutt daliegen lassen
    const unsigned FIREANIMATIONDURATION[] = {1000, 750, 500, 250, 100, 1500, 2000};
    unsigned id = GAMECLIENT.Interpolate(FIREANIMATIONDURATION[gwg->GetGGS().getSelection(AddonId::BURN_DURATION)], dead_event);

    if(id < FIREANIMATIONDURATION[gwg->GetGGS().getSelection(AddonId::BURN_DURATION)] * 2 / 3)
    {
        // Loderndes Feuer
        LOADER.GetMapImageN(2500 + (isBig ? 8 : 0) + id % 8)->DrawFull(drawPt);
        LOADER.GetMapImageN(2530 + (isBig ? 8 : 0) + id % 8)->DrawFull(drawPt, 0xC0101010);

        // Feuersound abspielen in zufälligen Intervallen
        if(VIDEODRIVER.GetTickCount() - last_sound > next_interval)
        {
            SOUNDMANAGER.PlayNOSound(96, this, id);
            was_sounding = true;

            last_sound = VIDEODRIVER.GetTickCount();
            next_interval = 500 + rand() % 1400;
        }
    } else
    {
        // Schutt
        LOADER.GetMapImageN(2524 + (isBig ? 1 : 0))->DrawFull(drawPt);
    }
}

/// Benachrichtigen, wenn neuer gf erreicht wurde
void noFire::HandleEvent(const unsigned /*id*/)
{
    // Todesevent --> uns vernichten
    dead_event = nullptr;
    GetEvMgr().AddToKillList(this);
}
