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

void nofIronfounder::drawWorking(DrawPoint drawPt)
{
    const helpers::EnumArray<DrawPoint, Nation> offsets = {{{-22, 12}, {-23, 3}, {-19, 8}, {-18, 4}, {-33, 7}}};

    unsigned nowId = GAMECLIENT.interpolate(272, current_ev);

    if(nowId < 182)
    {
        LOADER.getPlayerImage("rom_bobs", 100 + (nowId % 8))
          ->drawFull(drawPt + offsets[workplace->getNation()], COLOR_WHITE,
                     world->getPlayer(workplace->getPlayer()).color);

        // Evtl Sound abspielen
        if(nowId % 16 == 3)
        {
            world->getSoundMgr().playNOSound(58, *this, nowId / 16);
            was_sounding = true;
        }
    }
}

helpers::OptionalEnum<GoodType> nofIronfounder::produceWare()
{
    return GoodType::Iron;
}
