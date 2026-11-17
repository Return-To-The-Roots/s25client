// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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

void nofMinter::drawWorking(DrawPoint drawPt)
{
    constexpr helpers::EnumArray<DrawPoint, Nation> offsets = {{{19, -20}, {19, -11}, {22, -12}, {28, 1}, {16, -12}}};

    unsigned nowId = GAMECLIENT.interpolate(136, current_ev);

    if(nowId < 91)
    {
        LOADER.getPlayerImage("rom_bobs", 84 + (nowId) % 8)
          ->drawFull(drawPt + offsets[workplace->getNation()], COLOR_WHITE,
                     world->getPlayer(workplace->getPlayer()).color);

        // Evtl Sound abspielen
        if(nowId % 8 == 3)
        {
            world->getSoundMgr().playNOSound(58, *this, nowId);
            was_sounding = true;
        }
    }

    last_id = nowId;
}

helpers::OptionalEnum<GoodType> nofMinter::produceWare()
{
    world->getPlayer(player).changeStatisticValue(StatisticType::Gold, 1);
    return GoodType::Coins;
}
