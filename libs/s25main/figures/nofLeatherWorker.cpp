// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofLeatherWorker.h"
#include "GamePlayer.h"
#include "LeatherLoader.h"
#include "Loader.h"
#include "SoundManager.h"
#include "buildings/nobUsual.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "world/GameWorld.h"

using namespace leatheraddon;

nofLeatherWorker::nofLeatherWorker(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofWorkman(Job::LeatherWorker, pos, player, workplace)
{}

nofLeatherWorker::nofLeatherWorker(SerializedGameData& sgd, const unsigned obj_id) : nofWorkman(sgd, obj_id) {}

void nofLeatherWorker::drawWorking(DrawPoint drawPt)
{
    constexpr helpers::EnumArray<DrawPoint, Nation> offsets = {{{11, -41}, {21, -20}, {21, -17}, {24, -13}, {22, -14}}};

    const unsigned nowId = GAMECLIENT.interpolate(230, current_ev);

    LOADER.getPlayerImage("leather_bobs", bobIndex[BobType::LeatherworksWorkWindowAnimation] + (nowId % 23))
      ->drawFull(drawPt + offsets[workplace->getNation()], COLOR_WHITE, world->getPlayer(workplace->getPlayer()).color);

    // hammer sound
    if(nowId % 23 == 3 || nowId % 23 == 7)
    {
        world->getSoundMgr().playNOSound(72, *this, nowId, 100);
        was_sounding = true;
    } else if(nowId % 23 == 9) // saw sound 1
    {
        world->getSoundMgr().playNOSound(54, *this, nowId);
        was_sounding = true;
    } else if(nowId % 23 == 17) // saw sound 2
    {
        world->getSoundMgr().playNOSound(55, *this, nowId);
        was_sounding = true;
    }

    last_id = nowId;
}

#ifdef _MSC_VER
#    pragma warning(push)
#    pragma warning(disable : 4646) // function declared with [[noreturn]] has non-void return type
#endif

unsigned short nofLeatherWorker::getCarryID() const
{
    throw std::logic_error("Must not be called. Handled by custom DrawWalkingWithWare");
}

#ifdef _MSC_VER
#    pragma warning(pop)
#endif

void nofLeatherWorker::drawWalkingWithWare(DrawPoint drawPt)
{
    drawWalking(drawPt, "leather_bobs", bobIndex[BobType::LeatherworkerCarryingArmorInOut]);
}

helpers::OptionalEnum<GoodType> nofLeatherWorker::produceWare()
{
    return GoodType::Armor;
}
