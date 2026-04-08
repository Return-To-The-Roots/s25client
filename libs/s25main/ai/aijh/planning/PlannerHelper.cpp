//
// Created by pavel on 11.04.25.
//

#include "PlannerHelper.h"

#include "GamePlayer.h"
#include "ai/aijh/runtime/AIWorldView.h"
#include "gameData/BuildingConsts.h"
#include "gameData/JobConsts.h"
#include "gameTypes/Inventory.h"

unsigned maxWorkers(const AIJH::AIWorldView& aijh, BuildingType type)
{
    const Inventory& inventory = aijh.GetPlayer().GetInventory();
    helpers::OptionalEnum<Job> job = BLD_WORK_DESC[type].job;
    if(job.has_value())
    {
        Job jobVal = job.value();
        GoodType requiredTool = JOB_CONSTS[jobVal].tool.get();
        return inventory.goods[requiredTool] + inventory.people[jobVal];
    }
    return inventory.people[Job::Helper];
}

unsigned maxFishers(const AIJH::AIWorldView& aijh)
{
    const Inventory& inventory = aijh.GetPlayer().GetInventory();
    return inventory.goods[GoodType::RodAndLine] + inventory.people[Job::Fisher];
}

unsigned maxHunters(const AIJH::AIWorldView& aijh)
{
    const Inventory& inventory = aijh.GetPlayer().GetInventory();
    return inventory.goods[GoodType::Bow] + inventory.people[Job::Hunter];
}

unsigned maxBakers(const AIJH::AIWorldView& aijh)
{
    const Inventory& inventory = aijh.GetPlayer().GetInventory();
    return inventory.goods[GoodType::Rollingpin] + inventory.people[Job::Baker];
}
unsigned maxButcher(const AIJH::AIWorldView& aijh)
{
    const Inventory& inventory = aijh.GetPlayer().GetInventory();
    return inventory.goods[GoodType::Cleaver] + inventory.people[Job::Butcher];
}
unsigned maxFarmer(const AIJH::AIWorldView& aijh)
{
    const Inventory& inventory = aijh.GetPlayer().GetInventory();
    return inventory.goods[GoodType::Scythe] + inventory.people[Job::Farmer];
}
unsigned maxWoodcutter(const AIJH::AIWorldView& aijh)
{
    const Inventory& inventory = aijh.GetPlayer().GetInventory();
    return inventory.goods[GoodType::Axe] + inventory.people[Job::Woodcutter];
}
unsigned maxIronFounder(const AIJH::AIWorldView& aijh)
{
    const Inventory& inventory = aijh.GetPlayer().GetInventory();
    return inventory.goods[GoodType::Crucible] + inventory.people[Job::IronFounder];
}
