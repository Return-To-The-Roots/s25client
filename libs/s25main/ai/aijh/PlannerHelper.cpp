//
// Created by pavel on 11.04.25.
//

#include "PlannerHelper.h"

unsigned maxFishers(const AIJH::AIPlayerJH& aijh)
{
    const Inventory& inventory = aijh.player.GetInventory();
    return inventory.goods[GoodType::RodAndLine] + inventory.people[Job::Fisher];
}

unsigned maxHunters(const AIJH::AIPlayerJH& aijh)
{
    const Inventory& inventory = aijh.player.GetInventory();
    return inventory.goods[GoodType::Bow] + inventory.people[Job::Hunter];
}

unsigned maxBakers(const AIJH::AIPlayerJH& aijh)
{
    const Inventory& inventory = aijh.player.GetInventory();
    return inventory.goods[GoodType::Rollingpin] + inventory.people[Job::Baker];
}
unsigned maxButcher(const AIJH::AIPlayerJH& aijh)
{
    const Inventory& inventory = aijh.player.GetInventory();
    return inventory.goods[GoodType::Cleaver] + inventory.people[Job::Butcher];
}
unsigned maxFarmer(const AIJH::AIPlayerJH& aijh)
{
    const Inventory& inventory = aijh.player.GetInventory();
    return inventory.goods[GoodType::Scythe] + inventory.people[Job::Farmer];
}
unsigned maxWoodcutter(const AIJH::AIPlayerJH& aijh)
{
    const Inventory& inventory = aijh.player.GetInventory();
    return inventory.goods[GoodType::Axe] + inventory.people[Job::Woodcutter];
}