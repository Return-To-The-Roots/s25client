// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nobBaseWarehouse.h"
#include "BurnedWarehouse.h"
#include "EventManager.h"
#include "FindWhConditions.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "SerializedGameData.h"
#include "Ware.h"
#include "WineLoader.h"
#include "commonDefines.h"
#include "factories/JobFactory.h"
#include "figures/nofAggressiveDefender.h"
#include "figures/nofCarrier.h"
#include "figures/nofDefender.h"
#include "figures/nofPassiveSoldier.h"
#include "figures/nofPassiveWorker.h"
#include "figures/nofTradeDonkey.h"
#include "figures/nofTradeLeader.h"
#include "figures/nofWarehouseWorker.h"
#include "helpers/containerUtils.h"
#include "helpers/pointerContainerUtils.h"
#include "network/GameClient.h"
#include "nobMilitary.h"
#include "random/Random.h"
#include "variant.h"
#include "world/GameWorld.h"
#include "nodeObjs/noFlag.h"
#include "gameData/JobConsts.h"
#include "gameData/SettingTypeConv.h"
#include "gameData/ShieldConsts.h"
#include "s25util/Log.h"
#include <boost/pointer_cast.hpp>
#include <algorithm>

/// Interval between unload operations (in game frames)
const unsigned empty_INTERVAL = 25;
/// Interval between store operations
const unsigned STORE_INTERVAL = 80;
/// Duration required to produce new carriers
const unsigned PRODUCE_HELPERS_GF = 150;
const unsigned PRODUCE_HELPERS_RANDOM_GF = 20;
/// Duration required to recruit soldiers
const unsigned RECRUITE_GF = 200;
const unsigned RECRUITE_RANDOM_GF = 200;
const unsigned LEAVE_INTERVAL = 20;
const unsigned LEAVE_INTERVAL_RAND = 10;

nobBaseWarehouse::nobBaseWarehouse(const BuildingType type, const MapPoint pos, const unsigned char player,
                                   const Nation nation)
    : nobBaseMilitary(type, pos, player, nation), fetch_double_protection(false), recruiting_event(nullptr),
      empty_event(nullptr), store_event(nullptr)
{
    producinghelpers_event = GetEvMgr().AddEvent(this, PRODUCE_HELPERS_GF + RANDOM_RAND(PRODUCE_HELPERS_RANDOM_GF), 1);
    // Reset reserve counters
    reserve_soldiers_available.fill(0);
    reserve_soldiers_claimed_visual.fill(0);
    reserve_soldiers_claimed_real.fill(0);
}

nobBaseWarehouse::~nobBaseWarehouse() = default;

void nobBaseWarehouse::DestroyBuilding()
{
    // Notify all figures and wares heading here that the warehouse no longer exists
    for(noFigure* dependent_figure : dependent_figures)
    {
        // Only send figures home that are not already at this position.
        if(dependent_figure->GetPos() == GetPos())
        {
            dependent_figure->SetGoalTonullptr();
            dependent_figure->CutCurrentRoad();
            dependent_figure->StartWandering();
        } else
            dependent_figure->GoHome();
    }
    dependent_figures.clear();

    for(Ware* dependent_ware : dependent_wares)
        WareNotNeeded(dependent_ware);
    dependent_wares.clear();

    // Cancel pending events
    GetEvMgr().RemoveEvent(recruiting_event);
    GetEvMgr().RemoveEvent(producinghelpers_event);
    GetEvMgr().RemoveEvent(empty_event);
    GetEvMgr().RemoveEvent(store_event);

    // Clear the waiting ware queue
    for(auto& waiting_ware : waiting_wares)
    {
        waiting_ware->WareLost(player);
        waiting_ware->Destroy();
    }
    waiting_wares.clear();

    // Subtract the remaining stock from the inventory totals
    for(const auto good : helpers::enumRange<GoodType>())
        world->GetPlayer(player).DecreaseInventoryWare(good, inventory[good]);

    // move soldiers from reserve to inventory.
    for(unsigned rank = 0; rank < world->GetGGS().GetMaxMilitaryRank(); ++rank)
    {
        if(reserve_soldiers_available[rank] > 0)
            inventory.real.Add(SOLDIER_JOBS[rank], reserve_soldiers_available[rank]);
    }

    // Spawn the helper object that releases fleeing people over time
    world->AddFigure(pos, std::make_unique<BurnedWarehouse>(pos, player, inventory.real.people));

    nobBaseMilitary::DestroyBuilding();
}

void nobBaseWarehouse::Serialize(SerializedGameData& sgd) const
{
    nobBaseMilitary::Serialize(sgd);

    sgd.PushObjectContainer(waiting_wares, true);
    sgd.PushBool(fetch_double_protection);
    sgd.PushObjectContainer(dependent_figures);
    sgd.PushObjectContainer(dependent_wares, true);
    sgd.PushEvent(producinghelpers_event);
    sgd.PushEvent(recruiting_event);
    sgd.PushEvent(empty_event);
    sgd.PushEvent(store_event);

    for(unsigned i = 0; i < 5; ++i)
    {
        // Persist only the authoritative values; visual counters are reconstructed on load
        sgd.PushUnsignedInt(reserve_soldiers_available[i]);
        sgd.PushUnsignedInt(reserve_soldiers_claimed_real[i]);
    }

    for(const auto i : helpers::enumRange<GoodType>())
    {
        sgd.PushUnsignedInt(inventory.visual[i]);
        sgd.PushUnsignedInt(inventory.real[i]);
        sgd.PushUnsignedChar(static_cast<uint8_t>(inventorySettings[i]));
    }
    for(const auto i : helpers::enumRange<Job>())
    {
        sgd.PushUnsignedInt(inventory.visual[i]);
        sgd.PushUnsignedInt(inventory.real[i]);
        sgd.PushUnsignedChar(static_cast<uint8_t>(inventorySettings[i]));
    }
}

nobBaseWarehouse::nobBaseWarehouse(SerializedGameData& sgd, const unsigned obj_id) : nobBaseMilitary(sgd, obj_id)
{
    sgd.PopObjectContainer(waiting_wares, GO_Type::Ware);
    fetch_double_protection = sgd.PopBool();
    sgd.PopObjectContainer(dependent_figures);
    sgd.PopObjectContainer(dependent_wares, GO_Type::Ware);

    producinghelpers_event = sgd.PopEvent();
    recruiting_event = sgd.PopEvent();
    empty_event = sgd.PopEvent();
    store_event = sgd.PopEvent();

    for(unsigned i = 0; i < 5; ++i)
    {
        reserve_soldiers_available[i] = sgd.PopUnsignedInt();
        reserve_soldiers_claimed_visual[i] = reserve_soldiers_claimed_real[i] = sgd.PopUnsignedInt();
    }

    for(const auto i : helpers::enumRange<GoodType>())
    {
        if(sgd.GetGameDataVersion() < 11 && wineaddon::isWineAddonGoodType(i))
            continue;
        inventory.visual[i] = sgd.PopUnsignedInt();
        inventory.real[i] = sgd.PopUnsignedInt();
        inventorySettings[i] = inventorySettingsVisual[i] = static_cast<InventorySetting>(sgd.PopUnsignedChar());
    }
    for(const auto i : helpers::enumRange<Job>())
    {
        if(sgd.GetGameDataVersion() < 11 && wineaddon::isWineAddonJobType(i))
            continue;
        inventory.visual[i] = sgd.PopUnsignedInt();
        inventory.real[i] = sgd.PopUnsignedInt();
        inventorySettings[i] = inventorySettingsVisual[i] = static_cast<InventorySetting>(sgd.PopUnsignedChar());
    }
}

void nobBaseWarehouse::Clear()
{
    // Add reserve soldiers back
    for(unsigned i = 0; i < reserve_soldiers_available.size(); i++)
        inventory.Add(SOLDIER_JOBS[i], reserve_soldiers_available[i]);
    reserve_soldiers_available.fill(0);

    GamePlayer& owner = world->GetPlayer(player);
    for(const auto i : helpers::enumRange<GoodType>())
        owner.DecreaseInventoryWare(i, inventory[i]);

    for(const auto i : helpers::enumRange<Job>())
        owner.DecreaseInventoryJob(i, inventory[i]);

    inventory.clear();

    for(auto& waiting_ware : waiting_wares)
    {
        waiting_ware->WareLost(player);
        waiting_ware->Destroy();
    }
    waiting_wares.clear();
}

void nobBaseWarehouse::OrderCarrier(noRoadNode& goal, RoadSegment& workplace)
{
    RTTR_Assert(workplace.getCarrier(0) == nullptr);
    const bool isBoatRequired = workplace.GetRoadType() == RoadType::Water;

    // We assume, that the caller already checked, if this is possible
    RTTR_Assert(inventory[Job::Helper]);
    if(isBoatRequired)
        RTTR_Assert(inventory[GoodType::Boat]);

    std::unique_ptr<noFigure> carrier = std::make_unique<nofCarrier>(
      isBoatRequired ? CarrierType::Boat : CarrierType::Normal, pos, player, &workplace, &goal);
    workplace.setCarrier(0, static_cast<nofCarrier*>(carrier.get()));

    if(!UseFigureAtOnce(carrier, goal))
        AddLeavingFigure(std::move(carrier));

    inventory.real.Remove(Job::Helper);
    if(isBoatRequired)
        inventory.real.Remove(GoodType::Boat);

    // If no helper remains, pause recruiting
    TryStopRecruiting();
}

bool nobBaseWarehouse::OrderJob(const Job job, noRoadNode* const goal, const bool allow_recruiting)
{
    RTTR_Assert(goal);
    // Maybe we have to recruit one
    if(!inventory[job])
    {
        if(!allow_recruiting || !TryRecruitJob(job))
            return false;
    }

    std::unique_ptr<noFigure> fig = JobFactory::CreateJob(job, pos, player, goal);
    // Inform the target building that a new worker is on the way (flags handle this differently)
    if(goal->GetType() != NodalObjectType::Flag)
        checkedCast<noBaseBuilding*>(goal)->GotWorker(job, *fig);

    // If the derived class does not use the figure immediately, enqueue it for departure
    if(!UseFigureAtOnce(fig, *goal))
        AddLeavingFigure(std::move(fig));

    inventory.real.Remove(job);

    // If no helper remains, pause recruiting
    TryStopRecruiting();

    return true;
}

nofCarrier* nobBaseWarehouse::OrderDonkey(RoadSegment* road, noRoadNode* const goal_flag)
{
    // Do we have any pack donkeys available?
    if(!inventory[Job::PackDonkey])
        return nullptr;

    auto donkey = std::make_unique<nofCarrier>(CarrierType::Donkey, pos, player, road, goal_flag);
    nofCarrier* donkeyRef = donkey.get();
    AddLeavingFigure(std::move(donkey));
    inventory.real.Remove(Job::PackDonkey);

    return donkeyRef;
}

void nobBaseWarehouse::HandleBaseEvent(const unsigned id)
{
    switch(id)
    {
        case 0:
            leaving_event = nullptr;
            HandleLeaveEvent();
            break;
        case 1:
            producinghelpers_event = nullptr;
            HandleProduceHelperEvent();
            break;
        case 2:
            recruiting_event = nullptr;
            HandleRecrutingEvent();
            break;
        case 3:
            empty_event = nullptr;
            HandleSendoutEvent();
            break;
        case 4:
            store_event = nullptr;
            HandleCollectEvent();
            break;
    }
}

void nobBaseWarehouse::HandleCollectEvent()
{
    // Storing wares done?
    bool storing_done = false;
    // Is storing still wanted?
    bool storing_wanted = false;

    // Determine which wares and figures should be collected
    for(const auto i : helpers::enumRange<GoodType>())
    {
        // Is this ware type configured for collection?
        if(!GetInventorySetting(i).IsSet(EInventorySetting::Collect))
            continue;

        storing_wanted = true;

        // Find a warehouse that still holds this ware
        nobBaseWarehouse* wh = world->GetPlayer(player).FindWarehouse(*this, FW::HasWareButNoCollect(i), false, false);
        // Found one?
        if(wh)
        {
            // Place the order
            Ware* ware = wh->OrderWare(i, this);
            if(ware)
            {
                RTTR_Assert(IsWareDependent(*ware));
                storing_done = true;
                break;
            }
        }
    }

    // If no ware was ordered, try to collect figures instead
    if(!storing_done)
    {
        for(const auto i : helpers::enumRange<Job>())
        {
            // Is this figure type configured for collection?
            if(!GetInventorySetting(i).IsSet(EInventorySetting::Collect))
                continue;

            storing_wanted = true;

            // Look for a warehouse keeping that profession
            nobBaseWarehouse* wh =
              world->GetPlayer(player).FindWarehouse(*this, FW::HasFigureButNoCollect(i, false), false, false);
            // Found one?
            if(wh)
            {
                // Place the order
                if(wh->OrderJob(i, this, false))
                    break;
            }
        }
    }

    // Storing still wanted?
    // Then continue ordering new stuff
    if(storing_wanted)
        store_event = GetEvMgr().AddEvent(this, STORE_INTERVAL, 4);
}

void nobBaseWarehouse::HandleSendoutEvent()
{
    // Fight or something in front of the house? Try again later!
    if(!world->IsRoadNodeForFigures(world->GetNeighbour(pos, Direction::SouthEast)))
    {
        empty_event = GetEvMgr().AddEvent(this, empty_INTERVAL, 3);
        return;
    }

    std::vector<boost_variant2<GoodType, Job>> possibleTypes;
    // Collect candidate wares and figures for export
    // If the flag is full, skip exporting wares
    if(GetFlag()->HasSpaceForWare())
    {
        for(const auto i : helpers::enumRange<GoodType>())
        {
            if(GetInventorySetting(i).IsSet(EInventorySetting::Send) && inventory[i])
                possibleTypes.push_back(i);
        }
    }

    for(const auto i : helpers::enumRange<Job>())
    {
        // Figuren, die noch nicht implementiert sind, nicht nehmen!
        if(GetInventorySetting(i).IsSet(EInventorySetting::Send) && inventory[i])
            possibleTypes.push_back(i);
    }

    // Anything to send out?
    if(possibleTypes.empty())
        // If not, there is nothing to do
        return;

    // Pick a random candidate
    const auto selectedId = RANDOM_ELEMENT(possibleTypes);

    if(holds_alternative<GoodType>(selectedId))
    {
        // Handle wares
        const auto goodType = get<GoodType>(selectedId);
        auto ware = std::make_unique<Ware>(goodType, nullptr, this);
        noBaseBuilding* wareGoal = world->GetPlayer(player).FindClientForWare(*ware);
        if(wareGoal != this)
        {
            ware->SetGoal(wareGoal);

            // Queue the ware for dispatch
            waiting_wares.push_back(std::move(ware));

            AddLeavingEvent();

            // Remove the ware from inventory
            inventory.real.Remove(goodType);

            // If we ran out of recruitment materials, pause recruiting
            TryStopRecruiting();
        } else
            world->GetPlayer(player).RemoveWare(*ware);
    } else
    {
        const auto jobType = get<Job>(selectedId);
        nobBaseWarehouse* wh =
          world->GetPlayer(player).FindWarehouse(*this, FW::AcceptsFigureButNoSend(jobType), true, false);
        if(wh != this)
        {
            auto fig = std::make_unique<nofPassiveWorker>(jobType, pos, player, nullptr);

            if(wh)
                fig->GoHome(wh);
            else
                fig->StartWandering();

            AddLeavingFigure(std::move(fig));

            // Remove the figure from inventory
            inventory.real.Remove(jobType);

            // If we ran out of helpers, pause recruiting
            TryStopRecruiting();
        }
    }

    // Are there more items to export?
    if(AreWaresToEmpty())
        // Schedule the next export attempt
        empty_event = GetEvMgr().AddEvent(this, empty_INTERVAL, 3);
}

void nobBaseWarehouse::HandleRecrutingEvent()
{
    // Recruit as many soldiers as permitted by the military settings.
    // Fractional results are handled by random rounding.

    unsigned max_recruits;
    max_recruits = std::min(inventory[GoodType::Sword], inventory[GoodType::ShieldRomans]);
    max_recruits = std::min(inventory[GoodType::Beer], max_recruits);
    max_recruits = std::min(inventory[Job::Helper], max_recruits);

    GamePlayer& owner = world->GetPlayer(player);
    const unsigned recruiting_ratio = owner.GetMilitarySetting(0);
    unsigned real_recruits = max_recruits * recruiting_ratio / MILITARY_SETTINGS_SCALE[0];
    // Wurde abgerundet?
    unsigned remainingRecruits = real_recruits * recruiting_ratio % MILITARY_SETTINGS_SCALE[0];
    if(remainingRecruits != 0 && unsigned(RANDOM_RAND(MILITARY_SETTINGS_SCALE[0] - 1)) < remainingRecruits)
        ++real_recruits;
    else if(real_recruits == 0)
        return; // Nothing to do

    inventory.Add(Job::Private, real_recruits);
    owner.IncreaseInventoryJob(Job::Private, real_recruits);

    inventory.Remove(Job::Helper, real_recruits);
    owner.DecreaseInventoryJob(Job::Helper, real_recruits);

    inventory.Remove(GoodType::Sword, real_recruits);
    owner.DecreaseInventoryWare(GoodType::Sword, real_recruits);

    inventory.Remove(GoodType::ShieldRomans, real_recruits);
    owner.DecreaseInventoryWare(GoodType::ShieldRomans, real_recruits);

    inventory.Remove(GoodType::Beer, real_recruits);
    owner.DecreaseInventoryWare(GoodType::Beer, real_recruits);

    // Try to schedule another recruitment if possible
    TryRecruiting();

    // If there were no soldiers before
    if(inventory[Job::Private] == real_recruits)
    {
        // Update reserve information
        this->RefreshReserve(0);
        // Notify military buildings that new soldiers are available
        if(inventory[Job::Private] > 0)
            owner.NewSoldiersAvailable(inventory[Job::Private]);
    }
}

void nobBaseWarehouse::HandleProduceHelperEvent()
{
    // Produce additional helpers only if we have fewer than 100
    if(inventory[Job::Helper] < 100)
    {
        inventory.Add(Job::Helper);

        GamePlayer& owner = world->GetPlayer(player);
        owner.IncreaseInventoryJob(Job::Helper, 1);

        if(inventory[Job::Helper] == 1)
        {
            // If no carriers existed previously, re-evaluate routes that might now be serviceable
            owner.FindCarrierForAllRoads();
            // Some carriers with tools might convert into new professions
            owner.FindWarehouseForAllJobs();
        }
    } else if(inventory[Job::Helper] > 100)
    {
        // Curb overpopulation by removing excess helpers
        inventory.Remove(Job::Helper);

        world->GetPlayer(player).DecreaseInventoryJob(Job::Helper, 1);
    }

    producinghelpers_event = GetEvMgr().AddEvent(this, PRODUCE_HELPERS_GF + RANDOM_RAND(PRODUCE_HELPERS_RANDOM_GF), 1);

    // This helper might satisfy recruitment requirements
    TryRecruiting();

    // Consider exporting helpers immediately if configured
    CheckOuthousing(Job::Helper);
}

void nobBaseWarehouse::HandleLeaveEvent()
{
#if RTTR_ENABLE_ASSERTS
    // Harbors have more queues. Ignore for now
    if(GetGOT() != GO_Type::NobHarborbuilding)
    {
        Inventory should = inventory.real;
        for(auto& it : leave_house)
        {
            // Don't count warehouse workers
            if(!it->MemberOfWarehouse())
            {
                if(it->GetJobType() == Job::BoatCarrier)
                    should.Add(Job::Helper);
                else
                    should.Add(it->GetJobType());
            }
        }
        RTTR_Assert(should.people == inventory.visual.people);
    }
#endif

    // If an order was cancelled
    if(leave_house.empty() && waiting_wares.empty())
    {
        go_out = false;
        return;
    }

    // Fight or something in front of the house and we are not defending?
    if(!world->IsRoadNodeForFigures(world->GetNeighbour(pos, Direction::SouthEast)))
    {
        // there's a fight

        // try to find a defender
        const auto it = helpers::find_if(leave_house, [](const auto& sld) {
            return sld->GetGOT() == GO_Type::NofAggressivedefender || sld->GetGOT() == GO_Type::NofDefender;
        });
        // no defender found? trigger next leaving event :)
        if(it == leave_house.end())
        {
            go_out = false;
            AddLeavingEvent();
            return;
        }
        // and make him leave the house first
        // remove defender from list, insert him again in front of all others
        leave_house.push_front(std::move(*it));
        leave_house.erase(it);
    }

    // Let figures leave the building first
    if(!leave_house.empty())
    {
        noFigure& fig = world->AddFigure(pos, std::move(leave_house.front()));
        leave_house.pop_front();

        // Init road walking for figures walking on roads
        if(fig.IsWalkingOnRoad())
            fig.InitializeRoadWalking(GetRoute(Direction::SouthEast), 0, true);

        fig.ActAtFirst();
        // Do not adjust counts for warehouse workers
        if(!fig.MemberOfWarehouse())
        {
            // Was this a boat carrier?
            if(fig.GetJobType() == Job::BoatCarrier)
            {
                // Remove helper and boat separately
                inventory.visual.Remove(Job::Helper);
                inventory.visual.Remove(GoodType::Boat);
            } else
                inventory.visual.Remove(fig.GetJobType());

            if(fig.GetGOT() == GO_Type::NofTradedonkey)
            {
                // If the trade donkey carries wares, adjust the visual inventory
                const auto& carriedWare = static_cast<nofTradeDonkey&>(fig).GetCarriedWare();
                if(carriedWare)
                    inventory.visual.Remove(*carriedWare);
            }
        }
    } else
    {
        if(GetFlag()->HasSpaceForWare())
        {
            // Carry out the next ware
            auto ware = std::move(waiting_wares.front());
            waiting_wares.pop_front();
            inventory.visual.Remove(ConvertShields(ware->type));
            ware->Carry(GetFlag());
            world->AddFigure(pos, std::make_unique<nofWarehouseWorker>(pos, player, std::move(ware), false))
              .WalkToGoal();
        } else
        {
            // No space left at the flag; nothing else should leave for now
            go_out = false;
        }
    }

    // Stop scheduling departures once there are no figures or wares left (or the flag is full)
    if(leave_house.empty() && waiting_wares.empty())
        go_out = false;

    if(go_out)
        leaving_event = GetEvMgr().AddEvent(this, LEAVE_INTERVAL + RANDOM_RAND(LEAVE_INTERVAL_RAND));
}

/// Abgeleitete kann eine gerade erzeugte Ware ggf. sofort verwenden
/// (return true if the ware was consumed on the spot)
bool nobBaseWarehouse::UseWareAtOnce(std::unique_ptr<Ware>& /*ware*/, noBaseBuilding& /*goal*/)
{
    return false;
}

/// Same logic for figures
bool nobBaseWarehouse::UseFigureAtOnce(std::unique_ptr<noFigure>& /*fig*/, noRoadNode& /*goal*/)
{
    return false;
}

Ware* nobBaseWarehouse::OrderWare(const GoodType good, noBaseBuilding* const goal)
{
    RTTR_Assert(goal);
    // Ensure the ware actually exists in inventory (defensive check)
    if(!inventory[good])
    {
        LOG.write("nobBaseWarehouse::OrderWare: WARNING: No ware type %u in warehouse!\n")
          % static_cast<unsigned>(good);
        return nullptr;
    }

    auto ware = std::make_unique<Ware>(good, goal, this);
    inventory.Remove(good);

    // Copy pointer so functions below can take ownership
    Ware* wareRef = ware.get();

    // If we don't want to use the ware right away we add it to the waiting wares
    if(!UseWareAtOnce(ware, *goal))
        AddWaitingWare(std::move(ware));
    RTTR_Assert(!ware);

    // Pause recruiting if key supplies were depleted
    TryStopRecruiting();

    return wareRef;
}

void nobBaseWarehouse::AddWaitingWare(std::unique_ptr<Ware> ware)
{
    inventory.visual.Add(ConvertShields(ware->type));
    ware->WaitInWarehouse(this);
    waiting_wares.push_back(std::move(ware));
    AddLeavingEvent();
}

bool nobBaseWarehouse::FreePlaceAtFlag()
{
    if(!waiting_wares.empty())
    {
        AddLeavingEvent();
        return true;
    } else
    {
        // The flag might have been full earlier and exports were paused
        // Restart the export event if needed
        if(AreWaresToEmpty() && !empty_event)
            // Schedule the next export attempt
            empty_event = GetEvMgr().AddEvent(this, empty_INTERVAL, 3);

        return false;
    }
}

void nobBaseWarehouse::AddWare(std::unique_ptr<Ware> ware)
{
    // Ware not dependent anymore (only if we had a goal)
    if(ware->GetGoal())
    {
        RTTR_Assert(ware->GetGoal() == this); // The goal should be here
        RemoveDependentWare(*ware);
    } else
        RTTR_Assert(!IsWareDependent(*ware));

    // Convert shields from different nations to the Roman variant used internally
    GoodType type = ConvertShields(ware->type);

    world->GetPlayer(player).RemoveWare(*ware);

    inventory.Add(type);

    CheckUsesForNewWare(type);
}

/// Evaluate potential uses for a newly arrived ware
void nobBaseWarehouse::CheckUsesForNewWare(const GoodType gt)
{
    // If it is a tool, look for jobs that can now be created
    if(gt >= GoodType::Tongs && gt <= GoodType::Boat)
    {
        for(const auto job : helpers::EnumRange<Job>{})
        {
            if(JOB_CONSTS[job].tool == gt)
                world->GetPlayer(player).FindWarehouseForAllJobs(job);
        }
    }

    // If it is building material, notify construction sites
    if(gt == GoodType::Boards || gt == GoodType::Stones)
        world->GetPlayer(player).FindMaterialForBuildingSites();

    // Beer or weapons might enable recruiting
    TryRecruiting();

    // Consider exporting the ware immediately if required
    CheckOuthousing(gt);
}

/// Handle all side effects caused by a newly arrived figure
void nobBaseWarehouse::CheckJobsForNewFigure(const Job job)
{
    // A helper entering might enable recruiting
    if(job == Job::Helper)
        TryRecruiting();

    if(job >= Job::Private && job <= Job::General)
    {
        // Update reserve state
        RefreshReserve(getSoldierRank(job));
        if(inventory[job] > 0)
        {
            // Notify all military buildings about the additional soldiers
            world->GetPlayer(player).NewSoldiersAvailable(inventory[job]);
        }
    } else
    {
        if(job == Job::PackDonkey)
        {
            // Assign the donkey to a suitable road if possible
            noRoadNode* goal;
            if(RoadSegment* road = world->GetPlayer(player).FindRoadForDonkey(this, &goal))
                road->GotDonkey(OrderDonkey(road, goal));
        } else
        {
            // Let the economy reevaluate where this profession is needed
            GamePlayer& owner = world->GetPlayer(player);
            owner.FindWarehouseForAllJobs(job);
            // If the figure is a carrier, update road assignments too
            if(job == Job::Helper && inventory[Job::Helper] == 1)
            {
                // Potentially send carriers onto roads
                owner.FindCarrierForAllRoads();
                // Carriers with tools might unlock new professions
                owner.FindWarehouseForAllJobs();
            }
        }
    }

    // Immediately export the figure if configured to do so
    CheckOuthousing(job);
}

void nobBaseWarehouse::AddFigure(std::unique_ptr<noFigure> figure, const bool increase_visual_counts)
{
    // Ignore warehouse workers for inventory bookkeeping
    if(!figure->MemberOfWarehouse())
    {
        // Special handling for boat carriers
        if(figure->GetJobType() == Job::BoatCarrier)
        {
            if(increase_visual_counts)
            {
                inventory.Add(Job::Helper);
                inventory.Add(GoodType::Boat);
            } else
            {
                inventory.real.Add(Job::Helper);
                inventory.real.Add(GoodType::Boat);
            }
        } else
        {
            if(increase_visual_counts)
                inventory.Add(figure->GetJobType());
            else
                inventory.real.Add(figure->GetJobType());
        }
    }

    // Check if we were actually waiting for this figure or if it was just added (e.g. builder that constructed it) to
    // not confuse implementations of Remove...
    if(IsDependentFigure(*figure))
        RemoveDependentFigure(*figure);

    CheckJobsForNewFigure(figure->GetJobType());
    GetEvMgr().AddToKillList(std::move(figure));
}

void nobBaseWarehouse::FetchWare()
{
    if(!fetch_double_protection)
        AddLeavingFigure(std::make_unique<nofWarehouseWorker>(pos, player, nullptr, true));

    fetch_double_protection = false;
}

void nobBaseWarehouse::WareLost(Ware& ware)
{
    RemoveDependentWare(ware);
}

void nobBaseWarehouse::CancelWare(Ware*& ware)
{
    inventory.real.Add(ConvertShields(ware->type));
    helpers::extractPtr(waiting_wares, ware);
    ware = nullptr;
}

/// Cancel a queued figure that can no longer arrive
void nobBaseWarehouse::CancelFigure(noFigure* figure)
{
    auto it = helpers::findPtr(leave_house, figure);
    RTTR_Assert(it != leave_house.end());

    // Reinsert the figure using AddFigure so counts stay consistent
    AddFigure(std::move(*it), false);
    leave_house.erase(it);
}

void nobBaseWarehouse::TakeWare(Ware* ware)
{
    // Track the ware so it can be notified if this warehouse gets destroyed
    RTTR_Assert(!helpers::contains(dependent_wares, ware));
    dependent_wares.push_back(ware);
}

void nobBaseWarehouse::OrderTroops(nobMilitary* goal, std::array<unsigned, NUM_SOLDIER_RANKS>& counts, unsigned& max)
{
    unsigned start, limit;
    int step;

    if(world->GetPlayer(player).GetMilitarySetting(1) >= MILITARY_SETTINGS_SCALE[1] / 2)
    {
        // Traverse ranks in descending order (strongest first)
        start = SOLDIER_JOBS.size();
        step = -1;
        limit = 0;
    } else
    {
        // Traverse ranks in ascending order (weakest first)
        start = 1;
        step = 1;
        limit = SOLDIER_JOBS.size() + 1;
    }

    for(unsigned i = start; i != limit && max; i += step)
    {
        const Job curRank = SOLDIER_JOBS[i - 1];
        // Dispatch soldiers of the current rank as needed
        while(inventory[curRank] && max && counts[i - 1])
        {
            auto soldier = std::make_unique<nofPassiveSoldier>(pos, player, goal, goal, i - 1);
            inventory.real.Remove(curRank);
            goal->GotWorker(curRank, *soldier);
            AddLeavingFigure(std::move(soldier));
            --max;
            --counts[i - 1];
        }
    }
}

nofAggressiveDefender* nobBaseWarehouse::SendAggressiveDefender(nofAttacker& attacker)
{
    // Determine whether any soldiers remain
    unsigned char rank;
    for(rank = SOLDIER_JOBS.size(); rank > 0; --rank) //-V1029
    {
        if(inventory[SOLDIER_JOBS[rank - 1]])
            break;
    }

    // No soldiers left -> abort
    if(!rank)
        return nullptr;

    // Send out the strongest available defender
    auto soldier = std::make_unique<nofAggressiveDefender>(pos, player, *this, rank - 1, attacker);
    nofAggressiveDefender& soldierRef = *soldier;
    inventory.real.Remove(SOLDIER_JOBS[rank - 1]);
    AddLeavingFigure(std::move(soldier));

    troops_on_mission.push_back(&soldierRef);

    return &soldierRef;
}

void nobBaseWarehouse::SoldierLost(nofSoldier* soldier)
{
    // Soldier could not arrive; remove it from tracking
    RTTR_Assert(dynamic_cast<nofActiveSoldier*>(soldier));
    RTTR_Assert(helpers::contains(troops_on_mission, static_cast<nofActiveSoldier*>(soldier)));
    troops_on_mission.remove(static_cast<nofActiveSoldier*>(soldier));
}

void nobBaseWarehouse::AddActiveSoldier(std::unique_ptr<nofActiveSoldier> soldier)
{
    // Add soldier. If he is still in the leave-queue, then don't add him to the visual settings again
    if(helpers::contains(leave_house, soldier))
        inventory.real.Add(soldier->GetJobType());
    else
        inventory.Add(SOLDIER_JOBS[soldier->GetRank()]);

    // Returning soldiers may go back into reserve
    RefreshReserve(soldier->GetRank());

    // Re-assess troop distribution in all buildings
    world->GetPlayer(player).RegulateAllTroops();

    // Returned home
    if(soldier.get() == defender_)
        NoDefender();
    else
    {
        // Ggf. war er auf Mission
        RTTR_Assert(helpers::contains(troops_on_mission, soldier.get()));
        troops_on_mission.remove(soldier.get());
    }

    // Finally dispose the soldier object
    soldier->ResetHome();
    GetEvMgr().AddToKillList(std::move(soldier));
}

std::unique_ptr<nofDefender> nobBaseWarehouse::ProvideDefender(nofAttacker& attacker)
{
    // Count available ranks
    unsigned rank_count = 0;

    for(unsigned i = 0; i < SOLDIER_JOBS.size(); ++i)
    {
        if(inventory[SOLDIER_JOBS[i]] || reserve_soldiers_available[i])
            ++rank_count;
    }

    if(rank_count)
    {
        // Determine the desired rank based on the military defense setting
        unsigned rank = (rank_count - 1) * world->GetPlayer(player).GetMilitarySetting(1) / MILITARY_SETTINGS_SCALE[1];

        // Locate a soldier of the desired rank
        unsigned r = 0;
        for(unsigned i = 0; i < SOLDIER_JOBS.size(); ++i)
        {
            // Prefer active soldiers first
            if(inventory[SOLDIER_JOBS[i]])
            {
                if(r == rank)
                {
                    // This is the soldier we want
                    inventory.real.Remove(SOLDIER_JOBS[i]);
                    return std::make_unique<nofDefender>(pos, player, *this, i, attacker);
                }
                ++r;
            }
            // Otherwise check the reserve
            else if(reserve_soldiers_available[i])
            {
                if(r == rank)
                {
                    // This is the soldier we want
                    --reserve_soldiers_available[i];
                    // Add him back to the visual count so subsequent departures keep numbers non-negative
                    inventory.visual.Add(SOLDIER_JOBS[i]);
                    return std::make_unique<nofDefender>(pos, player, *this, i, attacker);
                }
                ++r;
            }
        }
    }

    // Fallback: check soldiers that are still queued to leave
    for(auto it = leave_house.begin(); it != leave_house.end(); ++it)
    {
        std::unique_ptr<nofSoldier> soldier;
        // Soldat?
        if((*it)->GetGOT() == GO_Type::NofAggressivedefender)
        {
            soldier = boost::static_pointer_cast<nofSoldier>(std::move(*it));
            static_cast<nofAggressiveDefender&>(*soldier).NeedForHomeDefence();
        } else if((*it)->GetGOT() == GO_Type::NofPassivesoldier)
            soldier = boost::static_pointer_cast<nofSoldier>(std::move(*it));
        else
            continue;

        leave_house.erase(it); // Only allowed in the loop as we return now
        soldier->Abrogate();

        auto defender = std::make_unique<nofDefender>(pos, player, *this, soldier->GetRank(), attacker);
        soldier->Destroy();
        return defender;
    }

    return nullptr;
}

bool nobBaseWarehouse::AreRecruitingConditionsComply()
{
    // Compute required helper count based on the first military setting
    unsigned needed_helpers = 100 - 10 * world->GetPlayer(player).GetMilitarySetting(0);

    // Always require at least one helper
    if(!needed_helpers)
        needed_helpers = 1;

    // Only schedule recruitment when all resources are available
    return (inventory[Job::Helper] >= needed_helpers && inventory[GoodType::Sword] && inventory[GoodType::ShieldRomans]
            && inventory[GoodType::Beer]);
}

void nobBaseWarehouse::TryRecruiting()
{
    // Register a recruitment event if none is pending and requirements are satisfied
    if(!recruiting_event)
    {
        if(AreRecruitingConditionsComply())
            recruiting_event = GetEvMgr().AddEvent(this, RECRUITE_GF + RANDOM_RAND(RECRUITE_RANDOM_GF), 2);
    }
}

void nobBaseWarehouse::TryStopRecruiting()
{
    // Cancel the recruitment event if the conditions are no longer met
    if(recruiting_event)
    {
        if(!AreRecruitingConditionsComply())
        {
            GetEvMgr().RemoveEvent(recruiting_event);
            recruiting_event = nullptr;
        }
    }
}

const Inventory& nobBaseWarehouse::GetInventory() const
{
    return inventory.visual;
}

void nobBaseWarehouse::AddGoods(const Inventory& goods, bool addToPlayer)
{
    GamePlayer& owner = world->GetPlayer(player);
    for(const auto i : helpers::enumRange<GoodType>())
    {
        if(!goods[i])
            continue;
        // Can only add canonical shields (romans)
        RTTR_Assert(i == GoodType::ShieldRomans || ConvertShields(i) != GoodType::ShieldRomans);

        inventory.Add(i, goods.goods[i]);
        if(addToPlayer)
            owner.IncreaseInventoryWare(i, goods.goods[i]);
        CheckUsesForNewWare(i);
    }

    for(const auto i : helpers::enumRange<Job>())
    {
        if(!goods.people[i])
            continue;
        // Boatcarriers are added as carriers and boat individually
        RTTR_Assert(i != Job::BoatCarrier);

        inventory.Add(i, goods.people[i]);
        if(addToPlayer)
            owner.IncreaseInventoryJob(i, goods.people[i]);
        CheckJobsForNewFigure(i);
    }
}

void nobBaseWarehouse::AddToInventory()
{
    GamePlayer& owner = world->GetPlayer(player);
    for(const auto i : helpers::enumRange<GoodType>())
        owner.IncreaseInventoryWare(i, inventory[i]);

    for(const auto i : helpers::enumRange<Job>())
        owner.IncreaseInventoryJob(i, inventory[i]);
}

bool nobBaseWarehouse::CanRecruit(const Job job) const
{
    if(const GoodType* requiredTool = JOB_CONSTS[job].tool.get_ptr())
    {
        // Do we have a helper and a tool (if required)?
        return inventory[Job::Helper] > 0 && (*requiredTool == GoodType::Nothing || inventory[*requiredTool] > 0);
    } else // Cannot recruit
        return false;
}

bool nobBaseWarehouse::TryRecruitJob(const Job job)
{
    RTTR_Assert(!helpers::contains(SOLDIER_JOBS, job) && job != Job::PackDonkey);
    if(!CanRecruit(job))
        return false;

    auto& owner = world->GetPlayer(player);

    const GoodType requiredTool = JOB_CONSTS[job].tool.get(); // Validity checked in CanRecruit
    if(requiredTool != GoodType::Nothing)
    {
        inventory.Remove(requiredTool);
        owner.DecreaseInventoryWare(requiredTool, 1);
    }

    inventory.Remove(Job::Helper);
    owner.DecreaseInventoryJob(Job::Helper, 1);

    inventory.Add(job);
    owner.IncreaseInventoryJob(job, 1);
    return true;
}

InventorySetting nobBaseWarehouse::GetInventorySettingVisual(const Job job) const
{
    return inventorySettingsVisual[(job == Job::BoatCarrier) ? Job::Helper : job];
}

InventorySetting nobBaseWarehouse::GetInventorySettingVisual(const GoodType ware) const
{
    return inventorySettingsVisual[ConvertShields(ware)];
}

InventorySetting nobBaseWarehouse::GetInventorySetting(const Job job) const
{
    return inventorySettings[(job == Job::BoatCarrier) ? Job::Helper : job];
}

InventorySetting nobBaseWarehouse::GetInventorySetting(const GoodType ware) const
{
    return inventorySettings[ConvertShields(ware)];
}

/// Update import/export settings for the UI representation
void nobBaseWarehouse::SetInventorySettingVisual(const boost_variant2<GoodType, Job>& what, InventorySetting state)
{
    state.MakeValid();
    visit([this, state](auto type) { inventorySettingsVisual[type] = state; }, what);

    NotifyListeners(1);
}

/// Update the authoritative import/export settings
void nobBaseWarehouse::SetInventorySetting(const boost_variant2<GoodType, Job>& what, InventorySetting state)
{
    state.MakeValid();
    InventorySetting& selectedSetting =
      visit([this](auto type) -> InventorySetting& { return inventorySettings[type]; }, what);

    InventorySetting oldState = selectedSetting;
    selectedSetting = state;

    /// Remote players and replays must update the visual settings as well
    if(GAMECLIENT.IsReplayModeOn() || GAMECLIENT.GetPlayerId() != player)
        SetInventorySettingVisual(what, state);

    if(holds_alternative<GoodType>(what) && oldState.IsSet(EInventorySetting::Stop)
       && !state.IsSet(EInventorySetting::Stop))
    {
        // Previously lost wares might now have a destination again
        world->GetPlayer(player).FindClientForLostWares();
    } // No else here!
    if(!oldState.IsSet(EInventorySetting::Send) && state.IsSet(EInventorySetting::Send))
    {
        // Schedule an export if we have wares to send and no event is active
        auto getWaresOrJobs = [this](auto type) { return inventory[type]; };
        if(!empty_event && visit(getWaresOrJobs, what))
            empty_event = GetEvMgr().AddEvent(this, empty_INTERVAL, 3);
    } else if(!oldState.IsSet(EInventorySetting::Collect) && state.IsSet(EInventorySetting::Collect))
    {
        // Schedule a collect event if we should store wares
        if(!store_event)
            store_event = GetEvMgr().AddEvent(this, STORE_INTERVAL, 4);
    }
    NotifyListeners(1);
}

/// Update all import/export settings for one category (wares or figures)
void nobBaseWarehouse::SetAllInventorySettings(const bool isJob, const std::vector<InventorySetting>& states)
{
    bool isUnstopped = false;
    bool isCollectSet = false;

    const unsigned numElements = isJob ? inventorySettings.people.size() : inventorySettings.goods.size();
    RTTR_Assert(states.size() == numElements);
    InventorySetting* settings = isJob ? inventorySettings.people.begin() : inventorySettings.goods.begin();

    for(InventorySetting state : states)
    {
        state.MakeValid();
        *(settings++) = state;
        isUnstopped |= !state.IsSet(EInventorySetting::Stop);
        isCollectSet |= state.IsSet(EInventorySetting::Collect);
    }

    // Restart handling of lost wares if any setting became unstopped
    if(isUnstopped)
        world->GetPlayer(player).FindClientForLostWares();
    // no else!
    // Schedule exports if required and no event is pending
    if(AreWaresToEmpty() && !empty_event)
        empty_event = GetEvMgr().AddEvent(this, empty_INTERVAL, 3);
    // Schedule collection if requested
    if(isCollectSet && !store_event)
        store_event = GetEvMgr().AddEvent(this, STORE_INTERVAL, 4);
}

bool nobBaseWarehouse::IsWareDependent(const Ware& ware)
{
    return helpers::contains(dependent_wares, &ware);
}

bool nobBaseWarehouse::AreWaresToEmpty() const
{
    // Check if any ware type is marked for export and still available
    for(const auto i : helpers::enumRange<GoodType>())
    {
        if(GetInventorySetting(i).IsSet(EInventorySetting::Send) && inventory[i])
            return true;
    }

    // Repeat the export check for figures
    for(const auto i : helpers::enumRange<Job>())
    {
        if(GetInventorySetting(i).IsSet(EInventorySetting::Send) && inventory[i])
            return true;
    }

    return false;
}

bool nobBaseWarehouse::DefendersAvailable() const
{
    const auto isNonZero = [](const unsigned ct) { return ct != 0; };
    if(helpers::contains_if(reserve_soldiers_available, isNonZero))
        return true;
    for(Job job : SOLDIER_JOBS)
    {
        if(inventory[job])
            return true;
    }

    return false;
}

void nobBaseWarehouse::SetReserveVisual(const unsigned rank, const unsigned count)
{
    reserve_soldiers_claimed_visual[rank] = count;
}

void nobBaseWarehouse::SetRealReserve(const unsigned rank, const unsigned count)
{
    reserve_soldiers_claimed_real[rank] = count;

    // In replays or for remote players, mirror the change to the visual count
    if(GAMECLIENT.IsReplayModeOn() || GAMECLIENT.GetPlayerId() != player)
        reserve_soldiers_claimed_visual[rank] = count;

    // Adjust reserve availability accordingly
    RefreshReserve(rank);
}

void nobBaseWarehouse::RefreshReserve(unsigned rank)
{
    // Are we holding too few or too many soldiers in reserve?
    if(reserve_soldiers_available[rank] < reserve_soldiers_claimed_real[rank])
    {
        // Too few reserves -> try to pull more soldiers in
        if(inventory[SOLDIER_JOBS[rank]])
        {
            // Transfer as many as needed without exceeding availability
            unsigned add = std::min(inventory[SOLDIER_JOBS[rank]],                                           // available
                                    reserve_soldiers_claimed_real[rank] - reserve_soldiers_available[rank]); // required

            // Increase the reserve count
            reserve_soldiers_available[rank] += add;
            // Remove them from the active inventory
            inventory.Remove(SOLDIER_JOBS[rank], add);
        }
    } else if(reserve_soldiers_available[rank] > reserve_soldiers_claimed_real[rank])
    {
        // Too many reserves -> release some soldiers
        unsigned subtract = reserve_soldiers_available[rank] - reserve_soldiers_claimed_real[rank];

        // Decrease the reserve count
        reserve_soldiers_available[rank] -= subtract;
        // Return them to active inventory
        inventory.Add(SOLDIER_JOBS[rank], subtract);
        // if the rank is supposed to be send away, do it!
        CheckOuthousing(SOLDIER_JOBS[rank]);
        // Update troop distribution for military buildings if necessary
        world->GetPlayer(player).RegulateAllTroops();
    }
    // Otherwise reserves match the requested amount
}

void nobBaseWarehouse::CheckOuthousing(const boost_variant2<GoodType, Job>& what)
{
    // Check if we need to send this ware or figure and register an event for this
    // If we already have an event, we don't need to do anything
    if(empty_event)
        return;

    const InventorySetting setting =
      visit(composeVisitor(
              [this](Job job) { // Convert boat carriers to regular carriers before evaluation
                  return GetInventorySetting((job == Job::BoatCarrier) ? Job::Helper : job);
              },
              [this](GoodType good) { return GetInventorySetting(good); }),
            what);

    if(setting.IsSet(EInventorySetting::Send))
        empty_event = GetEvMgr().AddEvent(this, empty_INTERVAL, 3);
}

/// For debug only
bool nobBaseWarehouse::IsDependentFigure(const noFigure& fig) const
{
    return helpers::contains(dependent_figures, &fig);
}

/// Available goods of a specific type that can be used for trading
unsigned nobBaseWarehouse::GetAvailableWaresForTrading(const GoodType gt) const
{
    // We need a helper as leader
    if(!inventory[Job::Helper])
        return 0;

    return std::min(inventory[gt], inventory[Job::PackDonkey]);
}

/// Available figures of a speciefic type that can be used for trading
unsigned nobBaseWarehouse::GetAvailableFiguresForTrading(const Job job) const
{
    // We need a helper as leader
    if(!inventory[Job::Helper])
        return 0;

    if(job == Job::Helper)
        return (inventory[Job::Helper] - 1) / 2; // need one as leader
    else
        return std::min(inventory[job], inventory[Job::Helper] - 1);
}

/// Starts a trade caravane from this warehouse
void nobBaseWarehouse::StartTradeCaravane(const boost_variant2<GoodType, Job>& what, const unsigned count,
                                          const TradeRoute& tr, nobBaseWarehouse* goal)
{
    auto tlOwned = std::make_unique<nofTradeLeader>(pos, player, tr, this->GetPos(), goal->GetPos());
    auto& tl = *tlOwned;
    AddLeavingFigure(std::move(tlOwned));

    // Create the donkeys or other people
    nofTradeDonkey* last = nullptr;
    for(unsigned i = 0; i < count; ++i)
    {
        auto next = std::make_unique<nofTradeDonkey>(pos, player, what);

        if(last)
            last->SetSuccessor(next.get());
        else
            tl.SetSuccessor(next.get());

        last = next.get();
        AddLeavingFigure(std::move(next));
    }

    GamePlayer& owner = world->GetPlayer(player);
    // Remove leader
    inventory.real.Remove(Job::Helper);
    owner.DecreaseInventoryJob(Job::Helper, 1);

    // Also diminish the count of donkeys
    visit(composeVisitor(
            [&](const Job job) {
                // remove the jobs
                inventory.real.Remove(job, count);
                owner.DecreaseInventoryJob(job, count);
            },
            [&](const GoodType gt) {
                // Diminish the goods in the warehouse
                inventory.real.Remove(gt, count);
                owner.DecreaseInventoryWare(gt, count);
                // now that we have removed the goods lets remove the donkeys
                inventory.real.Remove(Job::PackDonkey, count);
                owner.DecreaseInventoryJob(Job::PackDonkey, count);
            }),
          what);
}
