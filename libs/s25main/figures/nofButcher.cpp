// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofButcher.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SoundManager.h"
#include "buildings/nobUsual.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "world/GameWorld.h"

nofButcher::nofButcher(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofWorkman(Job::Butcher, pos, player, workplace)
{}

nofButcher::nofButcher(SerializedGameData& sgd, const unsigned obj_id) : nofWorkman(sgd, obj_id) {}

void nofButcher::DrawWorking(DrawPoint drawPt)
{
    static constexpr helpers::EnumArray<DrawPoint, Nation> offsets = {{{38, 2}, {-3, 5}, {21, -1}, {26, -5}, {-7, 2}}};

    unsigned now_id;

    LOADER.GetPlayerImage("rom_bobs", 160 + (now_id = GAMECLIENT.Interpolate(136, current_ev)) % 6)
      ->DrawFull(drawPt + offsets[workplace->GetNation()], COLOR_WHITE, world->GetPlayer(workplace->GetPlayer()).color);

    if(now_id % 6 == 5)
    {
        world->GetSoundMgr().playNOSound(66, *this, now_id / 6);
        was_sounding = true;
    }

    last_id = now_id;
}

helpers::OptionalEnum<GoodType> nofButcher::ProduceWare()
{
    return GoodType::Meat;
}
