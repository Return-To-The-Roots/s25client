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
#include "gameData/JobConsts.h"
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
        default: return {};
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

        return {};
    }

    if(fallback == MineNoOutputFallback::ProduceLowerGradeResource)
        return GetLowerGradeFallbackGood(buildingType);

    return {};
}

std::vector<MapPoint> GetPointsWithResource(const GameWorld& world, const MapPoint pos, const ResourceType type)
{
    return world.GetMatchingPointsInRadius(
      pos, MINER_RADIUS, [&world, type](const MapPoint pt) { return world.getNode(pt).resources.has(type); }, true);
}

void ReduceS4LikeResource(GameWorld& world, const std::vector<MapPoint>& resourcePts)
{
    for(const MapPoint pt : resourcePts)
    {
        if(world.getNode(pt).resources.getAmount() > S4LIKE_MIN_RESOURCE_AMOUNT)
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
    const unsigned mineIdx = rttr::enum_cast(workplace->getBuildingType()) - rttr::enum_cast(BuildingType::GraniteMine);
    RTTR_Assert(mineIdx < offsets.size());

    unsigned now_id = GAMECLIENT.Interpolate(160, current_ev);
    unsigned texture;
    if(workplace->getNation() == Nation::Romans)
        texture = 92 + now_id % 8;
    else
        texture = 1799 + now_id % 4;
    LOADER.getPlayerImage("rom_bobs", texture)->DrawFull(drawPt + offsets[workplace->getNation()][mineIdx]);

    if(now_id % 8 == 3)
    {
        world->getSoundMgr().playNOSound(59, *this, now_id);
        was_sounding = true;
    }
}

unsigned short nofMiner::getCarryID() const
{
    switch(workplace->getBuildingType())
    {
        case BuildingType::GoldMine: return 65;
        case BuildingType::IronMine: return 66;
        case BuildingType::CoalMine: return 67;
        default: return 68;
    }
}

helpers::OptionalEnum<GoodType> nofMiner::ProduceWare()
{
    const GlobalGameSettings& settings = world->getGGS();
    const MineResourceBehavior behavior = getMineResourceBehavior(settings, workplace->getBuildingType());

    if(behavior == MineResourceBehavior::S4LikeExhaustion)
    {
        const std::vector<MapPoint> resourcePts = GetPointsWithResource(*world, pos, getRequiredResType());
        const auto productionRoll = static_cast<unsigned>(RANDOM_RAND(MAX_PRODUCTION_PERCENT));
        const bool produceNothingThisCycle = resourcePts.empty()
                                             || productionRoll >= getS4LikeMineProductionChance(
                                                  getRemainingMineResources(*world, pos, getRequiredResType()));
        if(produceNothingThisCycle)
            return GetNoOutputFallbackGood(settings, workplace->getBuildingType(), getObjId());

        // S4-like exhaustion always depletes, but only down to the minimum amount
        ReduceS4LikeResource(*world, resourcePts);
    }

    return getMineOutput(workplace->getBuildingType());
}

bool nofMiner::AreWaresAvailable() const
{
    if(!nofWorkman::AreWaresAvailable())
        return false;

    const MineResourceBehavior behavior = getMineResourceBehavior(world->getGGS(), workplace->getBuildingType());
    if(behavior == MineResourceBehavior::WorkEverywhere)
        return true;

    const bool hasResources = FindPointWithResource(getRequiredResType()).isValid();
    if(!hasResources)
        workplace->OnOutOfResources();
    return hasResources;
}

bool nofMiner::StartWorking()
{
    const GlobalGameSettings& settings = world->getGGS();
    const MineResourceBehavior behavior = getMineResourceBehavior(settings, workplace->getBuildingType());
    if(behavior == MineResourceBehavior::WorkEverywhere)
        return nofWorkman::StartWorking();

    const MapPoint resPt = FindPointWithResource(getRequiredResType());
    if(!resPt.isValid())
    {
        workplace->OnOutOfResources();
        return false;
    }

    if(behavior != MineResourceBehavior::S4LikeExhaustion
       && isMineResourceDepletable(settings, workplace->getBuildingType()))
        world->ReduceResource(resPt);

    return nofWorkman::StartWorking();
}

ResourceType nofMiner::getRequiredResType() const
{
    return getMineResourceType(workplace->getBuildingType());
}
