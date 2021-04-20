// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofCarpenter.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SoundManager.h"
#include "buildings/nobUsual.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "world/GameWorld.h"

nofCarpenter::nofCarpenter(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofWorkman(Job::Carpenter, pos, player, workplace)
{}

nofCarpenter::nofCarpenter(SerializedGameData& sgd, const unsigned obj_id) : nofWorkman(sgd, obj_id) {}

void nofCarpenter::DrawWorking(DrawPoint drawPt)
{
    static constexpr helpers::EnumArray<DrawPoint, Nation> offsets = {{{30, 3}, {38, 3}, {30, 8}, {17, -2}, {38, 3}}};

    unsigned now_id;

    LOADER.GetPlayerImage("rom_bobs", 32 + ((now_id = GAMECLIENT.Interpolate(136, current_ev)) % 8))
      ->DrawFull(drawPt + offsets[workplace->GetNation()], COLOR_WHITE, world->GetPlayer(workplace->GetPlayer()).color);

    // Evtl Sound abspielen
    if(now_id % 8 == 3 || now_id % 8 == 7)
    {
        world->GetSoundMgr().playNOSound(54 + ((now_id) % 8) / 4, *this, now_id / 4);
        was_sounding = true;
    }

    last_id = now_id;
}

helpers::OptionalEnum<GoodType> nofCarpenter::ProduceWare()
{
    return GoodType::Boards;
}
