// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "noBaseBuilding.h"
#include "BuildingEventLogger.h"
#include "GameInterface.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "Loader.h"
#include "RoadEventLogger.h"
#include "SerializedGameData.h"
#include "Ware.h"
#include "EventManager.h"
#include "addons/const_addons.h"
#include "nobBaseWarehouse.h"
#include "notifications/BuildingNote.h"
#include "world/GameWorld.h"
#include "nodeObjs/noExtension.h"
#include "nodeObjs/noFlag.h"
#include "gameData/BuildingConsts.h"
#include "gameData/DoorConsts.h"
#include "gameData/MapConsts.h"
#include "s25util/Log.h"

noBaseBuilding::noBaseBuilding(const NodalObjectType nop, const BuildingType type, const MapPoint pos,
                               const unsigned char player)
    : noRoadNode(nop, pos, player), bldType_(type), nation(world->GetPlayer(player).nation), door_point_x(1000000),
      door_point_y(DOOR_CONSTS[world->GetPlayer(player).nation][type]), buildStartingFrame(world->GetEvMgr().GetCurrentGF()),
      buildCompleteFrame(0)
{
    MapPoint flagPt = GetFlagPos();
    // Create a flag if none exists yet
    if(world->GetNO(flagPt)->GetType() != NodalObjectType::Flag)
    {
        world->DestroyNO(flagPt, false);
        world->SetNO(flagPt, new noFlag(flagPt, player));
        RoadEventLogger::LogFlagBuilt(world->GetEvMgr().GetCurrentGF(), *world, player, flagPt,
                                      RoadEventLogger::FlagBuildReason::BuildingFront);
    }

    // Set the road connection to the building if it does not already exist (e.g., from a previous construction site)
    if(world->GetPointRoad(pos, Direction::SouthEast) == PointRoad::None)
    {
        world->SetPointRoad(pos, Direction::SouthEast, PointRoad::Normal);

        // Create the road segment between the flag and the building, always from flag TO building
        std::vector<Direction> route(1, Direction::NorthWest);
        // Register the road on both endpoints
        auto* rs = new RoadSegment(RoadType::Normal, world->GetSpecObj<noRoadNode>(flagPt), this, route);
        world->GetSpecObj<noRoadNode>(flagPt)->SetRoute(Direction::NorthWest, rs); // on the flag
        SetRoute(Direction::SouthEast, rs);                                        // on the building
        RoadEventLogger::LogRoadConstructed(world->GetEvMgr().GetCurrentGF(), *world, player, flagPt, route,
                                            RoadType::Normal, false);
    } else
    {
        // Reuse the existing road that is already attached to the flag
        auto* flag = world->GetSpecObj<noFlag>(flagPt);

        RTTR_Assert(flag->GetRoute(Direction::NorthWest));
        SetRoute(Direction::SouthEast, flag->GetRoute(Direction::NorthWest));
        GetRoute(Direction::SouthEast)->SetF2(this);
    }

    // If this is a large castle-type building, place the extensions
    if(GetSize() == BuildingQuality::Castle || GetSize() == BuildingQuality::Harbor)
    {
        for(const Direction i : {Direction::West, Direction::NorthWest, Direction::NorthEast})
        {
            MapPoint pos2 = world->GetNeighbour(pos, i);
            world->DestroyNO(pos2, false);
            world->SetNO(pos2, new noExtension(this));
        }
    }
}

noBaseBuilding::~noBaseBuilding() = default;

void noBaseBuilding::Destroy()
{
    const RoadEventLogger::ScopedRoadDemolitionContext demolitionContext(
      RoadEventLogger::RoadDemolitionReason::BuildingDestroyed);
    DestroyAllRoads();
    world->GetNotifications().publish(BuildingNote(BuildingNote::Destroyed, player, pos, bldType_));
    if(GetGOT() != GO_Type::Buildingsite)
        BuildingEventLogger::LogBuildingDestroyed(world->GetEvMgr().GetCurrentGF(), player, bldType_, GetObjId(), pos.x,
                                                  pos.y);

    if(world->GetGameInterface())
        world->GetGameInterface()->GI_UpdateMinimap(pos);

    // Tear down any extensions if necessary
    DestroyBuildingExtensions();

    // Refund construction costs (not for construction sites)
    const GlobalGameSettings& settings = world->GetGGS();
    if((GetGOT() != GO_Type::Buildingsite)
       && (settings.isEnabled(AddonId::REFUND_MATERIALS) || settings.isEnabled(AddonId::REFUND_ON_EMERGENCY)))
    {
        // Check whether the flag still exists
        noFlag* flag = GetFlag();
        if(flag)
        {
            unsigned percent_index = 0;

            // Choose the refund percentage if the addon is active
            if(settings.isEnabled(AddonId::REFUND_MATERIALS))
                percent_index = settings.getSelection(AddonId::REFUND_MATERIALS);
            // Emergency refund returns 50% when enabled and the player is in emergency mode
            else if(world->GetPlayer(player).hasEmergency() && settings.isEnabled(AddonId::REFUND_ON_EMERGENCY))
                percent_index = 2;

            // Percentages per ware
            const std::array<unsigned, 5> percents = {0, 25, 50, 75, 100};
            const unsigned percent = 10 * percents[percent_index];

            // Calculate the refunded amounts (rounded down)
            unsigned boards = (percent * BUILDING_COSTS[bldType_].boards) / 1000;
            unsigned stones = (percent * BUILDING_COSTS[bldType_].stones) / 1000;

            std::array<GoodType, 2> goods = {GoodType::Boards, GoodType::Stones};
            bool which = false;
            while(flag->HasSpaceForWare() && (boards > 0 || stones > 0))
            {
                if((!which && boards > 0) || (which && stones > 0))
                {
                    // Create the ware
                    auto ware = std::make_unique<Ware>(goods[which], nullptr, flag);
                    ware->WaitAtFlag(flag);
                    // Update the inventory
                    world->GetPlayer(player).IncreaseInventoryWare(goods[which], 1);
                    // Assign a client for the ware
                    ware->SetGoal(world->GetPlayer(player).FindClientForWare(*ware));
                    // Recalculate the ware's route
                    ware->RecalcRoute();
                    // Place the ware at the flag
                    flag->AddWare(std::move(ware));

                    if(!which)
                        --boards;
                    else
                        --stones;
                }

                which = !which;
            }
        }
    }

    noRoadNode::Destroy();
}

void noBaseBuilding::Serialize(SerializedGameData& sgd) const
{
    noRoadNode::Serialize(sgd);

    sgd.PushEnum<uint8_t>(bldType_);
    sgd.PushEnum<uint8_t>(nation);
    sgd.PushSignedInt(door_point_x);
    sgd.PushSignedInt(door_point_y);
    sgd.PushUnsignedInt(buildStartingFrame);
    sgd.PushUnsignedInt(buildCompleteFrame);
}

noBaseBuilding::noBaseBuilding(SerializedGameData& sgd, const unsigned obj_id)
    : noRoadNode(sgd, obj_id), bldType_(sgd.Pop<BuildingType>()), nation(sgd.Pop<Nation>()),
      door_point_x(sgd.PopSignedInt()), door_point_y(sgd.PopSignedInt())
{
    if(sgd.GetGameDataVersion() >= 12)
    {
        buildStartingFrame = sgd.PopUnsignedInt();
        buildCompleteFrame = sgd.PopUnsignedInt();
    } else
    {
        buildStartingFrame = 0;
        buildCompleteFrame = 0;
    }
}

int noBaseBuilding::GetDoorPointX()
{
    // Did we calculate the door point yet?
    if(door_point_x == 1000000)
    {
        // The door is on the line between the building and flag point. The position of the line is set by the y-offset
        // this is why we need the x-offset here according to the equation x = m*y + n
        // with n=0 (as door point is relative to building pos) and m = dx/dy
        const Position bldPos = world->GetNodePos(pos);
        const Position flagPos = world->GetNodePos(GetFlagPos());
        Position diff = flagPos - bldPos;

        // We could have crossed the map border which results in unreasonable diffs
        // clamp the diff to [-w/2,w/2],[-h/2, h/2] (maximum diffs)
        const int mapWidth = world->GetWidth() * TR_W;
        const int mapHeight = world->GetHeight() * TR_H;

        if(diff.x < -mapWidth / 2)
            diff.x += mapWidth;
        else if(diff.x > mapWidth / 2)
            diff.x -= mapWidth;
        if(diff.y < -mapHeight / 2)
            diff.y += mapHeight;
        else if(diff.y > mapHeight / 2)
            diff.y -= mapHeight;

        door_point_x = (door_point_y * diff.x) / diff.y;
    }

    return door_point_x;
}

noFlag* noBaseBuilding::GetFlag() const
{
    return world->GetSpecObj<noFlag>(GetFlagPos());
}

MapPoint noBaseBuilding::GetFlagPos() const
{
    return world->GetNeighbour(pos, Direction::SouthEast);
}

void noBaseBuilding::WareNotNeeded(Ware* ware)
{
    if(!ware)
    {
        RTTR_Assert(false);
        LOG.write("Warning: Trying to remove non-existing ware. Please report this replay!\n");
        return;
    }

    if(ware->IsWaitingInWarehouse())
    {
        // Cancel the warehouse order
        world->GetPlayer(player).RemoveWare(*ware);
        static_cast<nobBaseWarehouse*>(ware->GetLocation())->CancelWare(ware);
    } else
        ware->GoalDestroyed();
}

void noBaseBuilding::DestroyBuildingExtensions()
{
    // Only large buildings have these extensions
    if(GetSize() == BuildingQuality::Castle || GetSize() == BuildingQuality::Harbor)
    {
        for(const Direction i : {Direction::West, Direction::NorthWest, Direction::NorthEast})
        {
            world->DestroyNO(world->GetNeighbour(pos, i));
        }
    }
}

BuildingQuality noBaseBuilding::GetSize() const
{
    return BUILDING_SIZE[bldType_];
}

BlockingManner noBaseBuilding::GetBM() const
{
    return BlockingManner::Building;
}

/// Return the standard building texture
ITexture& noBaseBuilding::GetBuildingImage() const
{
    return GetBuildingImage(bldType_, nation);
}

ITexture& noBaseBuilding::GetBuildingImage(BuildingType type, Nation nation) //-V688
{
    return LOADER.building_cache[nation][type].building;
}

/// Return the door texture for the building
ITexture& noBaseBuilding::GetDoorImage() const
{
    return LOADER.building_cache[nation][bldType_].door;
}
