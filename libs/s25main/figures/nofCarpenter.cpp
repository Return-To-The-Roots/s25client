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

void nofCarpenter::drawWorking(DrawPoint drawPt)
{
    static constexpr helpers::EnumArray<DrawPoint, Nation> offsets = {{{30, 3}, {38, 3}, {30, 8}, {17, -2}, {38, 3}}};

    unsigned nowId;

    LOADER.getPlayerImage("rom_bobs", 32 + ((nowId = GAMECLIENT.interpolate(136, current_ev)) % 8))
      ->drawFull(drawPt + offsets[workplace->getNation()], COLOR_WHITE, world->getPlayer(workplace->getPlayer()).color);

    // Evtl Sound abspielen
    if(nowId % 8 == 3 || nowId % 8 == 7)
    {
        world->getSoundMgr().playNOSound(54 + ((nowId) % 8) / 4, *this, nowId / 4);
        was_sounding = true;
    }

    last_id = nowId;
}

helpers::OptionalEnum<GoodType> nofCarpenter::produceWare()
{
    return GoodType::Boards;
}
