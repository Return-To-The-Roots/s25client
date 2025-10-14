// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofTanner.h"
#include "GamePlayer.h"
#include "LeatherLoader.h"
#include "Loader.h"
#include "SoundManager.h"
#include "buildings/nobUsual.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "world/GameWorld.h"

nofTanner::nofTanner(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofWorkman(Job::Tanner, pos, player, workplace)
{}

nofTanner::nofTanner(SerializedGameData& sgd, const unsigned obj_id) : nofWorkman(sgd, obj_id) {}

void nofTanner::DrawWorking(DrawPoint drawPt)
{
    constexpr helpers::EnumArray<DrawPoint, Nation> offsets = {{{28, -14}, {28, -8}, {-14, -22}, {-5, -25}, {17, -35}}};

    unsigned now_id = GAMECLIENT.Interpolate(136, current_ev);

    LOADER
      .GetPlayerImage("leather_bobs",
                      leatheraddon::bobIndex[leatheraddon::BobTypes::TANNERY_WORK_WINDOW_ANIMATION] + (now_id) % 8)
      ->DrawFull(drawPt + offsets[workplace->GetNation()], COLOR_WHITE, world->GetPlayer(workplace->GetPlayer()).color);

    last_id = now_id;
}

unsigned short nofTanner::GetCarryID() const
{
    throw std::logic_error("Must not be called. Handled by custom DrawWalkingWithWare");
}

void nofTanner::DrawWalkingWithWare(DrawPoint drawPt)
{
    DrawWalking(drawPt, "leather_bobs", leatheraddon::bobIndex[leatheraddon::BobTypes::TANNER_CARRYING_LEATHER_IN_OUT]);
}

helpers::OptionalEnum<GoodType> nofTanner::ProduceWare()
{
    return GoodType::Leather;
}
