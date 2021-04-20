// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofIronfounder.h"

#include "GamePlayer.h"
#include "Loader.h"
#include "SoundManager.h"
#include "buildings/nobUsual.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "world/GameWorld.h"

nofIronfounder::nofIronfounder(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofWorkman(Job::IronFounder, pos, player, workplace)
{}

nofIronfounder::nofIronfounder(SerializedGameData& sgd, const unsigned obj_id) : nofWorkman(sgd, obj_id) {}

void nofIronfounder::DrawWorking(DrawPoint drawPt)
{
    const helpers::EnumArray<DrawPoint, Nation> offsets = {{{-22, 12}, {-23, 3}, {-19, 8}, {-18, 4}, {-33, 7}}};

    unsigned now_id = GAMECLIENT.Interpolate(272, current_ev);

    if(now_id < 182)
    {
        LOADER.GetPlayerImage("rom_bobs", 100 + (now_id % 8))
          ->DrawFull(drawPt + offsets[workplace->GetNation()], COLOR_WHITE,
                     world->GetPlayer(workplace->GetPlayer()).color);

        // Evtl Sound abspielen
        if(now_id % 16 == 3)
        {
            world->GetSoundMgr().playNOSound(58, *this, now_id / 16);
            was_sounding = true;
        }
    }
}

helpers::OptionalEnum<GoodType> nofIronfounder::ProduceWare()
{
    return GoodType::Iron;
}
