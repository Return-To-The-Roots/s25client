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

void nofTanner::drawWorking(DrawPoint drawPt)
{
    constexpr helpers::EnumArray<DrawPoint, Nation> offsets = {{{28, -14}, {28, -8}, {-14, -22}, {-5, -25}, {17, -35}}};

    const unsigned nowId = GAMECLIENT.interpolate(136, current_ev);

    LOADER
      .getPlayerImage("leather_bobs",
                      leatheraddon::bobIndex[leatheraddon::BobType::TanneryWorkWindowAnimation] + (nowId) % 8)
      ->drawFull(drawPt + offsets[workplace->getNation()], COLOR_WHITE, world->getPlayer(workplace->getPlayer()).color);

    last_id = nowId;
}

#ifdef _MSC_VER
#    pragma warning(push)
#    pragma warning(disable : 4646) // function declared with [[noreturn]] has non-void return type
#endif

unsigned short nofTanner::getCarryID() const
{
    throw std::logic_error("Must not be called. Handled by custom DrawWalkingWithWare");
}

#ifdef _MSC_VER
#    pragma warning(pop)
#endif

void nofTanner::drawWalkingWithWare(DrawPoint drawPt)
{
    drawWalking(drawPt, "leather_bobs", leatheraddon::bobIndex[leatheraddon::BobType::TannerCarryingLeatherInOut]);
}

helpers::OptionalEnum<GoodType> nofTanner::produceWare()
{
    return GoodType::Leather;
}
