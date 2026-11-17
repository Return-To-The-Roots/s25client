// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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

void nofButcher::drawWorking(DrawPoint drawPt)
{
    static constexpr helpers::EnumArray<DrawPoint, Nation> offsets = {{{38, 2}, {-3, 5}, {21, -1}, {26, -5}, {-7, 2}}};

    unsigned nowId;

    LOADER.getPlayerImage("rom_bobs", 160 + (nowId = GAMECLIENT.interpolate(136, current_ev)) % 6)
      ->drawFull(drawPt + offsets[workplace->getNation()], COLOR_WHITE, world->getPlayer(workplace->getPlayer()).color);

    if(nowId % 6 == 5)
    {
        world->getSoundMgr().playNOSound(66, *this, nowId / 6);
        was_sounding = true;
    }

    last_id = nowId;
}

helpers::OptionalEnum<GoodType> nofButcher::produceWare()
{
    return GoodType::Meat;
}
