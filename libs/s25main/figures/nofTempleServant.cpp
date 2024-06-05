// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofTempleServant.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SoundManager.h"
#include "WineLoader.h"
#include "buildings/nobUsual.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "random/Random.h"
#include "world/GameWorld.h"

namespace {
int producedGood = 0;
}

using namespace wineaddon;

nofTempleServant::nofTempleServant(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofWorkman(Job::TempleServant, pos, player, workplace)
{}

nofTempleServant::nofTempleServant(SerializedGameData& sgd, const unsigned obj_id) : nofWorkman(sgd, obj_id) {}

void nofTempleServant::DrawWorking(DrawPoint drawPt)
{
    constexpr helpers::EnumArray<DrawPoint, Nation> offsets = {{{26, -23}, {17, -40}, {4, 0}, {22, -25}, {20, -45}}};

    unsigned now_id = GAMECLIENT.Interpolate(82, current_ev);

    if(now_id < 11) // servant pours wine into crucible(play once at start)
    {
        LOADER.GetPlayerImage("wine_bobs", getStartIndexOfBob(BobTypes::TEMPLE_WORK_WINDOW_START_ANIMATION) + now_id)
          ->DrawFull(drawPt + offsets[workplace->GetNation()], COLOR_WHITE,
                     world->GetPlayer(workplace->GetPlayer()).color);
    }
    if(now_id >= 11 && now_id < 77) // wine and food is burned (loop for duration)
    {
        LOADER
          .GetPlayerImage("wine_bobs",
                          getStartIndexOfBob(BobTypes::TEMPLE_WORK_WINDOW_MAIN_ANIMATION) + (now_id + 1) % 6)
          ->DrawFull(drawPt + offsets[workplace->GetNation()], COLOR_WHITE,
                     world->GetPlayer(workplace->GetPlayer()).color);

        // Play sound burning
        if((now_id + 1) % 6 == 3)
        {
            world->GetSoundMgr().playNOSound(93, *this, now_id);
            was_sounding = true;
        }
    }
    if(now_id >= 77 && now_id < 82) // last loop of animation (play once at end)
    {
        LOADER
          .GetPlayerImage("wine_bobs", getStartIndexOfBob(BobTypes::TEMPLE_WORK_WINDOW_END_ANIMATION) + now_id - 77)
          ->DrawFull(drawPt + offsets[workplace->GetNation()], COLOR_WHITE,
                     world->GetPlayer(workplace->GetPlayer()).color);
    }

    last_id = now_id;
}

unsigned short nofTempleServant::GetCarryID() const
{
    throw std::logic_error("Must not be called. Handled by custom DrawWalkingWithWare");
}

/// Draws the figure while returning home / entering the building (often carrying wares)
void nofTempleServant::DrawWalkingWithWare(DrawPoint drawPt)
{
    std::array<BobTypes, 4> resultWare{
      BobTypes::TEMPLESERVANT_CARRYING_GOLD_IN_OUT, BobTypes::TEMPLESERVANT_CARRYING_IRON_IN_OUT,
      BobTypes::TEMPLESERVANT_CARRYING_COAL_IN_OUT, BobTypes::TEMPLESERVANT_CARRYING_STONE_IN_OUT};
    DrawWalking(drawPt, "wine_bobs", getStartIndexOfBob(resultWare[producedGood]) - 8);
}

helpers::OptionalEnum<GoodType> nofTempleServant::ProduceWare()
{
    std::array<GoodType, 4> resultWare{GoodType::Gold, GoodType::IronOre, GoodType::Coal, GoodType::Stones};
    producedGood = RANDOM_RAND(4);
    return resultWare[producedGood];
}
