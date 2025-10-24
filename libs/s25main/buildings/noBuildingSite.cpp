// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "buildings/noBuildingSite.h"
#include "FOWObjects.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "Ware.h"
#include "figures/nofBuilder.h"
#include "figures/nofPlaner.h"
#include "helpers/containerUtils.h"
#include "helpers/toString.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glSmartBitmap.h"
#include "world/GameWorld.h"
#include "gameData/BuildingConsts.h"
#include "gameData/MilitaryConsts.h"
#include "s25util/colors.h"
#include <stdexcept>

noBuildingSite::noBuildingSite(const BuildingType type, const MapPoint pos, const unsigned char player)
    : noBaseBuilding(NodalObjectType::Buildingsite, type, pos, player), state(BuildingSiteState::Building),
      planer(nullptr), builder(nullptr), boards(0), stones(0), used_boards(0), used_stones(0), build_progress(0)
{
    // Check whether the site has to be leveled first (only for medium and large buildings)
    if(GetSize() == BuildingQuality::House || GetSize() == BuildingQuality::Castle
       || GetSize() == BuildingQuality::Harbor)
    {
        // Altitude at the construction point
        int altitude = world->GetNode(pos).altitude;

        for(const auto dir : helpers::EnumRange<Direction>{})
        {
            // Direction SouthEast is the flag and must not be leveled
            if(dir != Direction::SouthEast)
            {
                // Is there any difference?
                if(altitude - world->GetNeighbourNode(pos, dir).altitude != 0)
                    state = BuildingSiteState::Planing;
            }
        }
    }

    // Request a planer or a builder
    world->GetPlayer(player).AddJobWanted((state == BuildingSiteState::Planing) ? Job::Planer : Job::Builder, this);

    // Order construction materials
    OrderConstructionMaterial();

    // Register the site so the economy knows about it
    world->GetPlayer(player).AddBuildingSite(this);
}

/// Constructor for harbor construction sites placed from a ship
noBuildingSite::noBuildingSite(const MapPoint pos, const unsigned char player)
    : noBaseBuilding(NodalObjectType::Buildingsite, BuildingType::HarborBuilding, pos, player),
      state(BuildingSiteState::Building), planer(nullptr), boards(BUILDING_COSTS[BuildingType::HarborBuilding].boards),
      stones(BUILDING_COSTS[BuildingType::HarborBuilding].stones), used_boards(0), used_stones(0), build_progress(0)
{
    GamePlayer& owner = world->GetPlayer(player);
    // Register the site so the economy knows about it
    owner.AddBuildingSite(this);
    // Spawn the builder on the map as well
    builder = &world->AddFigure(pos, std::make_unique<nofBuilder>(pos, player, this));

    // Deduct the construction materials from the owner's inventory
    owner.DecreaseInventoryWare(GoodType::Boards, boards);
    owner.DecreaseInventoryWare(GoodType::Stones, stones);
}

noBuildingSite::~noBuildingSite() = default;

void noBuildingSite::Destroy()
{
    // Notify the builder or planer
    if(builder)
    {
        builder->LostWork();
        builder = nullptr;
    } else if(planer)
    {
        planer->LostWork();
        planer = nullptr;
    } else
        world->GetPlayer(player).JobNotWanted(this);

    RTTR_Assert(!builder);
    RTTR_Assert(!planer);

    // Inform any ordered wares
    for(Ware* ordered_board : ordered_boards)
        WareNotNeeded(ordered_board);
    ordered_boards.clear();
    for(Ware* ordered_stone : ordered_stones)
        WareNotNeeded(ordered_stone);
    ordered_stones.clear();

    // Clear the map tile
    world->SetNO(pos, nullptr);

    // Remove the site from the player's list (keep expedition harbor status in mind)
    bool expeditionharbor = IsHarborBuildingSiteFromSea();
    world->GetPlayer(player).RemoveBuildingSite(this);

    noBaseBuilding::Destroy();

    // Is this a harbor construction site?
    if(expeditionharbor)
    {
        world->RemoveHarborBuildingSiteFromSea(this);
        // Recalculate territory after destruction because roads and similar objects may be removed already
        world->RecalcTerritory(*this, TerritoryChangeReason::Destroyed);
    }
    world->RecalcBQAroundPointBig(pos);
}

void noBuildingSite::Serialize(SerializedGameData& sgd) const
{
    noBaseBuilding::Serialize(sgd);

    sgd.PushEnum<uint8_t>(state);
    sgd.PushObject(planer, true);
    sgd.PushObject(builder, true);
    sgd.PushUnsignedChar(boards);
    sgd.PushUnsignedChar(stones);
    sgd.PushUnsignedChar(used_boards);
    sgd.PushUnsignedChar(used_stones);
    sgd.PushUnsignedChar(build_progress);
    sgd.PushObjectContainer(ordered_boards, true);
    sgd.PushObjectContainer(ordered_stones, true);
}

noBuildingSite::noBuildingSite(SerializedGameData& sgd, const unsigned obj_id)
    : noBaseBuilding(sgd, obj_id), state(sgd.Pop<BuildingSiteState>()),
      planer(sgd.PopObject<nofPlaner>(GO_Type::NofPlaner)), builder(sgd.PopObject<nofBuilder>(GO_Type::NofBuilder)),
      boards(sgd.PopUnsignedChar()), stones(sgd.PopUnsignedChar()), used_boards(sgd.PopUnsignedChar()),
      used_stones(sgd.PopUnsignedChar()), build_progress(sgd.PopUnsignedChar())
{
    sgd.PopObjectContainer(ordered_boards, GO_Type::Ware);
    sgd.PopObjectContainer(ordered_stones, GO_Type::Ware);
}

void noBuildingSite::OrderConstructionMaterial()
{
    // Do not order goods while the site is still in the leveling phase
    if(state == BuildingSiteState::Planing)
        return;

    // Boards
    GamePlayer& owner = world->GetPlayer(player);
    for(int i = used_boards + boards + ordered_boards.size(); i < BUILDING_COSTS[bldType_].boards; ++i)
    {
        Ware* w = owner.OrderWare(GoodType::Boards, this);
        if(!w)
            break;
        RTTR_Assert(helpers::contains(ordered_boards, w));
    }
    // Stones
    for(int i = used_stones + stones + ordered_stones.size(); i < BUILDING_COSTS[bldType_].stones; ++i)
    {
        Ware* w = owner.OrderWare(GoodType::Stones, this);
        if(!w)
            break;
        RTTR_Assert(helpers::contains(ordered_stones, w));
    }
}

unsigned noBuildingSite::GetMilitaryRadius() const
{
    /// Note: This actually only applies to harbor buildings made from expeditions. We rely on the calling functions to
    /// only take those into account
    return bldType_ == BuildingType::HarborBuilding ? HARBOR_RADIUS : 0;
}

void noBuildingSite::Draw(DrawPoint drawPt)
{
    if(state == BuildingSiteState::Planing)
    {
        // Draw the construction sign with its shadow
        LOADER.GetNationImage(world->GetPlayer(player).nation, 450)->DrawFull(drawPt);
        LOADER.GetNationImage(world->GetPlayer(player).nation, 451)->DrawFull(drawPt, COLOR_SHADOW);
    } else
    {
        // Draw the foundation stone and shadow
        LOADER.GetNationImage(world->GetPlayer(player).nation, 455)->DrawFull(drawPt);
        LOADER.GetNationImage(world->GetPlayer(player).nation, 456)->DrawFull(drawPt, COLOR_SHADOW);

        // Wares currently present on the construction site

        // Boards
        DrawPoint doorPos = drawPt + DrawPoint(GetDoorPointX(), GetDoorPointY());
        for(unsigned char i = 0; i < boards; ++i)
            LOADER.GetWareStackTex(GoodType::Boards)->DrawFull(doorPos - DrawPoint(5, 10 + i * 4));
        // Stones
        for(unsigned char i = 0; i < stones; ++i)
            LOADER.GetWareStackTex(GoodType::Stones)->DrawFull(doorPos + DrawPoint(8, -12 - i * 4));

        // Draw the partially constructed building

        // Shell of the building

        // Compute the current progress
        unsigned progressRaw, progressBld;
        unsigned maxProgressRaw, maxProgressBld;

        if(BUILDING_COSTS[bldType_].stones)
        {
            // Building contains both stones and planks
            maxProgressRaw = BUILDING_COSTS[bldType_].boards * 8;
            maxProgressBld = BUILDING_COSTS[bldType_].stones * 8;
        } else
        {
            // Building contains only planks, so ensure a 50:50 split
            maxProgressBld = maxProgressRaw = BUILDING_COSTS[bldType_].boards * 4;
        }
        progressRaw = std::min<unsigned>(build_progress, maxProgressRaw);
        progressBld = ((build_progress > maxProgressRaw) ? (build_progress - maxProgressRaw) : 0);

        LOADER.building_cache[nation][bldType_].skeleton.drawPercent(drawPt, progressRaw * 100 / maxProgressRaw);
        LOADER.building_cache[nation][bldType_].building.drawPercent(drawPt, progressBld * 100 / maxProgressBld);
    }
}

/// Create a fog-of-war memory object for the construction site
std::unique_ptr<FOWObject> noBuildingSite::CreateFOWObject() const
{
    return std::make_unique<fowBuildingSite>(state == BuildingSiteState::Planing, bldType_, nation, build_progress);
}

void noBuildingSite::GotWorker(Job /*job*/, noFigure& worker)
{
    // A planer or builder has arrived
    if(state == BuildingSiteState::Planing)
    {
        RTTR_Assert(worker.GetGOT() == GO_Type::NofPlaner);
        planer = static_cast<nofPlaner*>(&worker);
    } else
    {
        RTTR_Assert(worker.GetGOT() == GO_Type::NofBuilder);
        builder = static_cast<nofBuilder*>(&worker);
    }
}

void noBuildingSite::Abrogate()
{
    planer = nullptr;
    builder = nullptr;

    world->GetPlayer(player).AddJobWanted((state == BuildingSiteState::Planing) ? Job::Planer : Job::Builder, this);
}

unsigned noBuildingSite::CalcDistributionPoints(const GoodType goodtype)
{
    // Leveling requires no materials
    if(state == BuildingSiteState::Planing)
        return 0;

    // We only need boards and stones.
    if(goodtype != GoodType::Boards && goodtype != GoodType::Stones)
        return 0;

    const unsigned curBoards = ordered_boards.size() + boards + used_boards;
    const unsigned curStones = ordered_stones.size() + stones + used_stones;
    RTTR_Assert(curBoards <= BUILDING_COSTS[this->bldType_].boards);
    RTTR_Assert(curStones <= BUILDING_COSTS[this->bldType_].stones);

    // Skip if the required material is already fully stocked
    if((goodtype == GoodType::Boards && curBoards == BUILDING_COSTS[this->bldType_].boards)
       || (goodtype == GoodType::Stones && curStones == BUILDING_COSTS[this->bldType_].stones))
        return 0;

    // Start with 10,000 points so we can subtract values comfortably
    constexpr unsigned basePoints = 10000;
    unsigned points = basePoints;

    // Account for the remaining materials; sites closer to completion receive more points
    points -= (BUILDING_COSTS[bldType_].boards - curBoards) * 20;
    points -= (BUILDING_COSTS[bldType_].stones - curStones) * 20;

    // Factor in the construction priority (lower priority value means higher urgency, hence subtraction)
    const unsigned buildingSitePrio = world->GetPlayer(player).GetBuidingSitePriority(this) * 30;

    if(points > buildingSitePrio)
        points -= buildingSitePrio;
    else
        points = 0;

    RTTR_Assert(points <= basePoints); // Underflow detection. Should never happen...

    return points;
}

void noBuildingSite::AddWare(std::unique_ptr<Ware> ware)
{
    RTTR_Assert(state == BuildingSiteState::Building);

    if(ware->type == GoodType::Boards)
    {
        RTTR_Assert(helpers::contains(ordered_boards, ware.get()));
        ordered_boards.remove(ware.get());
        ++boards;
    } else if(ware->type == GoodType::Stones)
    {
        RTTR_Assert(helpers::contains(ordered_stones, ware.get()));
        ordered_stones.remove(ware.get());
        ++stones;
    } else
        throw std::logic_error("Wrong ware type " + helpers::toString(ware->type));

    // Decrease the player's inventory accordingly
    world->GetPlayer(player).DecreaseInventoryWare(ware->type, 1);
    world->GetPlayer(player).RemoveWare(*ware);
}

void noBuildingSite::WareLost(Ware& ware)
{
    RTTR_Assert(state == BuildingSiteState::Building);

    if(ware.type == GoodType::Boards)
    {
        RTTR_Assert(helpers::contains(ordered_boards, &ware));
        ordered_boards.remove(&ware);
    } else if(ware.type == GoodType::Stones)
    {
        RTTR_Assert(helpers::contains(ordered_stones, &ware));
        ordered_stones.remove(&ware);
    } else
        throw std::logic_error("Wrong ware type lost " + helpers::toString(ware.type));

    OrderConstructionMaterial();
}

void noBuildingSite::TakeWare(Ware* ware)
{
    RTTR_Assert(state == BuildingSiteState::Building);

    // Add the ware to the order list
    if(ware->type == GoodType::Boards)
    {
        RTTR_Assert(!helpers::contains(ordered_boards, ware));
        ordered_boards.push_back(ware);
    } else if(ware->type == GoodType::Stones)
    {
        RTTR_Assert(!helpers::contains(ordered_stones, ware));
        ordered_stones.push_back(ware);
    } else
        throw std::logic_error("Wrong ware type " + helpers::toString(ware->type));
}

bool noBuildingSite::IsBuildingComplete()
{
    return (build_progress == BUILDING_COSTS[bldType_].boards * 8 + BUILDING_COSTS[bldType_].stones * 8);
}

unsigned char noBuildingSite::GetBuildProgress(bool percent) const
{
    if(!percent)
        return build_progress;

    unsigned costs = BUILDING_COSTS[bldType_].boards * 8 + BUILDING_COSTS[bldType_].stones * 8;
    unsigned progress = (((unsigned)build_progress) * 100) / costs;

    return (unsigned char)progress;
}

/// Called when leveling is finished
void noBuildingSite::PlaningFinished()
{
    /// Switch back to normal construction
    state = BuildingSiteState::Building;
    planer = nullptr;

    // Request a builder
    world->GetPlayer(player).AddJobWanted(Job::Builder, this);

    // Order construction materials
    OrderConstructionMaterial();
}

/// Return whether this site was established from a ship
bool noBuildingSite::IsHarborBuildingSiteFromSea() const
{
    if(this->bldType_ == BuildingType::HarborBuilding)
        return world->IsHarborBuildingSiteFromSea(this);
    else
        return false;
}
