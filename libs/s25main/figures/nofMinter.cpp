// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofMinter.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SoundManager.h"
#include "buildings/nobUsual.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "world/GameWorld.h"

nofMinter::nofMinter(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofWorkman(Job::Minter, pos, player, workplace)
{}

nofMinter::nofMinter(SerializedGameData& sgd, const unsigned obj_id) : nofWorkman(sgd, obj_id) {}

void nofMinter::DrawWorking(DrawPoint drawPt)
{
    constexpr helpers::EnumArray<DrawPoint, Nation> offsets = {{{19, -20}, {19, -11}, {22, -12}, {28, 1}, {16, -12}}};

    unsigned now_id = GAMECLIENT.Interpolate(136, current_ev);

    if(now_id < 91)
    {
        LOADER.GetPlayerImage("rom_bobs", 84 + (now_id) % 8)
          ->DrawFull(drawPt + offsets[workplace->GetNation()], COLOR_WHITE,
                     world->GetPlayer(workplace->GetPlayer()).color);

        // Evtl Sound abspielen
        if(now_id % 8 == 3)
        {
            world->GetSoundMgr().playNOSound(58, *this, now_id);
            was_sounding = true;
        }
    }

    last_id = now_id;
}

helpers::OptionalEnum<GoodType> nofMinter::ProduceWare()
{
    world->GetPlayer(player).ChangeStatisticValue(StatisticType::Gold, 1);
    return GoodType::Coins;
}
