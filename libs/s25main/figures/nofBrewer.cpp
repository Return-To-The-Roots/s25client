// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofBrewer.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SoundManager.h"
#include "buildings/nobUsual.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "world/GameWorld.h"

nofBrewer::nofBrewer(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofWorkman(Job::Brewer, pos, player, workplace)
{}

nofBrewer::nofBrewer(SerializedGameData& sgd, const unsigned obj_id) : nofWorkman(sgd, obj_id) {}

void nofBrewer::DrawWorking(DrawPoint drawPt)
{
    static constexpr helpers::EnumArray<DrawPoint, Nation> offsets = {
      {{10, 17}, {10, 17}, {10, 17}, {10, 17}, {10, 17}}};

    unsigned now_id = GAMECLIENT.Interpolate(128, current_ev);

    if(now_id < 16)
        LOADER.GetPlayerImage("rom_bobs", now_id)
          ->DrawFull(drawPt + offsets[workplace->GetNation()], COLOR_WHITE,
                     world->GetPlayer(workplace->GetPlayer()).color);

    if(now_id == 5)
    {
        world->GetSoundMgr().playNOSound(51, *this, 0);
        was_sounding = true;
    }
    last_id = now_id;
}

helpers::OptionalEnum<GoodType> nofBrewer::ProduceWare()
{
    return GoodType::Beer;
}
