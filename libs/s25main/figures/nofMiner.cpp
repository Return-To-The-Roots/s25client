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
#include <random/Random.h>

nofMiner::nofMiner(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofWorkman(Job::Miner, pos, player, workplace), isAlteredWorkcycle(false)
{}

// TODO: need to look into saving/loading to correctly load the work cycle
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

bool nofMiner::AreWaresAvailable() const
{
    if(!nofWorkman::AreWaresAvailable())
        return false;

    if(GetMiningBehavior() == MiningBehavior::AlwaysAvailable)
        return true;

    return FindPointWithResource(GetRequiredResType()).isValid();
}

MiningBehavior nofMiner::GetMiningBehavior() const
{
    const GlobalGameSettings& settings = world->GetGGS();

    switch(workplace->GetBuildingType())
    {
        case BuildingType::GoldMine: return (MiningBehavior)settings.getSelection(AddonId::MINING_OVERHAUL_GOLD);
        case BuildingType::IronMine: return (MiningBehavior)settings.getSelection(AddonId::MINING_OVERHAUL_IRON);
        case BuildingType::CoalMine: return (MiningBehavior)settings.getSelection(AddonId::MINING_OVERHAUL_COAL);
        case BuildingType::GraniteMine: return (MiningBehavior)settings.getSelection(AddonId::MINING_OVERHAUL_GRANITE);
        default: return MiningBehavior::Normal;
    }
}

bool nofMiner::StartWorking()
{
    isAlteredWorkcycle = false;

    MiningBehavior addonSettings = GetMiningBehavior();

    switch (addonSettings)
    {
        case MiningBehavior::S4Like:
        {
            int sumResAmount = 0;
            MapPoint useResPt;

            std::vector<MapPoint> resPts = FindAllPointsWithResource(GetRequiredResType());

            // iterate over all points
            for each(MapPoint curPt in resPts)
            {
                // calculate the absolute amount of resource beneath
                uint8_t resAmount = world->GetNode(curPt).resources.getAmount();
                sumResAmount += resAmount;

                // if there is one with more than 1 quantity, keep it (for reducing)
                if(resAmount > 1 && !useResPt.isValid())
                    useResPt = curPt;
            }

            // no resources left (mine was built on invalid spot)
            if(sumResAmount == 0)
                return false;

            // 19 = amount of nodes a mine can reach
            // 7  = maximum resource amount a node possibly has
            if(RANDOM.Rand(RANDOM_CONTEXT(), 19 * 7) > sumResAmount)
            {
                // failed, use food and start working - but produce nothing
                isAlteredWorkcycle = true;
                //ProduceWare = boost::none;
            } else
            {
                // if success, use 1 quantity if any
                if(!useResPt.isValid())
                    world->ReduceResource(useResPt);
            }
        }
        break;
        case MiningBehavior::Inexhaustible:
        {
            MapPoint resPt = FindPointWithResource(GetRequiredResType());
            if(!resPt.isValid())
                return false;
        }
        break;
        case MiningBehavior::AlwaysAvailable: break;
        case MiningBehavior::Normal:
        default:
        {
            MapPoint resPt = FindPointWithResource(GetRequiredResType());
            if(!resPt.isValid())
                return false;

            world->ReduceResource(resPt);
        }
        break;
    }

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
