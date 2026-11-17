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

void nofBrewer::drawWorking(DrawPoint drawPt)
{
    static constexpr helpers::EnumArray<DrawPoint, Nation> offsets = {
      {{10, 17}, {10, 17}, {10, 17}, {10, 17}, {10, 17}}};

    unsigned nowId = GAMECLIENT.interpolate(128, current_ev);

    if(nowId < 16)
        LOADER.getPlayerImage("rom_bobs", nowId)
          ->drawFull(drawPt + offsets[workplace->getNation()], COLOR_WHITE,
                     world->getPlayer(workplace->getPlayer()).color);

    if(nowId == 5)
    {
        world->getSoundMgr().playNOSound(51, *this, 0);
        was_sounding = true;
    }
    last_id = nowId;
}

helpers::OptionalEnum<GoodType> nofBrewer::produceWare()
{
    return GoodType::Beer;
}
