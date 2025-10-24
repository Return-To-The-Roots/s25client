// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofFarmer.h"

#include "GamePlayer.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "SoundManager.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "world/GameWorld.h"
#include "nodeObjs/noEnvObject.h"
#include "nodeObjs/noGrainfield.h"

nofFarmer::nofFarmer(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofFarmhand(Job::Farmer, pos, player, workplace), harvest(false)
{}

void nofFarmer::Serialize(SerializedGameData& sgd) const
{
    nofFarmhand::Serialize(sgd);

    sgd.PushBool(harvest);
}

nofFarmer::nofFarmer(SerializedGameData& sgd, const unsigned obj_id) : nofFarmhand(sgd, obj_id), harvest(sgd.PopBool())
{}

/// Draws the worker while working
void nofFarmer::DrawWorking(DrawPoint drawPt)
{
    unsigned now_id;

    if(harvest)
    {
        LOADER.GetPlayerImage("rom_bobs", 140 + (now_id = GAMECLIENT.Interpolate(88, current_ev)) % 8)
          ->DrawFull(drawPt, COLOR_WHITE, world->GetPlayer(player).color);

        // Play a sound if applicable
        if(now_id % 8 == 3)
        {
            world->GetSoundMgr().playNOSound(64, *this, now_id / 8);
            was_sounding = true;
        }

    } else
    {
        LOADER.GetPlayerImage("rom_bobs", 132 + GAMECLIENT.Interpolate(88, current_ev) % 8)
          ->DrawFull(drawPt, COLOR_WHITE, world->GetPlayer(player).color);
    }
}

unsigned short nofFarmer::GetCarryID() const
{
    return 71;
}

/// Inform the derived class when it starts working (preparations)
void nofFarmer::WorkStarted()
{
    // If I go to a grain field, I harvest it; otherwise I sow
    harvest = (world->GetNO(pos)->GetType() == NodalObjectType::Grainfield);

    // Inform the grain field so it does not suddenly disappear while we work
    if(harvest)
        world->GetSpecObj<noGrainfield>(pos)->BeginHarvesting();
}

/// Inform the derived class when it is done working
void nofFarmer::WorkFinished()
{
    if(harvest)
    {
        // Destroy the grain field and first fetch the ID of the harvested field, which will then be placed
        // as a regular decorative object
        noBase* nob = world->GetNO(pos);
        // Check if there is still a grain field at this position
        if(nob->GetGOT() != GO_Type::Grainfield)
            return;
        unsigned mapLstId = static_cast<noGrainfield*>(nob)->GetHarvestMapLstID();
        world->DestroyNO(pos);
        world->SetNO(pos, new noEnvObject(pos, mapLstId));

        // Pick up the grain we harvested
        ware = GoodType::Grain;
    } else
    {
        // If the point got bad (e.g. something was build), abort work
        if(GetPointQuality(pos) == PointQuality::NotPossible)
            return;

        // What was here before?
        NodalObjectType noType = world->GetNO(pos)->GetType();

        // Only decorative objects and signs may be removed
        if(noType == NodalObjectType::Environment || noType == NodalObjectType::Nothing)
        {
            world->DestroyNO(pos, false);
            // place a new grain field
            world->SetNO(pos, new noGrainfield(pos));
        }

        // We only sowed (do not pick up anything)
        ware = boost::none;
    }

    // Recalculate the BQ around the point
    world->RecalcBQAroundPoint(pos);
}

/// Returns the quality of this working point or determines if the worker can work here at all
nofFarmhand::PointQuality nofFarmer::GetPointQuality(const MapPoint pt, bool /* isBeforeWork */) const
{
    // Either there is a grain field that we can harvest...
    if(world->GetNO(pt)->GetType() == NodalObjectType::Grainfield)
    {
        if(world->GetSpecObj<noGrainfield>(pt)->IsHarvestable())
            return PointQuality::Class1;
        else
            return PointQuality::NotPossible;
    }
    // ...or a free spot where we can sow a new one
    else
    {
        // Do not build on roads!
        for(const auto dir : helpers::EnumRange<Direction>{})
        {
            if(world->GetPointRoad(pt, dir) != PointRoad::None)
                return PointQuality::NotPossible;
        }

        // Examine the terrain
        if(!world->IsOfTerrain(pt, [](const auto& desc) { return desc.IsVital(); }))
            return PointQuality::NotPossible;

        // Is the spot free?
        NodalObjectType noType = world->GetNO(pt)->GetType();
        if(noType != NodalObjectType::Environment && noType != NodalObjectType::Nothing)
            return PointQuality::NotPossible;

        for(const MapPoint nb : world->GetNeighbours(pt))
        {
            // Do not place directly next to other grain fields and buildings!
            noType = world->GetNO(nb)->GetType();
            if(noType == NodalObjectType::Building || noType == NodalObjectType::Buildingsite)
                return PointQuality::NotPossible;
        }

        return PointQuality::Class2;
    }
}

void nofFarmer::WorkAborted()
{
    nofFarmhand::WorkAborted();
    // Inform the grain field so it can wither again if we abort harvesting
    if(harvest && state == State::Work)
        world->GetSpecObj<noGrainfield>(pos)->EndHarvesting();
}
