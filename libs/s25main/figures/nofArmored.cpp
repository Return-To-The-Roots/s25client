// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofArmored.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "LeatherLoader.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "addons/const_addons.h"
#include "ogl/glFont.h"
#include "world/GameWorld.h"
#include "gameTypes/JobTypes.h"

nofArmored::nofArmored(Job job, MapPoint pos, unsigned char player, noRoadNode* goal, bool armor)
    : noFigure(job, pos, player, goal)
{
    this->armor = armor;
}

nofArmored::nofArmored(Job job, MapPoint pos, unsigned char player, bool armor) : noFigure(job, pos, player)
{
    this->armor = armor;
}

void nofArmored::Serialize(SerializedGameData& sgd) const
{
    noFigure::Serialize(sgd);
}

nofArmored::nofArmored(SerializedGameData& sgd, const unsigned obj_id) : noFigure(sgd, obj_id) {}

void nofArmored::DrawArmorWalking(DrawPoint drawPt)
{
    if(HasArmor())
        DrawArmor(InterpolateWalkDrawPos(drawPt));
}

void nofArmored::DrawArmor(DrawPoint drawPt)
{
    if(world->GetGGS().isEnabled(AddonId::MILITARY_HITPOINTS))
    {
        SmallFont->Draw(drawPt + DrawPoint(7, -20), "+", FontStyle::CENTER, COLOR_RED);
        SmallFont->Draw(drawPt + DrawPoint(10, -20), "1", FontStyle::CENTER, COLOR_RED);
    }

    LOADER.GetImageN("leather_bobs", leatheraddon::bobIndex[leatheraddon::BobTypes::DONKEY_BOAT_CARRYING_ARMOR_WARE])
      ->DrawFull(drawPt + DrawPoint(0, -22));
}

void nofArmored::DrawArmorNotWalking(DrawPoint drawPt)
{
    if(HasArmor())
        DrawArmor(drawPt);
}

bool nofArmored::HasArmor() const
{
    return armor;
}

void nofArmored::SetArmor(bool armor)
{
    this->armor = armor;
}
