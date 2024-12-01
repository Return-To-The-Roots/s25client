// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofSkinner.h"
#include "GamePlayer.h"
#include "LeatherLoader.h"
#include "Loader.h"
#include "SoundManager.h"
#include "buildings/nobUsual.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "world/GameWorld.h"

using namespace leatheraddon;

nofSkinner::nofSkinner(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofWorkman(Job::Skinner, pos, player, workplace)
{}

nofSkinner::nofSkinner(SerializedGameData& sgd, const unsigned obj_id) : nofWorkman(sgd, obj_id) {}

void nofSkinner::DrawWorking(DrawPoint /*drawPt*/) {}

unsigned short nofSkinner::GetCarryID() const
{
    throw std::logic_error("Must not be called. Handled by custom DrawWalkingWithWare");
}

void nofSkinner::DrawWalkingWithWare(DrawPoint drawPt)
{
    DrawWalking(drawPt, "leather_bobs", bobIndex[BobTypes::SKINNER_CARRYING_SKINS]);
}

helpers::OptionalEnum<GoodType> nofSkinner::ProduceWare()
{
    return GoodType::Skins;
}
