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

void nofLeatherWorker::DrawWorking(DrawPoint drawPt)
{
    constexpr helpers::EnumArray<DrawPoint, Nation> offsets = {{{11, -41}, {21, -20}, {21, -17}, {24, -13}, {22, -14}}};

    const unsigned now_id = GAMECLIENT.Interpolate(230, current_ev);

    LOADER.GetPlayerImage("leather_bobs", bobIndex[BobType::LeatherworksWorkWindowAnimation] + (now_id % 23))
      ->DrawFull(drawPt + offsets[workplace->GetNation()], COLOR_WHITE, world->GetPlayer(workplace->GetPlayer()).color);

    // hammer sound
    if(now_id % 23 == 3 || now_id % 23 == 7)
    {
        world->GetSoundMgr().playNOSound(72, *this, now_id, 100);
        was_sounding = true;
    } else if(now_id % 23 == 9) // saw sound 1
    {
        world->GetSoundMgr().playNOSound(54, *this, now_id);
        was_sounding = true;
    } else if(now_id % 23 == 17) // saw sound 2
    {
        world->GetSoundMgr().playNOSound(55, *this, now_id);
        was_sounding = true;
    }

    last_id = now_id;
}

#ifdef _MSC_VER
#    pragma warning(push)
#    pragma warning(disable : 4646) // function declared with [[noreturn]] has non-void return type
#endif

unsigned short nofLeatherWorker::GetCarryID() const
{
    throw std::logic_error("Must not be called. Handled by custom DrawWalkingWithWare");
}

#ifdef _MSC_VER
#    pragma warning(pop)
#endif

void nofLeatherWorker::DrawWalkingWithWare(DrawPoint drawPt)
{
    DrawWalking(drawPt, "leather_bobs", bobIndex[BobType::LeatherworkerCarryingArmorInOut]);
}

helpers::OptionalEnum<GoodType> nofLeatherWorker::ProduceWare()
{
    return GoodType::Armor;
}
