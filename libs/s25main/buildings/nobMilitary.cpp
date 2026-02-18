// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nobMilitary.h"
#include "BuildingEventLogger.h"
#include "EventManager.h"
#include "FindWhConditions.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "Loader.h"
#include "MilitaryEventLogger.h"
#include "MilitaryStatsHolder.h"
#include "Point.h"
#include "SerializedGameData.h"
#include "Ware.h"
#include "addons/const_addons.h"
#include "CombatEventLogger.h"
#include "buildings/nobBaseWarehouse.h"
#include "figures/nofAggressiveDefender.h"
#include "figures/nofAttacker.h"
#include "figures/nofDefender.h"
#include "figures/nofPassiveSoldier.h"
#include "helpers/containerUtils.h"
#include "helpers/pointerContainerUtils.h"
#include "helpers/reverse.h"
#include "network/GameClient.h"
#include "notifications/BuildingNote.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "pathfinding/FindPathReachable.h"
#include "postSystem/PostMsgWithBuilding.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include "nodeObjs/noFlag.h"
#include "gameData/BuildingConsts.h"
#include "gameData/BuildingProperties.h"
#include "gameData/GameConsts.h"
#include "gameData/MilitaryConsts.h"
#include "gameData/SettingTypeConv.h"
#include "s25util/Log.h"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace {
constexpr unsigned DEFENDER_BONUS_CAPTURE_DELAY_GFS = 5000;
}

nobMilitary::nobMilitary(const BuildingType type, const MapPoint pos, const unsigned char player, const Nation nation)
    : nobBaseMilitary(type, pos, player, nation), new_built(true), numCoins(0), coinsDisabled(false),
      coinsDisabledVirtual(false), capturing(false), capturing_soldiers(0), goldorder_event(nullptr),
      upgrade_event(nullptr), is_regulating_troops(false), total_troop_limit(0)
{
    // Register this building as military and insert it into a military grid square
    world->GetMilitarySquares().Add(this);

    // Determine the building size
    switch(type)
    {
        case BuildingType::Barracks: size = 0; break;
        case BuildingType::Guardhouse: size = 1; break;
        case BuildingType::Watchtower: size = 2; break;
        case BuildingType::Fortress: size = 3; break;
        default:
            RTTR_Assert(false);
            size = 0xFF;
            break;
    }
    troop_limits.fill(GetMaxTroopsCt());
    total_troop_limit = GetMaxTroopsCt();

    // Keep the door open until the building is occupied
    OpenDoor();

    // Stop the gold supply if new military buildings should not receive coins
    if(world->GetGGS().isEnabled(AddonId::NO_COINS_DEFAULT))
    {
        coinsDisabled = true;
        coinsDisabledVirtual = true;
    }
}

nobMilitary::~nobMilitary() = default;

size_t nobMilitary::GetTotalSoldiers() const
{
    size_t sum = troops.size() + ordered_troops.size() + troops_on_mission.size();
    if(defender_)
        sum++;
    sum += /* capturing_soldiers*/ +far_away_capturers.size();
    return sum;
}

std::array<unsigned, NUM_SOLDIER_RANKS> nobMilitary::GetTotalSoldiersByRank() const
{
    std::array<unsigned, NUM_SOLDIER_RANKS> counts = {0};
    for(const auto& troop : troops)
        ++counts[troop->GetRank()];
    for(const auto& troop : ordered_troops)
        ++counts[troop->GetRank()];
    for(const auto& troop : troops_on_mission)
        ++counts[troop->GetRank()];
    if(defender_)
        ++counts[defender_->GetRank()];
    for(const auto& troop : far_away_capturers)
        ++counts[troop->GetRank()];
    return counts;
}

void nobMilitary::DestroyBuilding()
{
    // Remove from military square and buildings first, to avoid e.g. sending canceled soldiers back to this building
    world->GetMilitarySquares().Remove(this);

    // Cancel outstanding orders
    CancelOrders();

    // Send stationed soldiers outside
    for(auto& troop : troops)
    {
        MilitaryEventLogger::LogUndeployment(GetEvMgr().GetCurrentGF(), player, troop->GetRank(), bldType_, GetObjId());
        troop->LeftBuilding();
        auto& soldier = world->AddFigure(pos, std::move(troop));
        soldier.StartWandering();
        soldier.StartWalking(RANDOM_ENUM(Direction));
    }
    troops.clear();

    // Inform far-away capturers
    for(auto* far_away_capturer : far_away_capturers)
        far_away_capturer->AttackedGoalDestroyed();
    far_away_capturers.clear();

    // Remove pending events if necessary
    GetEvMgr().RemoveEvent(goldorder_event);
    GetEvMgr().RemoveEvent(upgrade_event);

    // Remove remaining gold coins from the player's stock
    world->GetPlayer(player).DecreaseInventoryWare(GoodType::Coins, numCoins);

    nobBaseMilitary::DestroyBuilding();
    // If this was occupied, recalc territory. AFTER calling base destroy as otherwise figures might get stuck here
    if(!new_built)
        world->RecalcTerritory(*this, TerritoryChangeReason::Destroyed);

    world->GetNotifications().publish(BuildingNote(BuildingNote::Lost, player, pos, bldType_));
}

void nobMilitary::Serialize(SerializedGameData& sgd) const
{
    nobBaseMilitary::Serialize(sgd);
    sgd.PushBool(new_built);
    sgd.PushUnsignedChar(numCoins);
    sgd.PushBool(coinsDisabled);
    sgd.PushEnum<uint8_t>(frontier_distance);
    sgd.PushUnsignedChar(size);
    sgd.PushBool(capturing);
    sgd.PushUnsignedInt(capturing_soldiers);
    sgd.PushEvent(goldorder_event);
    sgd.PushEvent(upgrade_event);

    sgd.PushObjectContainer(ordered_troops, true);
    sgd.PushObjectContainer(ordered_coins, true);
    sgd.PushObjectContainer(troops, true);
    sgd.PushObjectContainer(far_away_capturers, true);
    helpers::pushContainer(sgd, troop_limits);
    sgd.PushUnsignedInt(total_troop_limit);
}

nobMilitary::nobMilitary(SerializedGameData& sgd, const unsigned obj_id)
    : nobBaseMilitary(sgd, obj_id), new_built(sgd.PopBool()), numCoins(sgd.PopUnsignedChar()),
      coinsDisabled(sgd.PopBool()), coinsDisabledVirtual(coinsDisabled), frontier_distance(sgd.Pop<FrontierDistance>()),
      size(sgd.PopUnsignedChar()), capturing(sgd.PopBool()), capturing_soldiers(sgd.PopUnsignedInt()),
      goldorder_event(sgd.PopEvent()), upgrade_event(sgd.PopEvent()), is_regulating_troops(false),
      total_troop_limit(0)
{
    sgd.PopObjectContainer(ordered_troops, GO_Type::NofPassivesoldier);
    sgd.PopObjectContainer(ordered_coins, GO_Type::Ware);
    sgd.PopObjectContainer(troops, GO_Type::NofPassivesoldier);
    sgd.PopObjectContainer(far_away_capturers, GO_Type::NofAttacker);

    if(sgd.GetGameDataVersion() < 10)
        troop_limits.fill(GetMaxTroopsCt());
    else
        helpers::popContainer(sgd, troop_limits);

    if(sgd.GetGameDataVersion() < 15)
        total_troop_limit = GetMaxTroopsCt();
    else
        total_troop_limit = sgd.PopUnsignedInt();

    // Insert into the military grid square
    world->GetMilitarySquares().Add(this);

    if(capturing && capturing_soldiers == 0 && aggressors.empty())
    {
        LOG.write(
          "Bug in savegame detected: Building at (%d,%d) Being captured has no capturers. Trying to fix this...\n")
          % pos.x % pos.y;
        capturing = false;
    }
}

void nobMilitary::Draw(DrawPoint drawPt)
{
    // Draw the building itself
    DrawBaseBuilding(drawPt);

    // Draw up to four occupant flags
    auto flags = std::min<unsigned>(troops.size() + this->leave_house.size(), 4);

    for(unsigned i = 0; i < flags; ++i)
    {
        const unsigned flagTexture = 3162 + GAMECLIENT.GetGlobalAnimation(8, 2, 1, pos.x * pos.y * i);
        LOADER.GetMapPlayerImage(flagTexture)
          ->drawForPlayer(drawPt + TROOPS_FLAG_OFFSET[nation][size] + DrawPoint(0, i * 3),
                          world->GetPlayer(player).color);
    }

    // Draw the flag that shows how far the building is from the border
    FrontierDistance frontier_distance_tmp = frontier_distance;
    ITexture* bitmap = nullptr;
    unsigned animationFrame = GAMECLIENT.GetGlobalAnimation(4, 1, 1, pos.x * pos.y * GetObjId());
    if(new_built)
    {
        // don't draw a flag for new buildings - fixes bug #215
    } else if(frontier_distance_tmp == FrontierDistance::Harbor)
    {
        // TODO: harbor flag
        bitmap = LOADER.GetTextureN("map_new", 3150 + animationFrame);
    } else
    {
        if(frontier_distance_tmp == FrontierDistance::Near)
            frontier_distance_tmp = FrontierDistance::Harbor;
        bitmap = LOADER.GetMapTexture(3150 + rttr::enum_cast(frontier_distance_tmp) * 4 + animationFrame);
    }
    if(bitmap)
        bitmap->DrawFull(drawPt + BORDER_FLAG_OFFSET[nation][size]);

    // Draw the exterior sign if the gold supply is currently disabled
    if(coinsDisabledVirtual)
        LOADER.GetMapTexture(46)->DrawFull(drawPt + BUILDING_SIGN_CONSTS[nation][bldType_]);
}

void nobMilitary::HandleEvent(const unsigned id)
{
    switch(id)
    {
        // "Exit" event
        case 0:
        {
            leaving_event = nullptr;

            // Are there still people waiting to leave?
            if(!leave_house.empty())
            {
                // Send the next person out
                noFigure& soldier = world->AddFigure(pos, std::move(leave_house.front()));
                leave_house.pop_front();

                soldier.ActAtFirst();
            }

            // Schedule another exit if more figures remain inside
            if(!leave_house.empty())
                leaving_event = GetEvMgr().AddEvent(this, 30 + RANDOM_RAND(10));
            else
                go_out = false;

            RegulateTroops();
        }
        break;
        // Coin ordering event
        case 1:
        {
            goldorder_event = nullptr;

            // Check again for a new gold coin delivery
            SearchCoins();
        }
        break;
        // Promotion event
        case 2:
        {
            upgrade_event = nullptr;

            // Promote exactly one lowest-ranked soldier if possible
            const uint8_t maxRank = world->GetGGS().GetMaxMilitaryRank();
            nofPassiveSoldier* soldierToUpgrade = nullptr;
            uint8_t bestRank = maxRank;
            for(auto& soldier : troops)
            {
                const uint8_t rank = soldier->GetRank();
                if(rank < bestRank)
                {
                    soldierToUpgrade = soldier.get();
                    bestRank = rank;
                }
            }

            if(soldierToUpgrade && numCoins > 0 && bestRank < maxRank)
            {
                auto upgradedSoldier = helpers::extractPtr(troops, soldierToUpgrade);
                upgradedSoldier->Upgrade();
                MilitaryStatsHolder::ReportUnitUpgrade(player);
                MilitaryEventLogger::LogUpgrade(GetEvMgr().GetCurrentGF(), player, upgradedSoldier->GetRank(), bldType_,
                                                GetObjId());
                troops.insert(std::move(upgradedSoldier));

                // Consume one gold coin
                --numCoins;
                world->GetPlayer(player).DecreaseInventoryWare(GoodType::Coins, 1);

                RegulateTroops();

                // Queue another promotion event if required
                PrepareUpgrading();

                // Order new gold coins if needed
                SearchCoins();
            }
        }
        break;
    }
}

unsigned nobMilitary::GetMilitaryRadius() const
{
    return MILITARY_RADIUS[size];
}

unsigned nobMilitary::GetMaxCoinCt() const
{
    return NUM_GOLDS[nation][size];
}

unsigned nobMilitary::GetMaxTroopsCt() const
{
    return NUM_TROOPS[nation][size];
}

void nobMilitary::LookForEnemyBuildings(const nobBaseMilitary* const exception)
{
    // Search the surrounding area for military buildings
    sortedMilitaryBlds buildings = world->LookForMilitaryBuildings(pos, 3);
    frontier_distance = FrontierDistance::Far;

    const bool frontierDistanceCheck = world->GetGGS().isEnabled(AddonId::FRONTIER_DISTANCE_REACHABLE);

    for(auto* building : buildings)
    {
        // Is it an enemy military building?
        if(building != exception && building->GetPlayer() != player
           && world->GetPlayer(building->GetPlayer()).IsAttackable(player))
        {
            unsigned distance = world->CalcDistance(pos, building->GetPos());
            FrontierDistance newFrontierDistance = FrontierDistance::Far;

            if(distance <= GetMilitaryRadius() + building->GetMilitaryRadius())
            {
                newFrontierDistance = FrontierDistance::Near;
            }
            // in mittlerem Umkreis, also theoretisch angreifbar?
            else if(distance < BASE_ATTACKING_DISTANCE + (GetMaxTroopsCt() - 1) * EXTENDED_ATTACKING_DISTANCE)
            {
                newFrontierDistance = FrontierDistance::Mid;
            } else if(building->GetGOT() == GO_Type::NobMilitary)
            {
                auto* mil = static_cast<nobMilitary*>(building);
                if(distance < BASE_ATTACKING_DISTANCE + (mil->GetMaxTroopsCt() - 1) * EXTENDED_ATTACKING_DISTANCE)
                {
                    newFrontierDistance = FrontierDistance::Mid;
                }
            }

            // if new frontier distance is in military range, check if its reachable.
            if(frontierDistanceCheck && newFrontierDistance >= FrontierDistance::Mid
               && !DoesReachablePathExist(*world, building->GetPos(), pos, MAX_ATTACKING_RUN_DISTANCE))
            {
                // building is not reachable, so its "far" away.
                newFrontierDistance = FrontierDistance::Far;
            }

            // Override our own frontier distance if this building is closer to the border
            if(newFrontierDistance > frontier_distance)
                frontier_distance = newFrontierDistance;

            // Apply the calculated frontier distance to the checked building
            if(BuildingProperties::IsMilitary(building->GetBuildingType()))
                static_cast<nobMilitary*>(building)->NewEnemyMilitaryBuilding(newFrontierDistance);
        }
    }
    // check for harbor points
    if(frontier_distance <= FrontierDistance::Mid && world->GetGGS().isEnabled(AddonId::SEA_ATTACK)
       && world->CalcDistanceToNearestHarbor(pos) < SEAATTACK_DISTANCE + 2)
        frontier_distance = FrontierDistance::Harbor;

    // send troops
    RegulateTroops();
}

void nobMilitary::NewEnemyMilitaryBuilding(const FrontierDistance distance)
{
    // New frontier building nearby -> adjust our own frontier distance accordingly
    if(distance == FrontierDistance::Near)
    {
        // Close range
        frontier_distance = FrontierDistance::Near;
    }
    // in mittlerem Umkreis?
    else if(distance == FrontierDistance::Mid)
    {
        // Medium distance (only if we were previously far away)
        if(frontier_distance == FrontierDistance::Far)
            frontier_distance = FrontierDistance::Mid;
    }
    RegulateTroops();
}

void nobMilitary::RegulateTroops()
{
    RTTR_Assert(helpers::contains(world->GetPlayer(player).GetBuildingRegister().GetMilitaryBuildings(),
                                  this)); // If this fails, the building is Being destroyed!

    // Skip adjustments while the building is being captured and wait for the final garrison count
    if(IsBeingCaptured())
        return;

    // Already regulate its troops => Don't call this method again
    if(is_regulating_troops)
        return;

    is_regulating_troops = true;

    // This only has an effect if the military control addon is in use and the troop limits have been lowered.
    std::array<unsigned, NUM_SOLDIER_RANKS> counts = GetTotalSoldiersByRank();
    std::array<unsigned, NUM_SOLDIER_RANKS> lack;
    for(unsigned rank = 0; rank < NUM_SOLDIER_RANKS; rank++)
    {
        int excess = counts[rank] - troop_limits[rank];
        if(excess > 0)
        {
            lack[rank] = 0;

            std::vector<nofPassiveSoldier*> notNeededSoldiers;
            for(auto it = ordered_troops.begin(); excess && it != ordered_troops.end();)
            {
                if((*it)->GetRank() == rank)
                {
                    notNeededSoldiers.push_back(*it);
                    it = ordered_troops.erase(it);
                    --excess;
                } else
                {
                    ++it;
                }
            }
            for(auto* notNeededSoldier : notNeededSoldiers)
                notNeededSoldier->NotNeeded();
        } else
        {
            // This bit is for ordering troops later
            lack[rank] = troop_limits[rank] - counts[rank];
        }
        if(excess > 0
           && world->GetPlayer(player).FindWarehouse(*this, FW::AcceptsFigure(SOLDIER_JOBS[rank]), true, false))
        {
            for(auto it = troops.begin(); excess && it != troops.end() && troops.size() > 1;)
            {
                if((*it)->GetRank() == rank)
                {
                    MilitaryEventLogger::LogUndeployment(GetEvMgr().GetCurrentGF(), player, (*it)->GetRank(), bldType_,
                                                         GetObjId());
                    (*it)->LeaveBuilding();
                    AddLeavingFigure(std::move(*it));
                    it = troops.erase(it);
                    --excess;
                } else
                {
                    ++it;
                }
            }
        }
    }

    // Do we have too many or too few troops?
    int diff = static_cast<int>(CalcRequiredNumTroops()) - static_cast<int>(GetTotalSoldiers());
    if(diff < 0)
    {
        // Too many -> send surplus troops home
        // First send back the ordered soldiers
        // Weak ones first
        std::vector<nofPassiveSoldier*> notNeededSoldiers;
        GamePlayer& owner = world->GetPlayer(player);
        if(owner.GetMilitarySetting(1) > MILITARY_SETTINGS_SCALE[1] / 2)
        {
            for(auto it = ordered_troops.begin(); diff && !ordered_troops.empty(); ++diff)
            {
                notNeededSoldiers.push_back(*it);
                it = ordered_troops.erase(it);
            }
        }
        // Strong ones first
        else
        {
            for(auto it = ordered_troops.rbegin(); diff && !ordered_troops.empty(); ++diff)
            {
                notNeededSoldiers.push_back(*it);
                it = helpers::erase_reverse(ordered_troops, it);
            }
        }

        // send the not-needed-soldiers away
        for(auto* notNeededSoldier : notNeededSoldiers)
        {
            notNeededSoldier->NotNeeded();
        }

        // Only send them away if a path to a warehouse exists
        if(owner.FindWarehouse(*this, FW::NoCondition(), true, false))
        {
            // Then send out the remainder (at least one must stay inside)
            // Remove the weaker soldiers first
            if(owner.GetMilitarySetting(1) > MILITARY_SETTINGS_SCALE[1] / 2)
            {
                for(auto it = troops.begin(); diff && troops.size() > 1; ++diff)
                {
                    MilitaryEventLogger::LogUndeployment(GetEvMgr().GetCurrentGF(), player, (*it)->GetRank(), bldType_,
                                                         GetObjId());
                    (*it)->LeaveBuilding();
                    AddLeavingFigure(std::move(*it));
                    it = troops.erase(it);
                }
            }
            // Remove the stronger soldiers first
            else
            {
                for(auto it = troops.rbegin(); diff && troops.size() > 1; ++diff)
                {
                    MilitaryEventLogger::LogUndeployment(GetEvMgr().GetCurrentGF(), player, (*it)->GetRank(), bldType_,
                                                         GetObjId());
                    (*it)->LeaveBuilding();
                    AddLeavingFigure(std::move(*it));
                    it = helpers::erase_reverse(troops, it);
                }
            }
        }

    } else if(diff > 0)
    {
        // Not enough troops

        // Building is under attack and the addon limits replacements to the defender setting
        if(IsUnderAttack() && world->GetGGS().getSelection(AddonId::DEFENDER_BEHAVIOR) == 2)
        {
            diff = (world->GetPlayer(player).GetMilitarySetting(2) * diff) / MILITARY_SETTINGS_SCALE[2];
        }
        // only order new troops if there is a chance that there is a path - pathfinding from each warehouse with
        // soldiers to this mil building will start at the warehouse and cost time
        bool mightHaveRoad = false;
        for(const auto dir : helpers::enumRange<Direction>())
        {
            // every direction but 1 because 1 is the building connection so it doesn't count for this check
            if(dir == Direction::NorthWest)
                continue;
            if(GetFlag()->GetRoute(dir))
            {
                mightHaveRoad = true;
                break;
            }
        }
        if(mightHaveRoad)
            world->GetPlayer(player).OrderTroops(this, lack, diff);
    }

    is_regulating_troops = false;
}

unsigned nobMilitary::CalcRequiredNumTroops() const
{
    unsigned desired =
      CalcRequiredNumTroops(frontier_distance,
                            world->GetPlayer(player).GetMilitarySetting(4 + rttr::enum_cast(frontier_distance)));
    return std::min(desired, total_troop_limit);
}

unsigned nobMilitary::CalcRequiredNumTroops(FrontierDistance assumedFrontierDistance, unsigned settingValue) const
{
    return (GetMaxTroopsCt() - 1) * settingValue / MILITARY_SETTINGS_SCALE[4 + rttr::enum_cast(assumedFrontierDistance)]
           + 1;
}

void nobMilitary::SetTroopLimit(const unsigned rank, const unsigned limit)
{
    troop_limits[rank] = limit;
    RegulateTroops();
}

void nobMilitary::SetTotalTroopLimit(const unsigned limit)
{
    const unsigned clamped_limit = std::max(1u, std::min(limit, GetMaxTroopsCt()));
    if(total_troop_limit == clamped_limit)
        return;
    total_troop_limit = clamped_limit;
    RegulateTroops();
}

bool nobMilitary::IsUseless() const
{
    if(frontier_distance != FrontierDistance::Far || new_built)
        return false;
    return !world->DoesDestructionChangeTerritory(*this);
}

bool nobMilitary::IsAttackable(unsigned playerIdx) const
{
    // Cannot be attacked, if it is Being captured or not claimed yet (just built)
    return nobBaseMilitary::IsAttackable(playerIdx) && !IsBeingCaptured() && !IsNewBuilt();
}

bool nobMilitary::IsInTroops(const nofPassiveSoldier& soldier) const
{
    return helpers::containsPtr(troops, &soldier);
}

void nobMilitary::TakeWare(Ware* ware)
{
    // Add the gold coin to the order list
    RTTR_Assert(!helpers::contains(ordered_coins, ware));
    ordered_coins.push_back(ware);
}

void nobMilitary::AddWare(std::unique_ptr<Ware> ware)
{
    // One more gold piece stored
    ++numCoins;
    // Remove it from the order list
    RTTR_Assert(helpers::contains(ordered_coins, ware.get()));
    ordered_coins.remove(ware.get());

    // Delete the ware entity
    world->GetPlayer(player).RemoveWare(*ware);
    ware.reset();

    // Trigger soldier upgrades if possible
    PrepareUpgrading();
}

void nobMilitary::WareLost(Ware& ware)
{
    // A gold coin failed to arrive -> remove it from the order list
    RTTR_Assert(helpers::contains(ordered_coins, &ware));
    ordered_coins.remove(&ware);
}

bool nobMilitary::FreePlaceAtFlag()
{
    return false;
}
void nobMilitary::GotWorker(Job /*job*/, noFigure& worker)
{
    RTTR_Assert(dynamic_cast<nofPassiveSoldier*>(&worker));
    auto& soldier = static_cast<nofPassiveSoldier&>(worker);
    RTTR_Assert(soldier.GetPlayer() == player);
    ordered_troops.insert(&soldier);
}

void nobMilitary::CancelOrders()
{
    // Send ordered soldiers back
    for(auto* ordered_troop : ordered_troops)
        ordered_troop->NotNeeded();

    ordered_troops.clear();

    // Return ordered gold coins
    for(auto* ordered_coin : ordered_coins)
        WareNotNeeded(ordered_coin);

    ordered_coins.clear();
}

void nobMilitary::AddActiveSoldier(std::unique_ptr<nofActiveSoldier> soldier)
{
    // An active soldier returned -> convert him into a passive soldier (clone and destroy original)
    AddPassiveSoldier(std::make_unique<nofPassiveSoldier>(*soldier));

    soldier->ResetHome();

    RTTR_Assert(soldier->GetPlayer() == player);

    // Returned home
    if(soldier.get() == defender_)
        NoDefender();
    else if(helpers::contains(troops_on_mission, soldier.get()))
    {
        troops_on_mission.remove(soldier.get());
    } else
    {
        RTTR_Assert(dynamic_cast<nofAttacker*>(soldier.get()));
        if(IsBeingCaptured() || IsFarAwayCapturer(static_cast<const nofAttacker&>(*soldier)))
        {
            GetEvMgr().AddToKillList(std::move(soldier));
            return;
        }
    }
    GetEvMgr().AddToKillList(std::move(soldier));
    // Do only if not capturing
    RegulateTroops();
}

void nobMilitary::AddPassiveSoldier(std::unique_ptr<nofPassiveSoldier> soldier)
{
    RTTR_Assert(soldier->GetPlayer() == player);
    RTTR_Assert(troops.size() < GetMaxTroopsCt());
    const unsigned char rank = soldier->GetRank();

    ordered_troops.erase(soldier.get());
    troops.insert(std::move(soldier));
    MilitaryEventLogger::LogDeployment(GetEvMgr().GetCurrentGF(), player, rank, bldType_, GetObjId());

    // Was this building occupied for the first time?
    if(new_built)
    {
        SendPostMessage(
          player, std::make_unique<PostMsgWithBuilding>(GetEvMgr().GetCurrentGF(), _("Military building occupied"),
                                                        PostCategory::Military, *this, SoundEffect::Fanfare));
        // Mark as occupied
        new_built = false;
        // Update territory borders
        world->RecalcTerritory(*this, TerritoryChangeReason::Build);
        // Close the door
        CloseDoor();
        world->GetNotifications().publish(BuildingNote(BuildingNote::Captured, player, pos, bldType_));
    } else
    {
        // Try to promote soldiers
        PrepareUpgrading();
    }

    // Search for gold coins; new soldiers might need upgrades
    SearchCoins();
}

void nobMilitary::SoldierLost(nofSoldier* soldier)
{
    // Soldier failed to arrive -> remove him and request replacements if necessary
    if(soldier->GetGOT() == GO_Type::NofPassivesoldier)
    {
        RTTR_Assert(helpers::contains(ordered_troops, static_cast<nofPassiveSoldier*>(soldier)));
        ordered_troops.erase(static_cast<nofPassiveSoldier*>(soldier));
    } else
    {
        auto* actSoldier = dynamic_cast<nofActiveSoldier*>(soldier);
        RTTR_Assert(actSoldier);
        RTTR_Assert(helpers::contains(troops_on_mission, actSoldier));
        troops_on_mission.remove(actSoldier);
    }
    RegulateTroops();
}

void nobMilitary::SendAttacker(nofPassiveSoldier*& passive_soldier, nobBaseMilitary& goal,
                               const nobHarborBuilding* harbor)
{
    auto attacker = std::make_unique<nofAttacker>(*passive_soldier, goal, harbor);
    MilitaryEventLogger::LogUndeployment(GetEvMgr().GetCurrentGF(), player, passive_soldier->GetRank(), bldType_,
                                         GetObjId());
    passive_soldier->LeftBuilding();
    helpers::extractPtr(troops, passive_soldier)->Destroy();
    passive_soldier = nullptr;
    troops_on_mission.push_back(attacker.get());
    AddLeavingFigure(std::move(attacker));
}

nofPassiveSoldier* nobMilitary::ChooseSoldier()
{
    if(troops.empty())
        return nullptr;

    std::array<nofPassiveSoldier*, 5> candidates = {nullptr, nullptr, nullptr, nullptr, nullptr}; // candidates per rank

    // how many ranks
    unsigned rank_count = 0;

    for(auto& troop : troops)
    {
        if(!candidates[troop->GetRank()])
        {
            ++rank_count;
            candidates[troop->GetRank()] = troop.get();
        }
    }

    // Compute the index of the desired rank
    unsigned rank = ((rank_count - 1) * world->GetPlayer(player).GetMilitarySetting(1)) / MILITARY_SETTINGS_SCALE[1];

    unsigned r = 0;

    // Look for the matching rank
    for(auto* candidate : candidates)
    {
        if(candidate)
        {
            if(r == rank)
                // This is the soldier we are looking for
                return candidate;

            ++r;
        }
    }

    return nullptr;
}

nofAggressiveDefender* nobMilitary::SendAggressiveDefender(nofAttacker& attacker)
{
    // Don't send last soldier
    if(GetNumTroops() <= 1)
        return nullptr;
    nofPassiveSoldier* soldier = ChooseSoldier();
    if(soldier)
    {
        // Create a new aggressive defender from that soldier
        auto defender = std::make_unique<nofAggressiveDefender>(*soldier, attacker);
        MilitaryEventLogger::LogUndeployment(GetEvMgr().GetCurrentGF(), player, soldier->GetRank(), bldType_,
                                             GetObjId());
        soldier->LeftBuilding();
        helpers::extractPtr(troops, soldier)->Destroy();
        troops_on_mission.push_back(defender.get());
        nofAggressiveDefender* result = defender.get();
        AddLeavingFigure(std::move(defender));
        return result;
    } else
        return nullptr;
}

/// Returns the number of soldiers available to attack a specific target
unsigned nobMilitary::GetNumSoldiersForAttack(const MapPoint dest) const
{
    // Determine how many soldiers we may take based on the attack settings

    unsigned short soldiers_count =
      (GetNumTroops() > 1) ?
        ((GetNumTroops() - 1) * world->GetPlayer(GetPlayer()).GetMilitarySetting(3) / MILITARY_SETTINGS_SCALE[3]) :
        0;

    unsigned distance = world->CalcDistance(pos, dest);

    // Subtract soldiers if the distance exceeds the base attack range
    if(distance > BASE_ATTACKING_DISTANCE)
    {
        // Remove one soldier for every EXTENDED_ATTACKING_DISTANCE step
        unsigned short soldiers_to_remove =
          ((distance - BASE_ATTACKING_DISTANCE + EXTENDED_ATTACKING_DISTANCE - 1) / EXTENDED_ATTACKING_DISTANCE);
        if(soldiers_to_remove < soldiers_count)
            soldiers_count -= soldiers_to_remove;
        else
            return 0;
    }

    // The walking path must not exceed the limit either; only then can we commit the soldiers
    if(soldiers_count && world->FindHumanPath(pos, dest, MAX_ATTACKING_RUN_DISTANCE))
        // Use that many soldiers
        return soldiers_count;
    else
        return 0;
}

/// Returns the soldiers available to attack a specific target
std::vector<nofPassiveSoldier*> nobMilitary::GetSoldiersForAttack(const MapPoint dest) const
{
    std::vector<nofPassiveSoldier*> soldiers;
    unsigned soldiers_count = GetNumSoldiersForAttack(dest);
    for(const auto& sld : helpers::reverse(troops))
    {
        if(soldiers_count--)
            soldiers.push_back(sld.get());
        else
            break;
    }
    return soldiers;
}

/// Returns the total strength of soldiers available to attack a specific target
unsigned nobMilitary::GetSoldiersStrengthForAttack(const MapPoint dest, unsigned& soldiers_count) const
{
    unsigned strength = 0;

    soldiers_count = GetNumSoldiersForAttack(dest);
    unsigned numRemainingSoldiers = soldiers_count;

    for(const auto& sld : helpers::reverse(troops))
    {
        if(numRemainingSoldiers--)
            strength += HITPOINTS[sld->GetRank()];
        else
            break;
    }

    return strength;
}

/// Returns the strength of the soldiers stationed inside this military building
unsigned nobMilitary::GetSoldiersStrength() const
{
    unsigned strength = 0;
    for(const auto& troop : troops)
        strength += HITPOINTS[troop->GetRank()];

    return strength;
}

unsigned nobMilitary::CalcDefenderBonusHp() const
{
    if(!world)
        return 0;

    return GetOriginOwner() == GetPlayer() ? 1U : 0U;
}

unsigned nobMilitary::GetGarrisonStrengthWithBonus() const
{
    const unsigned bonusHp = CalcDefenderBonusHp();
    unsigned strength = 0;
    for(const auto& soldier : troops)
    {
        const unsigned rankHp = HITPOINTS[soldier->GetRank()];
        unsigned effectiveHp = soldier->GetHitpoints();
        const unsigned maxAllowed = rankHp + bonusHp;

        if(bonusHp && effectiveHp == rankHp)
            effectiveHp = maxAllowed;
        else if(effectiveHp > maxAllowed)
            effectiveHp = maxAllowed;

        strength += effectiveHp;
    }

    return strength;
}

unsigned nobMilitary::EstimateCaptureLossCount() const
{
    if(!world)
        return 0;
    return world->CountBuildingsLostOnCapture(*this);
}

bool nobMilitary::HasUpgradeableSoldier() const
{
    const unsigned maxRank = world->GetGGS().GetMaxMilitaryRank();
    return helpers::contains_if(troops,
                                [maxRank](const auto& soldier) { return soldier->GetRank() < maxRank; });
}

/// is there a max rank soldier in the building?
bool nobMilitary::HasMaxRankSoldier() const
{
    const unsigned maxRank = world->GetGGS().GetMaxMilitaryRank();
    return helpers::contains_if(helpers::reverse(troops),
                                [maxRank](const auto& it) { return it->GetRank() >= maxRank; });
}

std::unique_ptr<nofDefender> nobMilitary::ProvideDefender(nofAttacker& attacker)
{
    nofPassiveSoldier* soldier = ChooseSoldier();
    if(!soldier)
    {
        /// Cancel soldiers that still want to go on a mission and pull them back for defense
        CancelJobs();
        // Nochmal versuchen
        soldier = ChooseSoldier();
        if(!soldier)
            return nullptr;
    }

    auto defender = std::make_unique<nofDefender>(*soldier, attacker);

    auto oldSoldier = helpers::extractPtr(troops, soldier);
    MilitaryEventLogger::LogUndeployment(GetEvMgr().GetCurrentGF(), player, oldSoldier->GetRank(), bldType_,
                                         GetObjId());
    oldSoldier->LeftBuilding();
    oldSoldier->Destroy();

    return defender;
}

void nobMilitary::Capture(const unsigned char new_owner)
{
    RTTR_Assert(IsBeingCaptured());

    captureRisk_ = 0.0;
    captureRiskCached_ = false;
    importance_ = 0.0;

    // Transfer the stored gold coins from the old player to the new owner
    world->GetPlayer(player).DecreaseInventoryWare(GoodType::Coins, numCoins);
    world->GetPlayer(new_owner).IncreaseInventoryWare(GoodType::Coins, numCoins);

    // Reset desired troop setting
    troop_limits.fill(GetMaxTroopsCt());
    total_troop_limit = GetMaxTroopsCt();

    // Notify soldiers that are currently on missions
    for(auto* it : troops_on_mission)
        it->HomeDestroyed();

    // Cancel orders that are still en route to this building
    CancelOrders();

    // Tell agressive defending soldiers and far away capturers that their goal is gone
    for(auto* soldier : aggressive_defenders)
        soldier->AttackedGoalDestroyed();
    for(auto* soldier : far_away_capturers)
        soldier->AttackedGoalDestroyed();

    troops_on_mission.clear();
    aggressive_defenders.clear();
    far_away_capturers.clear();

    // Remember the previous owner
    unsigned char old_player = player;
    world->GetPlayer(old_player).RemoveBuilding(this, bldType_);
    // Change ownership to the new player
    player = new_owner;
    SetCapturedGF(GetEvMgr().GetCurrentGF());
    // Update the economic records for both players
    world->GetPlayer(new_owner).AddBuilding(this, bldType_);

    // Transfer ownership of the flag in front of the building
    GetFlag()->Capture(new_owner);

    // Recalculate territory
    world->RecalcTerritory(*this, TerritoryChangeReason::Captured);
    BuildingEventLogger::LogBuildingCaptured(GetEvMgr().GetCurrentGF(), player, bldType_, GetObjId(), pos.x, pos.y);
    CombatEventLogger::LogCapture(GetEvMgr().GetCurrentGF(), new_owner, old_player, bldType_, GetObjId());

    // Update visibility for the former owner
    world->RecalcVisibilitiesAroundPoint(pos, GetMilitaryRadius() + VISUALRANGE_MILITARY + 1, old_player, nullptr);

    // Recalculate frontier flags for enemies
    LookForEnemyBuildings();
    // And for allies as the enemy building disappeared
    sortedMilitaryBlds buildings = world->LookForMilitaryBuildings(pos, 4);
    for(auto* building : buildings)
    {
        // Allied building?
        if(world->GetPlayer(building->GetPlayer()).IsAttackable(old_player)
           && BuildingProperties::IsMilitary(building->GetBuildingType()))
            // Recalculate that building's border flags
            static_cast<nobMilitary*>(building)->LookForEnemyBuildings();
    }

    // Send former occupants home if they were on their way in or out
    std::array<MapPoint, 2> coords = {pos, world->GetNeighbour(pos, Direction::SouthEast)};
    for(const auto& coord : coords)
    {
        for(noBase& baseFigure : world->GetFigures(coord))
        {
            if(baseFigure.GetType() == NodalObjectType::Figure)
            {
                auto& figure = static_cast<noFigure&>(baseFigure);
                if(figure.GetCurrentRoad() == GetRoute(Direction::SouthEast) && figure.GetPlayer() != new_owner)
                {
                    figure.Abrogate();
                    figure.StartWandering();
                }
            }
        }
    }

    // Send all allied aggressors home (we own the building now!)
    for(auto it = aggressors.begin(); it != aggressors.end();)
    {
        nofAttacker* attacker = *it;
        // dont remove attackers owned by players not allied with the new owner!
        unsigned char attPlayer = attacker->GetPlayer();
        if(attPlayer != player && !world->GetPlayer(attPlayer).IsAttackable(player))
        {
            it = aggressors.erase(it);
            attacker->CapturedBuildingFull();
        } else
            ++it;
    }

    // Send notifications to both the old owner and the new owner
    SendPostMessage(old_player,
                    std::make_unique<PostMsgWithBuilding>(GetEvMgr().GetCurrentGF(), _("Military building lost"),
                                                          PostCategory::Military, *this));
    SendPostMessage(player,
                    std::make_unique<PostMsgWithBuilding>(GetEvMgr().GetCurrentGF(), _("Military building captured"),
                                                          PostCategory::Military, *this));

    world->GetNotifications().publish(BuildingNote(BuildingNote::Captured, player, pos, bldType_));
    world->GetNotifications().publish(BuildingNote(BuildingNote::Lost, old_player, pos, bldType_));

    // Check if we need to change the coin order

    switch(world->GetGGS().getSelection(AddonId::COINS_CAPTURED_BLD))
    {
        case 1: // enable coin order
            coinsDisabled = false;
            coinsDisabledVirtual = false;
            break;
        case 2: // disable coin order
            coinsDisabled = true;
            coinsDisabledVirtual = true;
            break;
    }
}

void nobMilitary::NeedOccupyingTroops()
{
    RTTR_Assert(IsBeingCaptured()); // Only valid during capturing
    // Check if we need more soldiers from the attacking soldiers
    // Choose the closest ones first to avoid having them walk a long way

    nofAttacker* best_attacker = nullptr;
    unsigned best_radius = std::numeric_limits<unsigned>::max();

    unsigned needed_soldiers = CalcRequiredNumTroops();
    unsigned currentSoldiers = troops.size() + capturing_soldiers + troops_on_mission.size();

    if(needed_soldiers > currentSoldiers)
    {
        // Look through the attackers
        for(auto* aggressor : aggressors)
        {
            // Is the soldier standing around and owned by the player?
            if(!aggressor->IsAttackerReady() || aggressor->GetPlayer() != player)
                continue;
            // Closer than the current best candidate?
            if(aggressor->GetRadius() >= best_radius)
                continue;
            // And can the soldier actually reach the flag or is he blocked by others in front?
            if(world->FindHumanPath(aggressor->GetPos(), world->GetNeighbour(pos, Direction::SouthEast), 10, false))
            {
                // Then this is the best candidate so far
                best_attacker = aggressor;
                best_radius = best_attacker->GetRadius();
            }
        }

        // Found someone?
        if(best_attacker)
        {
            // Send that soldier inside
            best_attacker->CaptureBuilding();
            ++capturing_soldiers;
            // Nothing more to do now
            return;
        }

        // If necessary look for further soldiers who are not standing around the building
        for(auto it = aggressors.begin();
            it != aggressors.end() && needed_soldiers > currentSoldiers + far_away_capturers.size();)
        {
            nofAttacker* attacker = *it;

            if(attacker->GetPlayer() == player)
            {
                // Ask attacker if this is possible
                if(attacker->CanStartFarAwayCapturing(*this))
                {
                    it = aggressors.erase(it);
                    far_away_capturers.push_back(attacker);
                    continue;
                }
            }
            ++it;
        }
    }

    // At this point aggressors contains only soldiers, that cannot capture the building (from other player or without a
    // path to flag), the one(s) that is currently walking to capture the building and possibly some more from other
    // (non-allied) players So send those home, who cannot capture the building
    for(auto it = aggressors.begin(); it != aggressors.end();)
    {
        nofAttacker* attacker = *it;
        // Do not remove soldiers that are still capturing the building
        // i.e. ignore attackers from players that are not allied with the new owner
        if(attacker->GetState() != nofActiveSoldier::SoldierState::AttackingCapturingNext
           && !world->GetPlayer(attacker->GetPlayer()).IsAttackable(player))
        {
            it = aggressors.erase(it);
            attacker->CapturedBuildingFull();
        } else
            ++it;
    }
}

void nobMilitary::SetCoinsAllowed(const bool enabled)
{
    if(coinsDisabled == !enabled)
        return;

    // Apply the new setting
    coinsDisabled = !enabled;
    // If another player or a replay changed this, keep the visual indicator in sync
    if(GAMECLIENT.GetPlayerId() != player || GAMECLIENT.IsReplayModeOn())
        coinsDisabledVirtual = coinsDisabled;

    if(!coinsDisabled)
        SearchCoins(); // Order coins if we just enabled it
    else
    {
        // send coins back if just deactivated
        for(auto it = ordered_coins.begin(); it != ordered_coins.end();)
        {
            // But only those, that are not just Being carried in
            if((*it)->GetLocation() != this)
            {
                WareNotNeeded(*it);
                it = ordered_coins.erase(it);
            } else
                ++it;
        }
    }
}

unsigned nobMilitary::CalcCoinsPoints() const
{
    // Abort immediately if we do not want gold coins here
    if(!WantCoins())
        return 0;

    // Start with a base value of 10000 so we can subtract from it
    int points = 10000;

    // Existing or ordered coins reduce the priority of requesting more
    points -= (numCoins + ordered_coins.size()) * 30;

    const unsigned maxRank = world->GetGGS().GetMaxMilitaryRank();
    // Count soldiers that can still be promoted
    for(const auto& soldier : troops)
    {
        // As long as the soldier is below max rank he can be promoted
        if(soldier->GetRank() < maxRank)
            points += 20;
    }

    if(points < 0)
        throw std::logic_error("Negative points are not allowed");

    return static_cast<unsigned>(points);
}

bool nobMilitary::WantCoins() const
{
    // Do not request coins if supply is disabled, storage is full, the building is still new, or nobody can be promoted
    return (!coinsDisabled && numCoins + ordered_coins.size() != GetMaxCoinCt() && !new_built && HasUpgradeableSoldier());
}

void nobMilitary::SearchCoins()
{
    if(!HasUpgradeableSoldier())
    {
        if(goldorder_event)
        {
            GetEvMgr().RemoveEvent(goldorder_event);
            goldorder_event = nullptr;
        }

        for(auto it = ordered_coins.begin(); it != ordered_coins.end();)
        {
            if((*it)->GetLocation() != this)
            {
                WareNotNeeded(*it);
                it = ordered_coins.erase(it);
            } else
                ++it;
        }
        return;
    }

    // Only proceed if we want gold coins and no order event is pending
    if(WantCoins() && !goldorder_event)
    {
        // Look for a warehouse that has gold coins
        nobBaseWarehouse* wh =
          world->GetPlayer(player).FindWarehouse(*this, FW::HasMinWares(GoodType::Coins), false, false);
        if(wh)
        {
            // Order one gold coin from that warehouse
            Ware* ware = wh->OrderWare(GoodType::Coins, this);

            if(!ware)
            {
                RTTR_Assert(false);
                // A warehouse with coins should always provide a ware, so this should never happen
                LOG.write("nobMilitary::SearchCoins: WARNING: ware = nullptr. Bug alarm!\n");
                return;
            }

            RTTR_Assert(helpers::contains(ordered_coins, ware));

            // Schedule another check for additional gold coins
            goldorder_event = GetEvMgr().AddEvent(this, 200 + RANDOM_RAND(400), 1);
        }
    }
}

void nobMilitary::PrepareUpgrading()
{
    // Abort if no gold coins are available
    if(!numCoins)
        return;

    // Skip if a promotion event is already scheduled
    if(upgrade_event)
        return;

    // Abort if there are no soldiers that can be promoted
    if(!HasUpgradeableSoldier())
        return;

    // Everything is ready -> schedule the promotion event
    upgrade_event = GetEvMgr().AddEvent(this, UPGRADE_TIME + RANDOM_RAND(UPGRADE_TIME_RANDOM), 2);
}

void nobMilitary::HitOfCatapultStone()
{
    // Remove one soldier if any remain
    if(!troops.empty())
    {
        std::unique_ptr<nofPassiveSoldier> soldier = std::move(*troops.begin());
        helpers::pop_front(troops);
        MilitaryEventLogger::LogUndeployment(GetEvMgr().GetCurrentGF(), player, soldier->GetRank(), bldType_,
                                             GetObjId());
        // Shortcut for Die(): No need to remove from world as it is inside and we can delete it right away
        soldier->RemoveFromInventory();
        soldier->LeftBuilding();
        soldier->Destroy();
    }

    // If there are troops left, order some more, else this will be destroyed
    if(!troops.empty())
        RegulateTroops();

    // Send a notification
    SendPostMessage(player,
                    std::make_unique<PostMsgWithBuilding>(GetEvMgr().GetCurrentGF(), _("A catapult is firing upon us!"),
                                                          PostCategory::Military, *this));
}

/**
 *  Determine whether the military building may be demolished (respecting demolition restrictions).
 */
bool nobMilitary::IsDemolitionAllowed() const
{
    switch(world->GetGGS().getSelection(AddonId::DEMOLITION_PROHIBITION))
    {
        default: // off
            break;
        case 1: // under attack
        {
            // Check if the building is under attack
            if(!aggressors.empty())
                return false;
        }
        break;
        case 2: // near frontiers
        {
            // Check if the building is close to the border
            if(frontier_distance == FrontierDistance::Near)
                return false;
        }
        break;
    }

    return true;
}

void nobMilitary::UnlinkAggressor(nofAttacker& soldier)
{
    RTTR_Assert(IsAggressor(soldier) || IsFarAwayCapturer(soldier));
    aggressors.remove(&soldier);
    far_away_capturers.remove(&soldier);

    if(aggressors.empty())
        RegulateTroops();
}

void nobMilitary::CapturingSoldierArrived()
{
    RTTR_Assert(IsBeingCaptured());
    RTTR_Assert(capturing_soldiers > 0);
    --capturing_soldiers;
    if(capturing_soldiers == 0)
    {
        // Search again
        NeedOccupyingTroops();
        if(capturing_soldiers > 0)
            return; // Found more
        // Capturing finished
        capturing = false;
        // Re-evaluate the garrison
        RegulateTroops();
    }
}

void nobMilitary::FarAwayCapturerReachedGoal(nofAttacker& attacker, bool walkingIntoBld)
{
    RTTR_Assert(IsFarAwayCapturer(attacker));
    if(IsBeingCaptured())
    {
        // If we are still capturing just re-add this soldier to the aggressors
        // one of the currently capturing soldiers will notify him
        far_away_capturers.remove(&attacker);
        aggressors.push_back(&attacker);
    } else if(walkingIntoBld)
    {
        // Otherwise we are in a kind of "normal" working state of the building and will just add him when he gets in
        // Call the next one unless we are not actually walking into the building, i.e. we ware only waiting until we
        // can do so Avoids e.g. https://github.com/Return-To-The-Roots/s25client/issues/1405
        CallNextFarAwayCapturer(attacker);
    }
}

bool nobMilitary::IsFarAwayCapturer(const nofAttacker& attacker)
{
    return helpers::contains(far_away_capturers, &attacker);
}

void nobMilitary::CallNextFarAwayCapturer(nofAttacker& attacker)
{
    const MapPoint flagPos = GetFlagPos();
    unsigned minLength = std::numeric_limits<unsigned>::max();
    nofAttacker* bestAttacker = nullptr;
    for(auto* far_away_capturer : far_away_capturers)
    {
        // Skip us and possible capturers at the building
        if(far_away_capturer == &attacker || far_away_capturer->GetPos() == pos)
            continue;
        if(!far_away_capturer->IsAttackerReady())
            continue;
        RTTR_Assert(far_away_capturer->GetPos() != flagPos); // Impossible. This should be the current attacker
        unsigned length;
        if(!world->FindHumanPath(far_away_capturer->GetPos(), flagPos, MAX_FAR_AWAY_CAPTURING_DISTANCE, false, &length))
            continue;
        if(length < minLength)
        {
            minLength = length;
            bestAttacker = far_away_capturer;
        }
    }
    if(bestAttacker)
        bestAttacker->AttackFlag();
}
