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
#include "gameTypes/MineResourceBehavior.h"
#include "gameTypes/Resource.h"
#include "gameData/GameConsts.h"
#include <algorithm>
#include <vector>

namespace {
constexpr unsigned MAX_PRODUCTION_PERCENT = 100;
constexpr unsigned S4LIKE_PRODUCTION_PERCENT_PER_RESOURCE = 5;
constexpr unsigned S4LIKE_MIN_RESOURCE_AMOUNT = 1;
constexpr uint8_t WORK_EVERYWHERE_RESOURCE_MIN_AMOUNT = 8;
constexpr unsigned WORK_EVERYWHERE_RESOURCE_AMOUNT_VARIANTS = 8;

AddonId GetResourceBehaviorAddonId(const BuildingType buildingType)
{
    switch(buildingType)
    {
        case BuildingType::GoldMine: return AddonId::GOLDMINE_RESOURCE_BEHAVIOR;
        case BuildingType::IronMine: return AddonId::IRONMINE_RESOURCE_BEHAVIOR;
        case BuildingType::CoalMine: return AddonId::COALMINE_RESOURCE_BEHAVIOR;
        default: return AddonId::GRANITEMINE_RESOURCE_BEHAVIOR;
    }
}

MineResourceBehavior GetConfiguredResourceBehavior(const GlobalGameSettings& settings, const BuildingType buildingType)
{
    switch(static_cast<MineResourceBehavior>(settings.getSelection(GetResourceBehaviorAddonId(buildingType))))
    {
        case MineResourceBehavior::S4LikeExhaustion: return MineResourceBehavior::S4LikeExhaustion;
        case MineResourceBehavior::Inexhaustible: return MineResourceBehavior::Inexhaustible;
        case MineResourceBehavior::WorkEverywhere: return MineResourceBehavior::WorkEverywhere;
        default: return MineResourceBehavior::Default;
    }
}

MineResourceBehavior GetEffectiveResourceBehavior(const GlobalGameSettings& settings, const BuildingType buildingType,
                                                  const MineResourceBehavior configuredBehavior)
{
    if(configuredBehavior != MineResourceBehavior::Default)
        return configuredBehavior;

    if(buildingType == BuildingType::GraniteMine && settings.isEnabled(AddonId::GRANITEMINES_WORK_EVERYWHERE))
        return MineResourceBehavior::WorkEverywhere;

    if(settings.isEnabled(AddonId::INEXHAUSTIBLE_MINES))
        return MineResourceBehavior::Inexhaustible;

    return MineResourceBehavior::Default;
}

bool ShouldReduceResources(const GlobalGameSettings& settings, const BuildingType buildingType,
                           const MineResourceBehavior configuredBehavior, const MineResourceBehavior effectiveBehavior)
{
    if(effectiveBehavior == MineResourceBehavior::Inexhaustible)
        return false;

    if(configuredBehavior == MineResourceBehavior::Default && settings.isEnabled(AddonId::INEXHAUSTIBLE_MINES))
        return false;

    if(configuredBehavior == MineResourceBehavior::Default && buildingType == BuildingType::GraniteMine
       && settings.isEnabled(AddonId::INEXHAUSTIBLE_GRANITEMINES))
        return false;

    return true;
}

unsigned GetS4LikeProductionChance(const GameWorld& world, const std::vector<MapPoint>& resourcePts)
{
    unsigned resourceAmount = 0;
    for(const MapPoint pt : resourcePts)
        resourceAmount += world.GetNode(pt).resources.getAmount();

    return std::min(MAX_PRODUCTION_PERCENT, resourceAmount * S4LIKE_PRODUCTION_PERCENT_PER_RESOURCE);
}

std::vector<MapPoint> GetPointsWithResource(const GameWorld& world, const MapPoint pos, const ResourceType type)
{
    return world.GetMatchingPointsInRadius<1>(
      pos, MINER_RADIUS, [&world, type](const MapPoint pt) { return world.GetNode(pt).resources.has(type); }, true);
}

bool CanCreateWorkEverywhereResource(const GameWorld& world, const MapPoint pos, const MineResourceBehavior behavior)
{
    return behavior == MineResourceBehavior::WorkEverywhere
           && world.GetNode(pos).resources.getType() == ResourceType::Nothing;
}

MapPoint CreateWorkEverywhereResource(GameWorld& world, const MapPoint pos, const ResourceType type,
                                      const MineResourceBehavior behavior, const unsigned objId)
{
    if(!CanCreateWorkEverywhereResource(world, pos, behavior))
        return MapPoint::Invalid();

    const auto amount =
      static_cast<uint8_t>(WORK_EVERYWHERE_RESOURCE_MIN_AMOUNT
                           + RANDOM.Rand(RANDOM_CONTEXT2(objId), WORK_EVERYWHERE_RESOURCE_AMOUNT_VARIANTS));
    world.SetResource(pos, Resource(type, amount));
    return pos;
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
    const MineResourceBehavior configuredBehavior =
      GetConfiguredResourceBehavior(settings, workplace->GetBuildingType());
    const MineResourceBehavior effectiveBehavior =
      GetEffectiveResourceBehavior(settings, workplace->GetBuildingType(), configuredBehavior);

    if(effectiveBehavior == MineResourceBehavior::S4LikeExhaustion)
    {
        const std::vector<MapPoint> resourcePts = GetPointsWithResource(*world, pos, GetRequiredResType());
        const auto productionRoll = static_cast<unsigned>(RANDOM_RAND(MAX_PRODUCTION_PERCENT));
        const bool produceNothingThisCycle =
          resourcePts.empty() || productionRoll >= GetS4LikeProductionChance(*world, resourcePts);
        if(produceNothingThisCycle)
            return boost::none;

        if(ShouldReduceResources(settings, workplace->GetBuildingType(), configuredBehavior, effectiveBehavior))
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

MapPoint nofMiner::FindPointWithResourceQuiet(ResourceType type) const
{
    const auto pts = GetPointsWithResource(*world, pos, type);
    return pts.empty() ? MapPoint::Invalid() : pts.front();
}

bool nofMiner::AreWaresAvailable() const
{
    if(!nofWorkman::AreWaresAvailable())
        return false;

    const MineResourceBehavior configuredBehavior =
      GetConfiguredResourceBehavior(world->GetGGS(), workplace->GetBuildingType());
    const MineResourceBehavior effectiveBehavior =
      GetEffectiveResourceBehavior(world->GetGGS(), workplace->GetBuildingType(), configuredBehavior);

    if(FindPointWithResourceQuiet(GetRequiredResType()).isValid()
       || CanCreateWorkEverywhereResource(*world, pos, effectiveBehavior))
        return true;

    workplace->OnOutOfResources();
    return false;
}

bool nofMiner::StartWorking()
{
    const GlobalGameSettings& settings = world->GetGGS();
    const MineResourceBehavior configuredBehavior =
      GetConfiguredResourceBehavior(settings, workplace->GetBuildingType());
    const MineResourceBehavior effectiveBehavior =
      GetEffectiveResourceBehavior(settings, workplace->GetBuildingType(), configuredBehavior);
    MapPoint resPt = FindPointWithResourceQuiet(GetRequiredResType());
    if(!resPt.isValid())
    {
        resPt = CreateWorkEverywhereResource(*world, pos, GetRequiredResType(), effectiveBehavior, GetObjId());
        if(!resPt.isValid())
        {
            workplace->OnOutOfResources();
            return false;
        }
    }

    if(effectiveBehavior != MineResourceBehavior::S4LikeExhaustion
       && ShouldReduceResources(settings, workplace->GetBuildingType(), configuredBehavior, effectiveBehavior))
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
