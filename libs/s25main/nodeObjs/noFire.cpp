// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
#include "world/GameWorld.h"

constexpr std::array<unsigned, 7> FIREDURATION = {3700, 2775, 1850, 925, 370, 5550, 7400};
constexpr std::array<unsigned, 7> FIREANIMATIONDURATION = {1000, 750, 500, 250, 100, 1500, 2000};

noFire::noFire(const MapPoint pos, bool isBig)
    : noCoordBase(NodalObjectType::Fire, pos), isBig(isBig), was_sounding(false), last_sound(0), next_interval(0)
{
    // Burn for some time
    dead_event = GetEvMgr().AddEvent(this, FIREDURATION[world->GetGGS().getSelection(AddonId::BURN_DURATION)]);
}
noFire::~noFire() = default;

void noFire::Destroy()
{
    // Remove from map if still present
    if(world->GetNode(pos).obj == this)
        world->SetNO(pos, nullptr);

    world->RecalcBQAroundPoint(pos);
    world->GetSoundMgr().stopSounds(*this);

    GetEvMgr().RemoveEvent(dead_event);
    noCoordBase::Destroy();
}

void noFire::Serialize(SerializedGameData& sgd) const
{
    noCoordBase::Serialize(sgd);

    sgd.PushBool(isBig);
    sgd.PushEvent(dead_event);
}

noFire::noFire(SerializedGameData& sgd, const unsigned obj_id)
    : noCoordBase(sgd, obj_id), isBig(sgd.PopBool()), dead_event(sgd.PopEvent()), was_sounding(false), last_sound(0),
      next_interval(0)
{}

void noFire::Draw(DrawPoint drawPt)
{
    unsigned id =
      GAMECLIENT.Interpolate(FIREANIMATIONDURATION[world->GetGGS().getSelection(AddonId::BURN_DURATION)], dead_event);

    // Burn for the first 2/3 of the time, leave debris for the last 1/3
    if(id < FIREANIMATIONDURATION[world->GetGGS().getSelection(AddonId::BURN_DURATION)] * 2 / 3)
    {
        // Fire animation
        LOADER.GetMapTexture(2500 + (isBig ? 8 : 0) + id % 8)->DrawFull(drawPt);
        LOADER.GetMapTexture(2530 + (isBig ? 8 : 0) + id % 8)->DrawFull(drawPt, 0xC0101010);

        // Fire sound play in random intervals
        if(VIDEODRIVER.GetTickCount() - last_sound > next_interval)
        {
            world->GetSoundMgr().playNOSound(96, *this, id);
            was_sounding = true;

            last_sound = VIDEODRIVER.GetTickCount();
            next_interval = 500 + rand() % 1400;
        }
    } else
    {
        // Debris
        LOADER.GetMapTexture(2524 + (isBig ? 1 : 0))->DrawFull(drawPt);
    }
}

void noFire::HandleEvent(const unsigned /*id*/)
{
    // End event: Remove from map and destroy
    RTTR_Assert(world->GetNode(pos).obj == this);
    world->SetNO(pos, nullptr);
    dead_event = nullptr;
    GetEvMgr().AddToKillList(this);
}
