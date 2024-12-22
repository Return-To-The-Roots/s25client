// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofVintner.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SoundManager.h"
#include "WineLoader.h"
#include "buildings/nobUsual.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "world/GameWorld.h"

using namespace wineaddon;

nofVintner::nofVintner(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofWorkman(Job::Vintner, pos, player, workplace)
{}

nofVintner::nofVintner(SerializedGameData& sgd, const unsigned obj_id) : nofWorkman(sgd, obj_id) {}

void nofVintner::DrawWorking(DrawPoint drawPt)
{
    constexpr helpers::EnumArray<DrawPoint, Nation> offsets = {{{13, -22}, {19, 6}, {-14, 16}, {4, -20}, {-19, 12}}};

    unsigned now_id = GAMECLIENT.Interpolate(136, current_ev);

    if(now_id < 91)
    {
        LOADER.GetPlayerImage("wine_bobs", bobIndex[BobTypes::VINTNER_WORK_WINDOW] + (now_id) % 8)
          ->DrawFull(drawPt + offsets[workplace->GetNation()], COLOR_WHITE,
                     world->GetPlayer(workplace->GetPlayer()).color);

        // Play rowing boat sound
        if(now_id % 8 == 3)
        {
            world->GetSoundMgr().playNOSound(81, *this, now_id);
            was_sounding = true;
        }
    }

    last_id = now_id;
}

unsigned short nofVintner::GetCarryID() const
{
    throw std::logic_error("Must not be called. Handled by custom DrawWalkingWithWare");
}

void nofVintner::DrawWalkingWithWare(DrawPoint drawPt)
{
    DrawWalking(drawPt, "wine_bobs", bobIndex[BobTypes::VINTNER_CARRYING_WINE_IN_OUT]);
}

helpers::OptionalEnum<GoodType> nofVintner::ProduceWare()
{
    return GoodType::Wine;
}
