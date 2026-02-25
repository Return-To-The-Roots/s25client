// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofStonemason.h"

#include "GameInterface.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SoundManager.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "world/GameWorld.h"
#include "nodeObjs/noGranite.h"

nofStonemason::nofStonemason(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofFarmhand(Job::Stonemason, pos, player, workplace)
{}

nofStonemason::nofStonemason(SerializedGameData& sgd, const unsigned obj_id) : nofFarmhand(sgd, obj_id) {}

/// Draws the worker while working
void nofStonemason::DrawWorking(DrawPoint drawPt)
{
    unsigned now_id;

    // Hammering stone
    LOADER.GetPlayerImage("rom_bobs", 40 + (now_id = GAMECLIENT.Interpolate(64, current_ev)) % 8)
      ->DrawFull(drawPt, COLOR_WHITE, world->GetPlayer(player).color);

    if(now_id % 8 == 5)
    {
        world->GetSoundMgr().playNOSound(56, *this, now_id);
        was_sounding = true;
    }
}

unsigned short nofStonemason::GetCarryID() const
{
    return 63;
}

/// Notify derived class when work starts (preparations)
void nofStonemason::WorkStarted() {}

/// Notify derived class when work is finished
void nofStonemason::WorkFinished()
{
    // Cut off one chunk of granite (if it is already minimal, remove it from the map)
    if(world->GetSpecObj<noGranite>(pos)->IsSmall())
    {
        // Remove the granite chunk
        world->DestroyNO(pos);

        // Notify minimap (granite chunk disappeared)
        if(world->GetGameInterface())
            world->GetGameInterface()->GI_UpdateMinimap(pos);

        // Recalculate nearby BQ, as it may have changed now
        world->RecalcBQAroundPoint(pos);
    } else
        // Otherwise decrease size by 1
        world->GetSpecObj<noGranite>(pos)->Hew();

    // Pick up a stone
    ware = GoodType::Stones;
}

/// Returns the quality of this working point or determines if the worker can work here at all
nofFarmhand::PointQuality nofStonemason::GetPointQuality(const MapPoint pt, bool /* isBeforeWork */) const
{
    // This position must contain granite
    return ((world->GetNO(pt)->GetType() == NodalObjectType::Granite) ? PointQuality::Class1 :
                                                                        PointQuality::NotPossible);
}
