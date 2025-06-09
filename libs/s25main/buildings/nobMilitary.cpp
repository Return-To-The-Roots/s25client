// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nobMilitary.h"
#include "EventManager.h"
#include "FindWhConditions.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "LeatherLoader.h"
#include "Loader.h"
#include "Point.h"
#include "SerializedGameData.h"
#include "Ware.h"
#include "addons/const_addons.h"
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
#include <limits>
#include <stdexcept>

nobMilitary::nobMilitary(const BuildingType type, const MapPoint pos, const unsigned char player, const Nation nation)
    : nobBaseMilitary(type, pos, player, nation), new_built(true), numCoins(0), coinsDisabled(false),
      coinsDisabledVirtual(false), numArmor(0), armorDisabled(false), armorDisabledVirtual(false), capturing(false),
      capturing_soldiers(0), goldorder_event(nullptr), armororder_event(nullptr), upgrade_event(nullptr),
      armor_upgrade_event(nullptr), is_regulating_troops(false)
{
    // Gebäude entsprechend als Militärgebäude registrieren und in ein Militärquadrat eintragen
    world->GetMilitarySquares().Add(this);

    // Größe ermitteln
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

    // Tür aufmachen, bis Gebäude besetzt ist
    OpenDoor();

    // Wenn kein Gold in neu gebaute Militärgebäude eingeliefert werden soll, wird die Goldzufuhr gestoppt
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

    // Bestellungen stornieren
    CancelOrders();

    // Soldaten rausschicken
    for(auto& troop : troops)
    {
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

    // Events ggf. entfernen
    GetEvMgr().RemoveEvent(goldorder_event);
    GetEvMgr().RemoveEvent(armororder_event);
    GetEvMgr().RemoveEvent(upgrade_event);
    GetEvMgr().RemoveEvent(armor_upgrade_event);

    // übriggebliebene Goldmünzen in der Inventur abmelden
    world->GetPlayer(player).DecreaseInventoryWare(GoodType::Coins, numCoins);
    world->GetPlayer(player).DecreaseInventoryWare(GoodType::Armor, numArmor);

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

    sgd.PushUnsignedChar(numArmor);
    sgd.PushBool(armorDisabled);
    sgd.PushEvent(armororder_event);
    sgd.PushEvent(armor_upgrade_event);
    sgd.PushObjectContainer(ordered_armor, true);
}

nobMilitary::nobMilitary(SerializedGameData& sgd, const unsigned obj_id)
    : nobBaseMilitary(sgd, obj_id), new_built(sgd.PopBool()), numCoins(sgd.PopUnsignedChar()),
      coinsDisabled(sgd.PopBool()), coinsDisabledVirtual(coinsDisabled), frontier_distance(sgd.Pop<FrontierDistance>()),
      size(sgd.PopUnsignedChar()), capturing(sgd.PopBool()), capturing_soldiers(sgd.PopUnsignedInt()),
      goldorder_event(sgd.PopEvent()), upgrade_event(sgd.PopEvent()), is_regulating_troops(false)
{
    sgd.PopObjectContainer(ordered_troops, GO_Type::NofPassivesoldier);
    sgd.PopObjectContainer(ordered_coins, GO_Type::Ware);
    sgd.PopObjectContainer(troops, GO_Type::NofPassivesoldier);
    sgd.PopObjectContainer(far_away_capturers, GO_Type::NofAttacker);

    if(sgd.GetGameDataVersion() < 10)
        troop_limits.fill(GetMaxTroopsCt());
    else
        helpers::popContainer(sgd, troop_limits);

    if(sgd.GetGameDataVersion() >= 12)
    {
        numArmor = sgd.PopUnsignedChar();
        armorDisabledVirtual = armorDisabled = sgd.PopBool();
        armororder_event = sgd.PopEvent();
        armor_upgrade_event = sgd.PopEvent();
        sgd.PopObjectContainer(ordered_armor, GO_Type::Ware);
    } else
    {
        numArmor = 0;
        armorDisabledVirtual = armorDisabled = false;
        armororder_event = nullptr;
        armor_upgrade_event = nullptr;
    }

    // ins Militärquadrat einfügen
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
    // Gebäude an sich zeichnen
    DrawBaseBuilding(drawPt);

    // (max 4) Besatzungs-Fähnchen zeichnen
    auto flags = std::min<unsigned>(troops.size() + this->leave_house.size(), 4);

    for(unsigned i = 0; i < flags; ++i)
    {
        const unsigned flagTexture = 3162 + GAMECLIENT.GetGlobalAnimation(8, 2, 1, pos.x * pos.y * i);
        LOADER.GetMapPlayerImage(flagTexture)
          ->drawForPlayer(drawPt + TROOPS_FLAG_OFFSET[nation][size] + DrawPoint(0, i * 3),
                          world->GetPlayer(player).color);
    }

    // Die Fahne, die anzeigt wie weit das Gebäude von der Grenze entfernt ist, zeichnen
    FrontierDistance frontier_distance_tmp = frontier_distance;
    ITexture* bitmap = nullptr;
    unsigned animationFrame = GAMECLIENT.GetGlobalAnimation(4, 1, 1, pos.x * pos.y * GetObjId());
    if(new_built)
    {
        // don't draw a flag for new buildings - fixes bug #215
    } else if(frontier_distance_tmp == FrontierDistance::Harbor)
    {
        // todo Hafenflagge
        bitmap = LOADER.GetTextureN("map_new", 3150 + animationFrame);
    } else
    {
        if(frontier_distance_tmp == FrontierDistance::Near)
            frontier_distance_tmp = FrontierDistance::Harbor;
        bitmap = LOADER.GetMapTexture(3150 + rttr::enum_cast(frontier_distance_tmp) * 4 + animationFrame);
    }
    if(bitmap)
        bitmap->DrawFull(drawPt + BORDER_FLAG_OFFSET[nation][size]);

    // If coins or armor delivery disabled, show sign at building
    if(leatheraddon::isAddonActive(world->GetPlayer(player).GetGameWorld()))
    {
        if(coinsDisabledVirtual)
            LOADER
              .GetImageN("leather_bobs", leatheraddon::bobIndex[leatheraddon::BobTypes::STOP_COINS_X_SIGN_OVERRIDE])
              ->DrawFull(drawPt + BUILDING_SIGN_CONSTS[nation][bldType_]);

        if(armorDisabledVirtual)
            LOADER.GetImageN("leather_bobs", leatheraddon::bobIndex[leatheraddon::BobTypes::STOP_ARMOR_X_SIGN])
              ->DrawFull(drawPt + BUILDING_ARMOR_SIGN_CONSTS[nation][bldType_]);
    }
    // Wenn Goldzufuhr gestoppt ist, Schild außen am Gebäude zeichnen zeichnen
    else
    {
        if(coinsDisabledVirtual)
            LOADER.GetMapTexture(46)->DrawFull(drawPt + BUILDING_SIGN_CONSTS[nation][bldType_]);
    }
}

void nobMilitary::HandleEvent(const unsigned id)
{
    switch(id)
    {
        // "Rausgeh-Event"
        case 0:
        {
            leaving_event = nullptr;

            // Sind Leute da, die noch rausgehen wollen?
            if(!leave_house.empty())
            {
                // Dann raus mit denen
                noFigure& soldier = world->AddFigure(pos, std::move(leave_house.front()));
                leave_house.pop_front();

                soldier.ActAtFirst();
            }

            // Wenn noch weitere drin sind, die müssen auch noch raus
            if(!leave_house.empty())
                leaving_event = GetEvMgr().AddEvent(this, 30 + RANDOM_RAND(10));
            else
                go_out = false;

            RegulateTroops();
        }
        break;
        // Goldbestell-Event
        case 1:
        {
            goldorder_event = nullptr;

            // ggf. nach neuen Goldmünzen suchen
            SearchCoins();
        }
        break;
        // Beförderungs-Event
        case 2:
        {
            upgrade_event = nullptr;

            // Soldaten befördern
            // Von hinten durchgehen
            // Wenn der nachfolgende (schwächere) Soldat einen niedrigeren Rang hat,
            // wird dieser ebenfalls befördert usw.!
            std::vector<std::unique_ptr<nofPassiveSoldier>> soldiersToUpgrade;
            // Rang des letzten beförderten Soldaten, MaxRank am Anfang setzen, damit keiner über den maximalen Rang
            // befördert wird
            uint8_t last_rank = world->GetGGS().GetMaxMilitaryRank();
            for(auto it = troops.rbegin(); it != troops.rend();)
            {
                auto& soldier = *it;
                // Es wurde schon einer befördert, dieser Soldat muss nun einen niedrigeren Rang
                // als der letzte haben, damit er auch noch befördert werden kann
                if(soldier->GetRank() < last_rank)
                {
                    // Rang merken
                    last_rank = soldier->GetRank();
                    // Remove from sorted container as changing it breaks sorting
                    soldiersToUpgrade.push_back(std::move(soldier));
                    it = helpers::erase_reverse(troops, it);
                } else
                    ++it;
            }

            // Wurde jemand befördert?
            if(!soldiersToUpgrade.empty())
            {
                // Reinsert upgraded soldiers
                for(auto& upgradedSoldier : soldiersToUpgrade)
                {
                    upgradedSoldier->Upgrade();
                    troops.insert(std::move(upgradedSoldier));
                }

                // Goldmünze verbrauchen
                --numCoins;
                world->GetPlayer(player).DecreaseInventoryWare(GoodType::Coins, 1);

                RegulateTroops();

                // Evtl neues Beförderungsevent anmelden
                PrepareUpgrading();

                // Ggf. neue Goldmünzen bestellen
                SearchCoins();
            }
        }
        break;
        // Order armor event
        case 3:
        {
            armororder_event = nullptr;
            SearchArmor();
        }
        break;
        // Armor upgrade event
        case 4:
        {
            armor_upgrade_event = nullptr;

            auto canidate = std::find_if(troops.rbegin(), troops.rend(),
                                         [](OwnedSortedTroops::value_type& troop) { return !troop->HasArmor(); });

            if(canidate != troops.rend())
            {
                auto& soldier = *canidate;
                soldier->SetArmor(true);

                --numArmor;
                world->GetPlayer(player).DecreaseInventoryWare(GoodType::Armor, 1);
                world->GetPlayer(player).IncreaseInventoryJob(figureToAmoredSoldierEnum(soldier.get()), 1);

                PrepareArmorUpgrading();
                SearchArmor();
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

unsigned nobMilitary::GetMaxArmorCt() const
{
    return NUM_ARMOR[nation][size];
}

unsigned nobMilitary::GetMaxTroopsCt() const
{
    return NUM_TROOPS[nation][size];
}

void nobMilitary::LookForEnemyBuildings(const nobBaseMilitary* const exception)
{
    // Umgebung nach Militärgebäuden absuchen
    sortedMilitaryBlds buildings = world->LookForMilitaryBuildings(pos, 3);
    frontier_distance = FrontierDistance::Far;

    const bool frontierDistanceCheck = world->GetGGS().isEnabled(AddonId::FRONTIER_DISTANCE_REACHABLE);

    for(auto* building : buildings)
    {
        // feindliches Militärgebäude?
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

            // override own frontier distance, if its nearer to a border
            if(newFrontierDistance > frontier_distance)
                frontier_distance = newFrontierDistance;

            // set calculated frontier distance to checked building.
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
    // Neues Grenzgebäude in der Nähe --> Distanz entsprechend setzen
    if(distance == FrontierDistance::Near)
    {
        // Nah
        frontier_distance = FrontierDistance::Near;
    }
    // in mittlerem Umkreis?
    else if(distance == FrontierDistance::Mid)
    {
        // Mittel (nur wenns vorher auf weit weg war)
        if(frontier_distance == FrontierDistance::Far)
            frontier_distance = FrontierDistance::Mid;
    }
    RegulateTroops();
}

void nobMilitary::RegulateTroops()
{
    RTTR_Assert(helpers::contains(world->GetPlayer(player).GetBuildingRegister().GetMilitaryBuildings(),
                                  this)); // If this fails, the building is Being destroyed!

    // Wenn das Gebäude eingenommen wird, erstmal keine neuen Truppen und warten, wieviele noch reinkommen
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

    // Zu viele oder zu wenig Truppen?
    int diff = static_cast<int>(CalcRequiredNumTroops()) - static_cast<int>(GetTotalSoldiers());
    if(diff < 0)
    {
        // Zu viel --> überflüssige Truppen nach Hause schicken
        // Zuerst die bestellten Soldaten wegschicken
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

        // Nur rausschicken, wenn es einen Weg zu einem Lagerhaus gibt!
        if(owner.FindWarehouse(*this, FW::NoCondition(), true, false))
        {
            // Dann den Rest (einer muss immer noch drinbleiben!)
            // erst die schwachen Soldaten raus
            if(owner.GetMilitarySetting(1) > MILITARY_SETTINGS_SCALE[1] / 2)
            {
                for(auto it = troops.begin(); diff && troops.size() > 1; ++diff)
                {
                    (*it)->LeaveBuilding();
                    AddLeavingFigure(std::move(*it));
                    it = troops.erase(it);
                }
            }
            // erst die starken Soldaten raus
            else
            {
                for(auto it = troops.rbegin(); diff && troops.size() > 1; ++diff)
                {
                    (*it)->LeaveBuilding();
                    AddLeavingFigure(std::move(*it));
                    it = helpers::erase_reverse(troops, it);
                }
            }
        }

    } else if(diff > 0)
    {
        // Zu wenig Truppen

        // Gebäude wird angegriffen und
        // Addon aktiv, nur soviele Leute zum Nachbesetzen schicken wie Verteidiger eingestellt
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
            world->GetPlayer(player).OrderTroops(*this, lack, diff);
    }

    is_regulating_troops = false;
}

unsigned nobMilitary::CalcRequiredNumTroops() const
{
    return CalcRequiredNumTroops(frontier_distance,
                                 world->GetPlayer(player).GetMilitarySetting(4 + rttr::enum_cast(frontier_distance)));
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
    // Goldmünze in Bestellliste aufnehmen
    if(ware->type == GoodType::Coins)
    {
        RTTR_Assert(!helpers::contains(ordered_coins, ware));
        ordered_coins.push_back(ware);
    } else if(ware->type == GoodType::Armor)
    {
        RTTR_Assert(!helpers::contains(ordered_armor, ware));
        ordered_armor.push_back(ware);
    }
}

void nobMilitary::AddWare(std::unique_ptr<Ware> ware)
{
    if(ware->type == GoodType::Coins)
    {
        // Ein Golstück mehr
        ++numCoins;
        // aus der Bestellliste raushaun
        RTTR_Assert(helpers::contains(ordered_coins, ware.get()));
        ordered_coins.remove(ware.get());

        // Ware vernichten
        world->GetPlayer(player).RemoveWare(*ware);
        ware.reset();

        // Evtl. Soldaten befördern
        PrepareUpgrading();
    } else if(ware->type == GoodType::Armor)
    {
        ++numArmor;
        RTTR_Assert(helpers::contains(ordered_armor, ware.get()));
        ordered_armor.remove(ware.get());

        world->GetPlayer(player).RemoveWare(*ware);
        ware.reset();

        PrepareArmorUpgrading();
    }
}

void nobMilitary::WareLost(Ware& ware)
{
    if(ware.type == GoodType::Coins)
    {
        // Ein Goldstück konnte nicht kommen --> aus der Bestellliste entfernen
        RTTR_Assert(helpers::contains(ordered_coins, &ware));
        ordered_coins.remove(&ware);
    } else if(ware.type == GoodType::Armor)
    {
        RTTR_Assert(helpers::contains(ordered_armor, &ware));
        ordered_armor.remove(&ware);
    }
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
    // Soldaten zurückschicken
    for(auto* ordered_troop : ordered_troops)
        ordered_troop->NotNeeded();

    ordered_troops.clear();

    // Goldmünzen zurückschicken
    for(auto* ordered_coin : ordered_coins)
        WareNotNeeded(ordered_coin);

    ordered_coins.clear();

    for(auto* ordered_armor : ordered_armor)
        WareNotNeeded(ordered_armor);

    ordered_armor.clear();
}

void nobMilitary::AddActiveSoldier(std::unique_ptr<nofActiveSoldier> soldier)
{
    // aktiver Soldat, eingetroffen werden --> dieser muss erst in einen passiven Soldaten
    // umoperiert werden (neu erzeugt und alter zerstört) werden
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

    ordered_troops.erase(soldier.get());
    troops.insert(std::move(soldier));

    // Wurde dieses Gebäude zum ersten Mal besetzt?
    if(new_built)
    {
        SendPostMessage(
          player, std::make_unique<PostMsgWithBuilding>(GetEvMgr().GetCurrentGF(), _("Military building occupied"),
                                                        PostCategory::Military, *this, SoundEffect::Fanfare));
        // Ist nun besetzt
        new_built = false;
        // Landgrenzen verschieben
        world->RecalcTerritory(*this, TerritoryChangeReason::Build);
        // Tür zumachen
        CloseDoor();
        world->GetNotifications().publish(BuildingNote(BuildingNote::Captured, player, pos, bldType_));
    } else
    {
        // Evtl. Soldaten befördern
        PrepareUpgrading();
        PrepareArmorUpgrading();
    }

    // Goldmünzen suchen, evtl sinds ja neue Soldaten
    SearchCoins();
    SearchArmor();
}

void nobMilitary::SoldierLost(nofSoldier* soldier)
{
    // Soldat konnte nicht (mehr) kommen --> rauswerfen und ggf. neue Soldaten rufen
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

    // ID ausrechnen
    unsigned rank = ((rank_count - 1) * world->GetPlayer(player).GetMilitarySetting(1)) / MILITARY_SETTINGS_SCALE[1];

    unsigned r = 0;

    // richtigen Rang suchen
    for(auto* candidate : candidates)
    {
        if(candidate)
        {
            if(r == rank)
                // diesen Soldaten wollen wir
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
        // neuen aggressiven Verteidiger daraus erzeugen
        auto defender = std::make_unique<nofAggressiveDefender>(*soldier, attacker);
        soldier->LeftBuilding();
        helpers::extractPtr(troops, soldier)->Destroy();
        troops_on_mission.push_back(defender.get());
        nofAggressiveDefender* result = defender.get();
        AddLeavingFigure(std::move(defender));
        return result;
    } else
        return nullptr;
}

/// Gibt die Anzahl der Soldaten zurück, die für einen Angriff auf ein bestimmtes Ziel zur Verfügung stehen
unsigned nobMilitary::GetNumSoldiersForAttack(const MapPoint dest) const
{
    // Soldaten ausrechnen, wie viel man davon nehmen könnte, je nachdem wie viele in den
    // Militäreinstellungen zum Angriff eingestellt wurden

    unsigned short soldiers_count =
      (GetNumTroops() > 1) ?
        ((GetNumTroops() - 1) * world->GetPlayer(GetPlayer()).GetMilitarySetting(3) / MILITARY_SETTINGS_SCALE[3]) :
        0;

    unsigned distance = world->CalcDistance(pos, dest);

    // Falls Entfernung größer als Basisreichweite, Soldaten subtrahieren
    if(distance > BASE_ATTACKING_DISTANCE)
    {
        // je einen soldaten zum entfernen vormerken für jeden EXTENDED_ATTACKING_DISTANCE großen Schritt
        unsigned short soldiers_to_remove =
          ((distance - BASE_ATTACKING_DISTANCE + EXTENDED_ATTACKING_DISTANCE - 1) / EXTENDED_ATTACKING_DISTANCE);
        if(soldiers_to_remove < soldiers_count)
            soldiers_count -= soldiers_to_remove;
        else
            return 0;
    }

    // und auch der Weg zu Fuß darf dann nicht so weit sein, wenn das alles bestanden ist, können wir ihn nehmen..
    if(soldiers_count && world->FindHumanPath(pos, dest, MAX_ATTACKING_RUN_DISTANCE))
        // Soldaten davon nehmen
        return soldiers_count;
    else
        return 0;
}

/// Gibt die Soldaten zurück, die für einen Angriff auf ein bestimmtes Ziel zur Verfügung stehen
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

/// Gibt die Stärke der Soldaten zurück, die für einen Angriff auf ein bestimmtes Ziel zur Verfügung stehen
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

/// Gibt die Stärke eines Militärgebäudes zurück
unsigned nobMilitary::GetSoldiersStrength() const
{
    unsigned strength = 0;
    for(const auto& troop : troops)
        strength += HITPOINTS[troop->GetRank()];

    return strength;
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
        /// Soldaten, die noch auf Mission gehen wollen, canceln und für die Verteidigung mit einziehen
        CancelJobs();
        // Nochmal versuchen
        soldier = ChooseSoldier();
        if(!soldier)
            return nullptr;
    }

    auto defender = std::make_unique<nofDefender>(*soldier, attacker);

    auto oldSoldier = helpers::extractPtr(troops, soldier);
    oldSoldier->LeftBuilding();
    oldSoldier->Destroy();

    return defender;
}

void nobMilitary::Capture(const unsigned char new_owner)
{
    RTTR_Assert(IsBeingCaptured());

    // Goldmünzen in der Inventur vom alten Spieler abziehen und dem neuen hinzufügen
    world->GetPlayer(player).DecreaseInventoryWare(GoodType::Coins, numCoins);
    world->GetPlayer(new_owner).IncreaseInventoryWare(GoodType::Coins, numCoins);

    world->GetPlayer(player).DecreaseInventoryWare(GoodType::Armor, numArmor);
    world->GetPlayer(new_owner).IncreaseInventoryWare(GoodType::Armor, numArmor);

    // Reset desired troop setting
    troop_limits.fill(GetMaxTroopsCt());

    // Soldaten, die auf Mission sind, Bescheid sagen
    for(auto* it : troops_on_mission)
        it->HomeDestroyed();

    // Bestellungen die hierher unterwegs sind canceln
    CancelOrders();

    // Tell agressive defending soldiers and far away capturers that their goal is gone
    for(auto* soldier : aggressive_defenders)
        soldier->AttackedGoalDestroyed();
    for(auto* soldier : far_away_capturers)
        soldier->AttackedGoalDestroyed();

    troops_on_mission.clear();
    aggressive_defenders.clear();
    far_away_capturers.clear();

    // Alten Besitzer merken
    unsigned char old_player = player;
    world->GetPlayer(old_player).RemoveBuilding(this, bldType_);
    // neuer Spieler
    player = new_owner;
    // In der Wirtschaftsverwaltung dieses Gebäude jetzt zum neuen Spieler zählen und beim alten raushauen
    world->GetPlayer(new_owner).AddBuilding(this, bldType_);

    // Flagge davor auch übernehmen
    GetFlag()->Capture(new_owner);

    // Territorium neu berechnen
    world->RecalcTerritory(*this, TerritoryChangeReason::Captured);

    // Sichtbarkeiten berechnen für alten Spieler
    world->RecalcVisibilitiesAroundPoint(pos, GetMilitaryRadius() + VISUALRANGE_MILITARY + 1, old_player, nullptr);

    // Grenzflagge entsprechend neu setzen von den Feinden
    LookForEnemyBuildings();
    // und von den Verbündeten (da ja ein Feindgebäude weg ist)!
    sortedMilitaryBlds buildings = world->LookForMilitaryBuildings(pos, 4);
    for(auto* building : buildings)
    {
        // verbündetes Gebäude?
        if(world->GetPlayer(building->GetPlayer()).IsAttackable(old_player)
           && BuildingProperties::IsMilitary(building->GetBuildingType()))
            // Grenzflaggen von dem neu berechnen
            static_cast<nobMilitary*>(building)->LookForEnemyBuildings();
    }

    // ehemalige Leute dieses Gebäudes nach Hause schicken, die ggf. grad auf dem Weg rein/raus waren
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

    // Post verschicken, an den alten Besitzer und an den neuen Besitzer
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
        // Soldaten absuchen
        for(auto* aggressor : aggressors)
        {
            // Is the soldier standing around and owned by the player?
            if(!aggressor->IsAttackerReady() || aggressor->GetPlayer() != player)
                continue;
            // Näher als der bisher beste?
            if(aggressor->GetRadius() >= best_radius)
                continue;
            // Und kommt er überhaupt zur Flagge (könnte ja in der 2. Reihe stehen, sodass die vor ihm ihn den Weg
            // versperren)?
            if(world->FindHumanPath(aggressor->GetPos(), world->GetNeighbour(pos, Direction::SouthEast), 10, false))
            {
                // Dann is das der bisher beste
                best_attacker = aggressor;
                best_radius = best_attacker->GetRadius();
            }
        }

        // Einen gefunden?
        if(best_attacker)
        {
            // Dann soll der hingehen
            best_attacker->CaptureBuilding();
            ++capturing_soldiers;
            // und raus hier
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
        // Nicht gerade Soldaten löschen, die das Gebäude noch einnehmen!
        // also: dont remove attackers owned by players not allied with the new owner!
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

    // Umstellen
    coinsDisabled = !enabled;
    // Wenn das von einem fremden Spieler umgestellt wurde (oder vom Replay), muss auch das visuelle umgestellt werden
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

void nobMilitary::SetArmorAllowed(const bool enabled)
{
    if(armorDisabled == !enabled)
        return;

    armorDisabled = !enabled;
    if(GAMECLIENT.GetPlayerId() != player || GAMECLIENT.IsReplayModeOn())
        armorDisabledVirtual = armorDisabled;

    if(!armorDisabled)
        SearchArmor();
    else
    {
        // send armor back if just deactivated
        for(auto it = ordered_armor.begin(); it != ordered_armor.end();)
        {
            // But only those, that are not just Being carried in
            if((*it)->GetLocation() != this)
            {
                WareNotNeeded(*it);
                it = ordered_armor.erase(it);
            } else
                ++it;
        }
    }
}

unsigned nobMilitary::CalcCoinsPoints() const
{
    // Will ich überhaupt Goldmünzen, wenn nich, sofort raus
    if(!WantCoins())
        return 0;

    // 10000 als Basis wählen, damit man auch noch was abziehen kann
    int points = 10000;

    // Wenn hier schon Münzen drin sind oder welche bestellt sind, wirkt sich das natürlich negativ auf die
    // "Wichtigkeit" aus
    points -= (numCoins + ordered_coins.size()) * 30;

    const unsigned maxRank = world->GetGGS().GetMaxMilitaryRank();
    // Beförderbare Soldaten zählen
    for(const auto& soldier : troops)
    {
        // Solange es kein Max Rank ist, kann der Soldat noch befördert werden
        if(soldier->GetRank() < maxRank)
            points += 20;
    }

    if(points < 0)
        throw std::logic_error("Negative points are not allowed");

    return static_cast<unsigned>(points);
}

unsigned nobMilitary::CalcArmorPoints() const
{
    if(!WantArmor())
        return 0;

    // choose 10000 as basic, so we can stil subtract something
    int points = 10000;

    // If we have already armor in house or ordered, our request is less important
    points -= (numArmor + ordered_armor.size()) * 30;

    auto numberOfUpgradeSoldier =
      helpers::count_if(troops, [](OwnedSortedTroops::value_type const& troop) { return !troop->HasArmor(); });

    points += static_cast<int>(numberOfUpgradeSoldier * 20);

    if(points < 0)
        throw std::logic_error("Negative points are not allowed");

    return static_cast<unsigned>(points);
}

bool nobMilitary::WantCoins() const
{
    // Wenn die Goldzufuhr gestoppt wurde oder Münzvorrat voll ist, will ich gar keine Goldmünzen
    return (!coinsDisabled && numCoins + ordered_coins.size() != GetMaxCoinCt() && !new_built);
}

void nobMilitary::SearchCoins()
{
    // Brauche ich überhaupt Goldmünzen bzw. hab ich vielleicht schon ein Event angemeldet?
    if(WantCoins() && !goldorder_event)
    {
        // Lagerhaus mit Goldmünzen suchen
        nobBaseWarehouse* wh =
          world->GetPlayer(player).FindWarehouse(*this, FW::HasMinWares(GoodType::Coins), false, false);
        if(wh)
        {
            // Wenns eins gibt, dort eine Goldmünze bestellen
            Ware* ware = wh->OrderWare(GoodType::Coins, *this);

            if(!ware)
            {
                RTTR_Assert(false);
                // Ware dürfte nicht 0 werden, da ja ein Lagerhaus MIT GOLDMÜNZEN bereits gesucht wird
                LOG.write("nobMilitary::SearchCoins: WARNING: ware = nullptr. Bug alarm!\n");
                return;
            }

            RTTR_Assert(helpers::contains(ordered_coins, ware));

            // Nach einer Weile nochmal nach evtl neuen Goldmünzen gucken
            goldorder_event = GetEvMgr().AddEvent(this, 200 + RANDOM_RAND(400), 1);
        }
    }
}

bool nobMilitary::WantArmor() const
{
    // If armor delivery stopped or stock already full, we do not want any armor
    return (!armorDisabled && numArmor + ordered_armor.size() != GetMaxArmorCt() && !new_built);
}

void nobMilitary::SearchArmor()
{
    if(!leatheraddon::isAddonActive(world->GetPlayer(player).GetGameWorld()))
        return;

    if(WantArmor() && !armororder_event)
    {
        nobBaseWarehouse* wh =
          world->GetPlayer(player).FindWarehouse(*this, FW::HasMinWares(GoodType::Armor), false, false);
        if(wh)
        {
            Ware* ware = wh->OrderWare(GoodType::Armor, this);

            if(!ware)
            {
                RTTR_Assert(false);
                LOG.write("nobMilitary::SearchArmor: WARNING: ware = nullptr. Bug alarm!\n");
                return;
            }

            RTTR_Assert(helpers::contains(ordered_armor, ware));

            // After some time, try to order new armor
            armororder_event = GetEvMgr().AddEvent(this, 200 + RANDOM_RAND(400), 3);
        }
    }
}

void nobMilitary::PrepareUpgrading()
{
    // Goldmünzen da?
    if(!numCoins)
        return;

    // Gibts auch noch kein Beförderungsevent?
    if(upgrade_event)
        return;

    // Noch Soldaten, die befördert werden können?
    bool soldiers_available = false;

    const unsigned maxRank = world->GetGGS().GetMaxMilitaryRank();
    for(auto& troop : troops)
    {
        if(troop->GetRank() < maxRank)
        {
            // es wurde ein Soldat gefunden, der befördert werden kann
            soldiers_available = true;
            break;
        }
    }

    if(!soldiers_available)
        return;

    // Alles da --> Beförderungsevent anmelden
    upgrade_event = GetEvMgr().AddEvent(this, UPGRADE_TIME + RANDOM_RAND(UPGRADE_TIME_RANDOM), 2);
}

void nobMilitary::PrepareArmorUpgrading()
{
    if(!numArmor)
        return;

    if(armor_upgrade_event)
        return;

    if(std::none_of(troops.begin(), troops.end(),
                    [](OwnedSortedTroops::value_type& troop) { return !troop->HasArmor(); }))
        return;

    // All here --> Trigger armor upgrade event
    armor_upgrade_event = GetEvMgr().AddEvent(this, UPGRADE_TIME + RANDOM_RAND(UPGRADE_TIME_RANDOM), 4);
}

void nobMilitary::HitOfCatapultStone()
{
    // Ein Soldat weniger, falls es noch welche gibt
    if(!troops.empty())
    {
        // 30 percent chance to not die if the soldier has an armor
        bool die = !(*troops.begin())->HasArmor() || (RANDOM_RAND(99) < 70);
        if(die)
        {
            std::unique_ptr<nofPassiveSoldier> soldier = std::move(*troops.begin());
            helpers::pop_front(troops);
            // Shortcut for Die(): No need to remove from world as it is inside and we can delete it right away
            soldier->RemoveFromInventory();
            soldier->LeftBuilding();
            soldier->Destroy();
        } else
        {
            std::unique_ptr<nofPassiveSoldier> soldier = std::move(*troops.begin());
            helpers::pop_front(troops);
            soldier->SetArmor(false);
            world->GetPlayer(player).DecreaseInventoryJob(figureToAmoredSoldierEnum(soldier.get()), 1);
            troops.insert(std::move(soldier));
        }
    }

    // If there are troops left, order some more, else this will be destroyed
    if(!troops.empty())
        RegulateTroops();

    // Post verschicken
    SendPostMessage(player,
                    std::make_unique<PostMsgWithBuilding>(GetEvMgr().GetCurrentGF(), _("A catapult is firing upon us!"),
                                                          PostCategory::Military, *this));
}

/**
 *  Darf das Militärgebäude abgerissen werden (Abriss-Verbot berücksichtigen)?
 */
bool nobMilitary::IsDemolitionAllowed() const
{
    switch(world->GetGGS().getSelection(AddonId::DEMOLITION_PROHIBITION))
    {
        default: // off
            break;
        case 1: // under attack
        {
            // Prüfen, ob das Gebäude angegriffen wird
            if(!aggressors.empty())
                return false;
        }
        break;
        case 2: // near frontiers
        {
            // Prüfen, ob es in Grenznähe steht
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
        // Einnahme beendet
        capturing = false;
        // Nun die Besetzung prüfen
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
