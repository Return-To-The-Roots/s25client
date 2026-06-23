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
#include "random/Random.h"
#include "world/GameWorld.h"
#include "gameTypes/MineNoOutputFallback.h"
#include "gameTypes/MineResourceBehavior.h"
#include "gameTypes/Resource.h"
#include "gameData/GameConsts.h"
#include <algorithm>
#include <vector>

namespace {
constexpr unsigned MAX_PRODUCTION_PERCENT = 100;
constexpr unsigned GRANITE_FALLBACK_25_PERCENT = 25;
constexpr unsigned GRANITE_FALLBACK_50_PERCENT = 50;
constexpr unsigned S4LIKE_MIN_RESOURCE_AMOUNT = 1;

MineNoOutputFallback GetConfiguredNoOutputFallback(const GlobalGameSettings& settings)
{
    const unsigned selection = settings.getSelection(AddonId::MINE_NO_OUTPUT_FALLBACK);
    if(!helpers::isValidEnumValue<MineNoOutputFallback>(selection))
        return MineNoOutputFallback::ProduceNothing;
    return static_cast<MineNoOutputFallback>(selection);
}

unsigned GetGraniteFallbackChance(const MineNoOutputFallback fallback)
{
    switch(fallback)
    {
        case MineNoOutputFallback::ProduceGranite25: return GRANITE_FALLBACK_25_PERCENT;
        case MineNoOutputFallback::ProduceGranite50: return GRANITE_FALLBACK_50_PERCENT;
        case MineNoOutputFallback::ProduceGranite100: return MAX_PRODUCTION_PERCENT;
        default: return 0;
    }
}

helpers::OptionalEnum<GoodType> GetLowerGradeFallbackGood(const BuildingType buildingType)
{
    switch(buildingType)
    {
        case BuildingType::GoldMine: return GoodType::IronOre;
        case BuildingType::IronMine: return GoodType::Coal;
        case BuildingType::CoalMine: return GoodType::Stones;
        default: return boost::none;
    }
}

helpers::OptionalEnum<GoodType> GetNoOutputFallbackGood(const GlobalGameSettings& settings,
                                                        const BuildingType buildingType, const unsigned objId)
{
    const MineNoOutputFallback fallback = GetConfiguredNoOutputFallback(settings);
    const unsigned graniteFallbackChance = GetGraniteFallbackChance(fallback);
    if(graniteFallbackChance > 0)
    {
        if(graniteFallbackChance == MAX_PRODUCTION_PERCENT
           || static_cast<unsigned>(RANDOM.Rand(RANDOM_CONTEXT2(objId), MAX_PRODUCTION_PERCENT))
                < graniteFallbackChance)
            return GoodType::Stones;

        return boost::none;
    }

    if(fallback == MineNoOutputFallback::ProduceLowerGradeResource)
        return GetLowerGradeFallbackGood(buildingType);

    return boost::none;
}

std::vector<MapPoint> GetPointsWithResource(const GameWorld& world, const MapPoint pos, const ResourceType type)
{
    return world.GetMatchingPointsInRadius(
      pos, MINER_RADIUS, [&world, type](const MapPoint pt) { return world.GetNode(pt).resources.has(type); }, true);
}

void ReduceS4LikeResource(GameWorld& world, const std::vector<MapPoint>& resourcePts)
{
    for(const MapPoint pt : resourcePts)
    {
        if(world.GetNode(pt).resources.getAmount() > S4LIKE_MIN_RESOURCE_AMOUNT)
        {
            world.ReduceResource(pt);
            return;
        }
    }
}
} // namespace

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
    const GlobalGameSettings& settings = world->GetGGS();
    const MineResourceBehavior behavior = GetMineResourceBehavior(settings, workplace->GetBuildingType());

    if(behavior == MineResourceBehavior::S4LikeExhaustion)
    {
        const std::vector<MapPoint> resourcePts = GetPointsWithResource(*world, pos, GetRequiredResType());
        const auto productionRoll = static_cast<unsigned>(RANDOM_RAND(MAX_PRODUCTION_PERCENT));
        const bool produceNothingThisCycle = resourcePts.empty()
                                             || productionRoll >= GetS4LikeMineProductionChance(
                                                  GetRemainingMineResources(*world, pos, GetRequiredResType()));
        if(produceNothingThisCycle)
            return GetNoOutputFallbackGood(settings, workplace->GetBuildingType(), GetObjId());

        if(IsMineResourceDepletable(settings, workplace->GetBuildingType()))
            ReduceS4LikeResource(*world, resourcePts);
    }

    switch(workplace->GetBuildingType())
    {
        case BuildingType::GoldMine: return GoodType::Gold;
        case BuildingType::IronMine: return GoodType::IronOre;
        case BuildingType::CoalMine: return GoodType::Coal;
        default: return GoodType::Stones;
    }
}

bool nofMiner::AreWaresAvailable() const
{
    if(!nofWorkman::AreWaresAvailable())
        return false;

    const MineResourceBehavior behavior = GetMineResourceBehavior(world->GetGGS(), workplace->GetBuildingType());
    if(behavior == MineResourceBehavior::WorkEverywhere)
        return true;

    if(FindPointWithResource(GetRequiredResType(), false).isValid())
        return true;

    workplace->OnOutOfResources();
    return false;
}

bool nofMiner::StartWorking()
{
    const GlobalGameSettings& settings = world->GetGGS();
    const MineResourceBehavior behavior = GetMineResourceBehavior(settings, workplace->GetBuildingType());
    if(behavior == MineResourceBehavior::WorkEverywhere)
        return nofWorkman::StartWorking();

    const MapPoint resPt = FindPointWithResource(GetRequiredResType());
    if(!resPt.isValid())
        return false;

    if(behavior != MineResourceBehavior::S4LikeExhaustion
       && IsMineResourceDepletable(settings, workplace->GetBuildingType()))
        world->ReduceResource(resPt);

    return nofWorkman::StartWorking();
}

ResourceType nofMiner::GetRequiredResType() const
{
    return GetMineResourceType(workplace->GetBuildingType());
}
