// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofWinegrower.h"

#include "GamePlayer.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "SoundManager.h"
#include "WineLoader.h"
#include "buildings/nobUsual.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "world/GameWorld.h"
#include "nodeObjs/noEnvObject.h"
#include "nodeObjs/noGrapefield.h"
#include <stdexcept>

using namespace wineaddon;

nofWinegrower::nofWinegrower(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofFarmhand(Job::Winegrower, pos, player, workplace), harvest(false)
{}

nofWinegrower::nofWinegrower(SerializedGameData& sgd, const unsigned obj_id)
    : nofFarmhand(sgd, obj_id), harvest(sgd.PopBool())
{}

void nofWinegrower::Serialize(SerializedGameData& sgd) const
{
    nofFarmhand::Serialize(sgd);
    sgd.PushBool(harvest);
}

void nofWinegrower::DrawWorking(DrawPoint drawPt)
{
    if(harvest)
    {
        const unsigned now_id = GAMECLIENT.Interpolate(44, current_ev);
        LOADER.GetPlayerImage("wine_bobs", bobIndex[BobTypes::WINEGROWER_PICKING_GRAPES_ANIMATION] + now_id % 4)
          ->DrawFull(drawPt, COLOR_WHITE, world->GetPlayer(player).color);
    } else
    {
        const unsigned now_id = GAMECLIENT.Interpolate(3 * 16 + 8 * 4, current_ev);
        unsigned draw_id;
        if(now_id < 48)
        {
            draw_id = bobIndex[BobTypes::WINEGROWER_DIGGING_ANIMATION] + now_id % 15;

            // Shovel-Sound
            if(now_id % 16 == 5)
            {
                world->GetSoundMgr().playNOSound(76, *this, now_id, 200);
                was_sounding = true;
            }
        } else
            draw_id = bobIndex[BobTypes::WINEGROWER_PLANTING_ANIMATION] + now_id % 4;

        LOADER.GetPlayerImage("wine_bobs", draw_id)->DrawFull(drawPt, COLOR_WHITE, world->GetPlayer(player).color);
    }
}

unsigned short nofWinegrower::GetCarryID() const
{
    throw std::logic_error("Must not be called. Handled by custom DrawWalkingWithWare");
}

void nofWinegrower::WorkStarted()
{
    // Inform grapefield to not disappear during harvesting
    if(harvest)
        world->GetSpecObj<noGrapefield>(pos)->BeginHarvesting();
}

void nofWinegrower::WorkFinished()
{
    if(harvest)
    {
        // Destroy grapefield, and take before the ID of the harvested grapefield, which will be set as normal
        // ornamental object
        noBase* nob = world->GetNO(pos);
        // Check if there is still a grapes field at this position
        if(nob->GetGOT() != GO_Type::Grapefield)
            return;
        const unsigned wine_bobs = static_cast<noGrapefield*>(nob)->GetHarvestID();
        world->DestroyNO(pos);
        world->SetNO(pos, new noEnvObject(pos, wine_bobs, 7));

        // Take harvested grapes into hands
        ware = GoodType::Grapes;
    } else
    {
        // If the point got bad (e.g. something was build), abort work
        if(GetPointQuality(pos) == PointQuality::NotPossible)
            return;

        // What is currently here
        const NodalObjectType noType = world->GetNO(pos)->GetType();

        // Only demolish ornamental objects and signs
        if(noType == NodalObjectType::Environment || noType == NodalObjectType::Nothing)
        {
            world->DestroyNO(pos, false);
            // place new grapefield
            world->SetNO(pos, new noGrapefield(pos));
        }

        // We have only planted (take nothing into hand)
        ware = boost::none;
    }

    // recompute BQ around
    world->RecalcBQAroundPoint(pos);
}

nofFarmhand::PointQuality nofWinegrower::GetPointQuality(const MapPoint pt, const bool isBeforeWork) const
{
    // Either a grapefield exists we can harvest...
    if(world->GetNO(pt)->GetType() == NodalObjectType::Grapefield)
    {
        if(world->GetSpecObj<noGrapefield>(pt)->IsHarvestable())
            return PointQuality::Class1;
        else
            return PointQuality::NotPossible;
    } else // or a free space, to place a new one
    {
        // Try to "plant" a new grapefield
        // Still enough wares when starting new work
        if(isBeforeWork && !workplace->WaresAvailable())
            return PointQuality::NotPossible;

        // Do not build on road
        for(const auto dir : helpers::EnumRange<Direction>{})
        {
            if(world->GetPointRoad(pt, dir) != PointRoad::None)
                return PointQuality::NotPossible;
        }

        // Investigate terrain
        if(!world->IsOfTerrain(pt, [](const auto& desc) { return desc.IsVital(); }))
            return PointQuality::NotPossible;

        // Is space free?
        NodalObjectType noType = world->GetNO(pt)->GetType();
        if(noType != NodalObjectType::Environment && noType != NodalObjectType::Nothing)
            return PointQuality::NotPossible;

        for(const MapPoint nb : world->GetNeighbours(pt))
        {
            // Do not build near to other grapefields and buildings!
            noType = world->GetNO(nb)->GetType();
            if(noType == NodalObjectType::Grapefield || noType == NodalObjectType::Grainfield
               || noType == NodalObjectType::Building || noType == NodalObjectType::Buildingsite)
                return PointQuality::NotPossible;
        }

        return PointQuality::Class2;
    }
}

void nofWinegrower::WorkAborted()
{
    nofFarmhand::WorkAborted();
    // inform the grapefield, so it can wither, when we harvest it
    if(harvest && state == State::Work)
        world->GetSpecObj<noGrapefield>(pos)->AbortHarvesting();
}

void nofWinegrower::WalkingStarted()
{
    // When walking to a grapefield, harvest it or plant a new one
    harvest = (world->GetNO(dest)->GetType() == NodalObjectType::Grapefield);

    if(!harvest)
        workplace->ConsumeWares();
}

void nofWinegrower::DrawWalkingWithWare(DrawPoint drawPt)
{
    DrawWalking(drawPt, "wine_bobs", bobIndex[BobTypes::WINEGROWER_WALKING_WITH_FULL_BASKET]);
}

void nofWinegrower::DrawOtherStates(DrawPoint drawPt)
{
    if(state == State::WalkToWorkpoint)
    {
        if(harvest) // Go to harvest grapes
            DrawWalking(drawPt, "wine_bobs", bobIndex[BobTypes::WINEGROWER_WALKING_WITH_EMPTY_BASKET]);
        else // Draw normal walking
            DrawWalking(drawPt);
    }
}

bool nofWinegrower::AreWaresAvailable() const
{
    // wine grower doesn't need wares for harvesting!
    // -> Wares are considered when calling GetPointQuality!
    return true;
}
