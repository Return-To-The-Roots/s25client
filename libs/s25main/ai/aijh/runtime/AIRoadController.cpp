// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AIRoadController.h"

#include "AIPlayerJH.h"

#include "ai/aijh/planning/AIConstruction.h"
#include "ai/AIInterface.h"
#include "buildings/noBuildingSite.h"
#include "nodeObjs/noFlag.h"
#include "world/GameWorldBase.h"
#include "helpers/containerUtils.h"

namespace AIJH {

AIRoadController::AIRoadController(AIPlayerJH& owner) : owner_(owner) {}

void AIRoadController::HandleRoadConstructionComplete(MapPoint pt, Direction dir)
{
    AIConstruction& construction = owner_.GetConstruction();
    const AIInterface& ai = owner_.GetInterface();
    const auto* flag = owner_.gwb.GetSpecObj<noFlag>(pt);
    if(!flag)
        return;

    const RoadSegment* const roadSeg = flag->GetRoute(dir);
    if(!roadSeg || roadSeg->GetLength() < 4)
        return;

    const noFlag& otherFlag = roadSeg->GetOtherFlag(*flag);
    const MapPoint bldPos = owner_.gwb.GetNeighbour(otherFlag.GetPos(), Direction::NorthWest);
    if(ai.IsBuildingOnNode(bldPos, BuildingType::Storehouse) || ai.IsBuildingOnNode(bldPos, BuildingType::HarborBuilding)
       || ai.IsBuildingOnNode(bldPos, BuildingType::Headquarters))
        construction.SetFlagsAlongRoad(otherFlag, roadSeg->GetOtherFlagDir(*flag) + 3u);
    else
        construction.SetFlagsAlongRoad(*flag, dir);
}

void AIRoadController::HandleRoadConstructionFailed(const MapPoint pt, Direction)
{
    const auto* flag = owner_.gwb.GetSpecObj<noFlag>(pt);
    if(!flag)
        return;
    if(flag->GetPlayer() != owner_.playerId)
        return;

    if(RemoveUnusedRoad(*flag, boost::none, true, false))
        owner_.GetConstruction().AddConnectFlagJob(flag);
}

bool AIRoadController::IsFlagPartOfCircle(const noFlag& startFlag, unsigned maxlen, const noFlag& curFlag,
                                          helpers::OptionalEnum<Direction> excludeDir,
                                          std::vector<const noFlag*> oldFlags)
{
    const AIInterface& ai = owner_.GetInterface();
    if(!oldFlags.empty() && &startFlag == &curFlag)
        return true;
    if(maxlen < 1)
        return false;
    for(Direction testDir : helpers::EnumRange<Direction>{})
    {
        if(testDir == excludeDir)
            continue;
        if(testDir == Direction::NorthWest
           && (ai.IsObjectTypeOnNode(owner_.gwb.GetNeighbour(curFlag.GetPos(), Direction::NorthWest),
                                     NodalObjectType::Building)
               || ai.IsObjectTypeOnNode(owner_.gwb.GetNeighbour(curFlag.GetPos(), Direction::NorthWest),
                                        NodalObjectType::Buildingsite)))
        {
            continue;
        }
        const RoadSegment* route = curFlag.GetRoute(testDir);
        if(route)
        {
            const noFlag& flag = route->GetOtherFlag(curFlag);
            if(!helpers::contains(oldFlags, &flag))
            {
                oldFlags.push_back(&flag);
                Direction revDir = route->GetOtherFlagDir(curFlag) + 3u;
                if(IsFlagPartOfCircle(startFlag, maxlen - 1, flag, revDir, oldFlags))
                    return true;
            }
        }
    }
    return false;
}

void AIRoadController::RemoveAllUnusedRoads(const MapPoint pt)
{
    AIConstruction& construction = owner_.GetConstruction();
    std::vector<const noFlag*> flags = construction.FindFlags(pt, 25);
    std::vector<const noFlag*> reconnectflags;
    for(const noFlag* flag : flags)
    {
        if(RemoveUnusedRoad(*flag, boost::none, true, false))
            reconnectflags.push_back(flag);
    }
    owner_.UpdateNodesAround(pt, 25);
    for(const noFlag* flag : reconnectflags)
        construction.AddConnectFlagJob(flag);
}

void AIRoadController::CheckForUnconnectedBuildingSites()
{
    AIConstruction& construction = owner_.GetConstruction();
    // if(construction.GetConnectJobNum() > 0 || construction.GetBuildJobNum() > 0)
    if(construction.GetConnectJobNum() > 0 )
        return;

    for(noBuildingSite* bldSite : owner_.player.GetBuildingRegister().GetBuildingSites())
    {
        noFlag* flag = bldSite->GetFlag();
        bool foundRoute = false;
        for(const auto dir : helpers::EnumRange<Direction>{})
        {
            if(dir == Direction::NorthWest)
                continue;
            if(flag->GetRoute(dir))
            {
                foundRoute = true;
                break;
            }
        }
        if(!foundRoute)
            construction.AddConnectFlagJob(flag);
    }
}

bool AIRoadController::RemoveUnusedRoad(const noFlag& startFlag, helpers::OptionalEnum<Direction> excludeDir,
                                        bool firstflag, bool allowcircle, bool keepstartflag)
{
    AIInterface& ai = owner_.GetInterface();
    helpers::OptionalEnum<Direction> foundDir, foundDir2;
    unsigned char finds = 0;
    for(Direction dir : helpers::EnumRange<Direction>{})
    {
        if(dir == excludeDir)
            continue;
        if(dir == Direction::NorthWest
           && (ai.IsObjectTypeOnNode(owner_.gwb.GetNeighbour(startFlag.GetPos(), Direction::NorthWest),
                                     NodalObjectType::Building)
               || ai.IsObjectTypeOnNode(owner_.gwb.GetNeighbour(startFlag.GetPos(), Direction::NorthWest),
                                        NodalObjectType::Buildingsite)))
        {
            return true;
        }
        if(startFlag.GetRoute(dir))
        {
            ++finds;
            if(finds == 1)
                foundDir = dir;
            else if(finds == 2)
                foundDir2 = dir;
        }
    }

    if(finds > 2)
        return false;
    else if(finds == 2)
    {
        if(allowcircle)
        {
            if(!IsFlagPartOfCircle(startFlag, 10, startFlag, boost::none, {}))
                return false;
            if(!firstflag)
                return false;
        }
        else
            return false;
    }

    if(keepstartflag)
    {
        if(foundDir)
            ai.DestroyRoad(startFlag.GetPos(), *foundDir);
    }
    else
        ai.DestroyFlag(&startFlag);

    if(!foundDir)
        return false;

    Direction revDir1 = startFlag.GetRoute(*foundDir)->GetOtherFlagDir(startFlag) + 3u;
    RemoveUnusedRoad(startFlag.GetRoute(*foundDir)->GetOtherFlag(startFlag), revDir1, false);
    if(foundDir2)
    {
        Direction revDir2 = startFlag.GetRoute(*foundDir2)->GetOtherFlagDir(startFlag) + 3u;
        RemoveUnusedRoad(startFlag.GetRoute(*foundDir2)->GetOtherFlag(startFlag), revDir2, false);
    }
    return false;
}

} // namespace AIJH
