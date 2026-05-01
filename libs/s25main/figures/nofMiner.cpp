// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofMiner.h"
#include "GlobalGameSettings.h"
#include "Loader.h"
#include "SoundManager.h"
#include "addons/const_addons.h"
#include "buildings/nobUsual.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "world/GameWorld.h"
#include "gameTypes/Resource.h"
#include "gameData/GameConsts.h"
#include "random/Random.h"

nofMiner::nofMiner(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofWorkman(Job::Miner, pos, player, workplace)
{}

nofMiner::nofMiner(SerializedGameData& sgd, const unsigned obj_id) : nofWorkman(sgd, obj_id) {}

void nofMiner::DrawWorking(DrawPoint drawPt)
{
    constexpr helpers::EnumArray<std::array<DrawPoint, 4>, Nation>
      offsets = // work animation offset per nation and (granite, coal, iron, gold)
      {{
        {{{5, 3}, {5, 3}, {5, 3}, {5, 3}}},     // africans
        {{{4, 1}, {4, 1}, {4, 1}, {4, 1}}},     // japanese
        {{{9, 4}, {9, 4}, {9, 4}, {9, 4}}},     // romans
        {{{10, 3}, {10, 3}, {10, 3}, {10, 3}}}, // vikings
        {{{8, 3}, {8, 3}, {8, 3}, {8, 3}}}      // babylonians
      }};
    const unsigned mineIdx = rttr::enum_cast(workplace->GetBuildingType()) - rttr::enum_cast(BuildingType::GraniteMine);
    RTTR_Assert(mineIdx < offsets.size());

    unsigned now_id = GAMECLIENT.Interpolate(160, current_ev);
    unsigned texture;
    if(workplace->GetNation() == Nation::Romans)
        texture = 92 + now_id % 8;
    else
        texture = 1799 + now_id % 4;
    LOADER.GetPlayerImage("rom_bobs", texture)->DrawFull(drawPt + offsets[workplace->GetNation()][mineIdx]);

    if(now_id % 8 == 3)
    {
        world->GetSoundMgr().playNOSound(59, *this, now_id);
        was_sounding = true;
    }
}

unsigned short nofMiner::GetCarryID() const
{
    switch(workplace->GetBuildingType())
    {
        case BuildingType::GoldMine: return 65;
        case BuildingType::IronMine: return 66;
        case BuildingType::CoalMine: return 67;
        default: return 68;
    }
}

helpers::OptionalEnum<GoodType> nofMiner::ProduceWare()
{
    switch(workplace->GetBuildingType())
    {
        case BuildingType::GoldMine: return GoodType::Gold;
        case BuildingType::IronMine: return GoodType::IronOre;
        case BuildingType::CoalMine: return GoodType::Coal;
        default: return GoodType::Stones;
    }
}

MapPoint nofMiner::FindPointWithResourceQuiet(ResourceType type) const
{
    const auto pts = world->GetMatchingPointsInRadius<1>(
      pos, MINER_RADIUS, [this, type](const MapPoint pt) { return world->GetNode(pt).resources.has(type); }, true);
    return pts.empty() ? MapPoint::Invalid() : pts.front();
}

bool nofMiner::CanCreateWorkEverywhereGraniteResource() const
{
    return workplace->GetBuildingType() == BuildingType::GraniteMine
           && world->GetGGS().isEnabled(AddonId::GRANITEMINES_WORK_EVERYWHERE)
           && world->GetNode(pos).resources.getType() == ResourceType::Nothing;
}

MapPoint nofMiner::CreateWorkEverywhereGraniteResource()
{
    if(!CanCreateWorkEverywhereGraniteResource())
        return MapPoint::Invalid();

    world->SetResource(pos, Resource(ResourceType::Granite, static_cast<uint8_t>(8 + RANDOM_RAND(8))));
    return pos;
}

bool nofMiner::AreWaresAvailable() const
{
    if(!nofWorkman::AreWaresAvailable())
        return false;

    if(FindPointWithResourceQuiet(GetRequiredResType()).isValid() || CanCreateWorkEverywhereGraniteResource())
        return true;

    workplace->OnOutOfResources();
    return false;
}

bool nofMiner::StartWorking()
{
    const GlobalGameSettings& settings = world->GetGGS();
    MapPoint resPt = FindPointWithResourceQuiet(GetRequiredResType());
    if(!resPt.isValid())
    {
        resPt = CreateWorkEverywhereGraniteResource();
        if(!resPt.isValid())
        {
            workplace->OnOutOfResources();
            return false;
        }
    }

    const bool inexhaustibleRes = settings.isEnabled(AddonId::INEXHAUSTIBLE_MINES)
                                  || (workplace->GetBuildingType() == BuildingType::GraniteMine
                                      && settings.isEnabled(AddonId::INEXHAUSTIBLE_GRANITEMINES));
    if(!inexhaustibleRes)
        world->ReduceResource(resPt);

    return nofWorkman::StartWorking();
}
ResourceType nofMiner::GetRequiredResType() const
{
    switch(workplace->GetBuildingType())
    {
        case BuildingType::GoldMine: return ResourceType::Gold;
        case BuildingType::IronMine: return ResourceType::Iron;
        case BuildingType::CoalMine: return ResourceType::Coal;
        default: return ResourceType::Granite;
    }
}
