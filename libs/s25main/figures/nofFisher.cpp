// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofFisher.h"

#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "SoundManager.h"
#include "addons/const_addons.h"
#include "enum_cast.hpp"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "pathfinding/PathConditionHuman.h"
#include "random/Random.h"
#include "world/GameWorld.h"

nofFisher::nofFisher(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofFarmhand(Job::Fisher, pos, player, workplace), fishing_dir(Direction::West), successful(false)
{}

void nofFisher::Serialize(SerializedGameData& sgd) const
{
    nofFarmhand::Serialize(sgd);

    sgd.PushEnum<uint8_t>(fishing_dir);
    sgd.PushBool(successful);
}

nofFisher::nofFisher(SerializedGameData& sgd, const unsigned obj_id)
    : nofFarmhand(sgd, obj_id), fishing_dir(sgd.Pop<Direction>()), successful(sgd.PopBool())
{}

/// Malt den Arbeiter beim Arbeiten
void nofFisher::DrawWorking(DrawPoint drawPt)
{
    unsigned short id = GAMECLIENT.Interpolate(232, current_ev);
    unsigned short draw_id;

    if(id < 16)
    {
        // Angel reinlassen
        const unsigned iFishingDir = rttr::enum_cast(fishing_dir);
        if(iFishingDir < 3)
            draw_id = 1566 + 8 * iFishingDir + (id / 2);
        else
            draw_id = 108 + 8 * (iFishingDir - 3) + (id / 2);

        if(id / 2 == 1)
        {
            world->GetSoundMgr().playNOSound(62, *this, 0);
            was_sounding = true;
        }
    } else if(id < 216)
    {
        // Angel im Wasser hängen lassen und warten
        draw_id = 1590 + 8 * rttr::enum_cast(fishing_dir + 3) + (id % 8);
    } else
    {
        // Angel wieder rausholn
        if(successful)
            // Mit Fisch an der Angel
            draw_id = 1638 + 8 * rttr::enum_cast(fishing_dir + 3) + (id - 216) / 2;
        else
        {
            // Ohne Fisch (wie das Reinlassen, bloß umgekehrt)
            const unsigned iFishingDir = rttr::enum_cast(fishing_dir);
            if(iFishingDir < 3)
                draw_id = 1566 + 8 * iFishingDir + 7 - (id - 216) / 2;
            else
                draw_id = 108 + 8 * (iFishingDir - 3) + 7 - (id - 216) / 2;
        }

        if((id - 216) / 2 == 1)
        {
            world->GetSoundMgr().playNOSound(62, *this, 1);
            was_sounding = true;
        }
    }

    LOADER.GetPlayerImage("rom_bobs", draw_id)->DrawFull(drawPt, COLOR_WHITE, world->GetPlayer(player).color);
    DrawShadow(drawPt, 0, Direction(fishing_dir));
}

unsigned short nofFisher::GetCarryID() const
{
    return 70;
}

void nofFisher::WorkStarted()
{
    // Find a random neighbour point with fish
    unsigned fishAmount = 0;
    for(Direction dir : helpers::enumRange(RANDOM_ENUM(Direction)))
    {
        const Resource neighbourRes = world->GetNode(world->GetNeighbour(pos, dir)).resources;
        if(neighbourRes.has(ResourceType::Fish))
        {
            fishing_dir = dir;
            fishAmount = neighbourRes.getAmount();
            break;
        }
    }

    // Probability to catch a fish is proportional to the amount of fish
    const int probability = (fishAmount == 0) ? 0 : 40 + fishAmount * 10;
    successful = RANDOM_RAND(100) < probability;
    // On success remove the fish now to make it unavailable to others
    if(successful && !world->GetGGS().isEnabled(AddonId::INEXHAUSTIBLE_FISH))
        world->ReduceResource(world->GetNeighbour(pos, fishing_dir));
}

void nofFisher::WorkFinished()
{
    if(successful)
        ware = GoodType::Fish;
    else
        ware = boost::none;
}

/// Returns the quality of this working point or determines if the worker can work here at all
nofFarmhand::PointQuality nofFisher::GetPointQuality(const MapPoint pt) const
{
    // Der Punkt muss passierbar sein für Figuren
    if(!PathConditionHuman(*world).IsNodeOk(pt))
        return PointQuality::NotPossible;

    // irgendwo drumherum muss es Fisch geben
    for(const MapPoint nb : world->GetNeighbours(pt))
    {
        if(world->GetNode(nb).resources.has(ResourceType::Fish))
            return PointQuality::Class1;
    }

    return PointQuality::NotPossible;
}
