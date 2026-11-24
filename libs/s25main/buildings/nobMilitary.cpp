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
#include "gameData/MilitaryConsts.h"
#include "gameData/SettingTypeConv.h"
#include "s25util/Log.h"
#include <algorithm>
#include <limits>
#include <stdexcept>

nobMilitary::nobMilitary(const BuildingType type, const MapPoint pos, const unsigned char player, const Nation nation)
    : nobBaseMilitary(type, pos, player, nation), new_built(true), numCoins(0), coinsDisabled(false),
      coinsDisabledVirtual(false), numArmor(0), armorAllowed(true), armorAllowedVirtual(true), capturing(false),
      capturing_soldiers(0), gold_order_event(nullptr), armor_order_event(nullptr), upgrade_event(nullptr),
      armor_upgrade_event(nullptr), is_regulating_troops(false)
{
    // Gebäude entsprechend als Militärgebäude registrieren und in ein Militärquadrat eintragen
    world->getMilitarySquares().add(this);

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
    troop_limits.fill(getMaxTroopsCt());

    // Tür aufmachen, bis Gebäude besetzt ist
    openDoor();

    // Wenn kein Gold in neu gebaute Militärgebäude eingeliefert werden soll, wird die Goldzufuhr gestoppt
    if(world->getGGS().isEnabled(AddonId::NO_COINS_DEFAULT))
    {
        coinsDisabled = true;
        coinsDisabledVirtual = true;
    }

    if(world->getGGS().isEnabled(AddonId::NO_ARMOR_DEFAULT))
    {
        armorAllowed = false;
        armorAllowedVirtual = false;
    }
}

nobMilitary::~nobMilitary() = default;

size_t nobMilitary::getTotalSoldiers() const
{
    size_t sum = troops.size() + ordered_troops.size() + troops_on_mission.size();
    if(defender_)
        sum++;
    sum += /* capturing_soldiers*/ +far_away_capturers.size();
    return sum;
}

std::array<unsigned, NUM_SOLDIER_RANKS> nobMilitary::getTotalSoldiersByRank() const
{
    std::array<unsigned, NUM_SOLDIER_RANKS> counts = {0};
    for(const auto& troop : troops)
        ++counts[troop->getRank()];
    for(const auto& troop : ordered_troops)
        ++counts[troop->getRank()];
    for(const auto& troop : troops_on_mission)
        ++counts[troop->getRank()];
    if(defender_)
        ++counts[defender_->getRank()];
    for(const auto& troop : far_away_capturers)
        ++counts[troop->getRank()];
    return counts;
}

void nobMilitary::DestroyBuilding()
{
    // Remove from military square and buildings first, to avoid e.g. sending canceled soldiers back to this building
    world->getMilitarySquares().remove(this);

    // Bestellungen stornieren
    cancelOrders();

    // Soldaten rausschicken
    for(auto& troop : troops)
    {
        troop->leftBuilding();
        auto& soldier = world->addFigure(pos, std::move(troop));
        soldier.startWandering();
    }
    troops.clear();

    // Inform far-away capturers
    for(auto* farAwayCapturer : far_away_capturers)
        farAwayCapturer->attackedGoalDestroyed();
    far_away_capturers.clear();

    // Events ggf. entfernen
    getEvMgr().removeEvent(gold_order_event);
    getEvMgr().removeEvent(armor_order_event);
    getEvMgr().removeEvent(upgrade_event);
    getEvMgr().removeEvent(armor_upgrade_event);

    // übriggebliebene Goldmünzen in der Inventur abmelden
    world->getPlayer(player).decreaseInventoryWare(GoodType::Coins, numCoins);
    world->getPlayer(player).decreaseInventoryWare(GoodType::Armor, numArmor);

    nobBaseMilitary::DestroyBuilding();
    // If this was occupied, recalc territory. AFTER calling base destroy as otherwise figures might get stuck here
    if(!new_built)
        world->recalcTerritory(*this, TerritoryChangeReason::Destroyed);

    world->getNotifications().publish(BuildingNote(BuildingNote::Lost, player, pos, bldType_));
}

void nobMilitary::serialize(SerializedGameData& sgd) const
{
    nobBaseMilitary::serialize(sgd);
    sgd.pushBool(new_built);
    sgd.pushUnsignedChar(numCoins);
    sgd.pushBool(coinsDisabled);
    sgd.pushEnum<uint8_t>(frontier_distance);
    sgd.pushUnsignedChar(size);
    sgd.pushBool(capturing);
    sgd.pushUnsignedInt(capturing_soldiers);
    sgd.pushEvent(gold_order_event);
    sgd.pushEvent(upgrade_event);

    sgd.pushObjectContainer(ordered_troops, true);
    sgd.pushObjectContainer(ordered_coins, true);
    sgd.pushObjectContainer(troops, true);
    sgd.pushObjectContainer(far_away_capturers, true);
    helpers::pushContainer(sgd, troop_limits);

    sgd.pushUnsignedChar(numArmor);
    sgd.pushBool(armorAllowed);
    sgd.pushEvent(armor_order_event);
    sgd.pushEvent(armor_upgrade_event);
    sgd.pushObjectContainer(ordered_armor, true);
}

nobMilitary::nobMilitary(SerializedGameData& sgd, const unsigned obj_id)
    : nobBaseMilitary(sgd, obj_id), new_built(sgd.popBool()), numCoins(sgd.popUnsignedChar()),
      coinsDisabled(sgd.popBool()), coinsDisabledVirtual(coinsDisabled), frontier_distance(sgd.pop<FrontierDistance>()),
      size(sgd.popUnsignedChar()), capturing(sgd.popBool()), capturing_soldiers(sgd.popUnsignedInt()),
      gold_order_event(sgd.popEvent()), upgrade_event(sgd.popEvent()), is_regulating_troops(false)
{
    sgd.popObjectContainer(ordered_troops, GO_Type::NofPassivesoldier);
    sgd.popObjectContainer(ordered_coins, GO_Type::Ware);
    sgd.popObjectContainer(troops, GO_Type::NofPassivesoldier);
    sgd.popObjectContainer(far_away_capturers, GO_Type::NofAttacker);

    if(sgd.getGameDataVersion() < 10)
        troop_limits.fill(getMaxTroopsCt());
    else
        helpers::popContainer(sgd, troop_limits);

    if(sgd.getGameDataVersion() >= 12)
    {
        numArmor = sgd.popUnsignedChar();
        armorAllowedVirtual = armorAllowed = sgd.popBool();
        armor_order_event = sgd.popEvent();
        armor_upgrade_event = sgd.popEvent();
        sgd.popObjectContainer(ordered_armor, GO_Type::Ware);
    } else
    {
        numArmor = 0;
        armorAllowedVirtual = armorAllowed = true;
        armor_order_event = nullptr;
        armor_upgrade_event = nullptr;
    }

    // ins Militärquadrat einfügen
    world->getMilitarySquares().add(this);

    if(capturing && capturing_soldiers == 0 && aggressors.empty())
    {
        LOG.write(
          "Bug in savegame detected: Building at (%d,%d) Being captured has no capturers. Trying to fix this...\n")
          % pos.x % pos.y;
        capturing = false;
    }
}

void nobMilitary::draw(DrawPoint drawPt)
{
    // Gebäude an sich zeichnen
    drawBaseBuilding(drawPt);

    // (max 4) Besatzungs-Fähnchen zeichnen
    auto flags = std::min<unsigned>(troops.size() + this->leave_house.size(), 4);

    for(unsigned i = 0; i < flags; ++i)
    {
        const unsigned flagTexture = 3162 + GAMECLIENT.getGlobalAnimation(8, 2, 1, pos.x * pos.y * i);
        LOADER.getMapPlayerImage(flagTexture)
          ->drawForPlayer(drawPt + TROOPS_FLAG_OFFSET[nation][size] + DrawPoint(0, i * 3),
                          world->getPlayer(player).color);
    }

    // Die Fahne, die anzeigt wie weit das Gebäude von der Grenze entfernt ist, zeichnen
    FrontierDistance frontier_distance_tmp = frontier_distance;
    ITexture* bitmap = nullptr;
    unsigned animationFrame = GAMECLIENT.getGlobalAnimation(4, 1, 1, pos.x * pos.y * getObjId());
    if(new_built)
    {
        // don't draw a flag for new buildings - fixes bug #215
    } else if(frontier_distance_tmp == FrontierDistance::Harbor)
    {
        // todo Hafenflagge
        bitmap = LOADER.getTextureN("map_new", 3150 + animationFrame);
    } else
    {
        if(frontier_distance_tmp == FrontierDistance::Near)
            frontier_distance_tmp = FrontierDistance::Harbor;
        bitmap = LOADER.getMapTexture(3150 + rttr::enumCast(frontier_distance_tmp) * 4 + animationFrame);
    }
    if(bitmap)
        bitmap->drawFull(drawPt + BORDER_FLAG_OFFSET[nation][size]);

    // If coins or armor delivery disabled, show sign at building
    if(leatheraddon::isAddonActive(world->getPlayer(player).getGameWorld()))
    {
        if(coinsDisabledVirtual)
        {
            LOADER.getImageN("leather_bobs", leatheraddon::bobIndex[leatheraddon::BobType::StopCoinsXSignOverride])
              ->drawFull(drawPt + BUILDING_SIGN_CONSTS[nation][bldType_]);
        }

        if(!armorAllowedVirtual)
        {
            LOADER.getImageN("leather_bobs", leatheraddon::bobIndex[leatheraddon::BobType::StopArmorXSign])
              ->drawFull(drawPt + BUILDING_ARMOR_SIGN_CONSTS[nation][bldType_]);
        }
    } else // Without leather addon use original graphic
    {
        if(coinsDisabledVirtual)
            LOADER.getMapTexture(46)->drawFull(drawPt + BUILDING_SIGN_CONSTS[nation][bldType_]);
    }
}

void nobMilitary::handleEvent(const unsigned id)
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
                noFigure& soldier = world->addFigure(pos, std::move(leave_house.front()));
                leave_house.pop_front();
                figureLeft(soldier);
                soldier.actAtFirst();
            }

            // Wenn noch weitere drin sind, die müssen auch noch raus
            if(!leave_house.empty())
                leaving_event = getEvMgr().addEvent(this, 30 + RANDOM_RAND(10));
            else
                go_out = false;

            regulateTroops();
        }
        break;
        // Goldbestell-Event
        case 1:
        {
            gold_order_event = nullptr;

            // ggf. nach neuen Goldmünzen suchen
            searchCoins();
        }
        break;
        // Beförderungs-Event
        case 2:
        {
            upgrade_event = nullptr;

            std::vector<std::unique_ptr<nofPassiveSoldier>> soldiersToUpgrade;
            const uint8_t maxRank = world->getGGS().getMaxMilitaryRank();

            if(world->getGGS().isEnabled(AddonId::SINGLE_SOLDIER_COIN_TRAINING))
            {
                auto it = std::find_if(troops.begin(), troops.end(),
                                       [maxRank](const auto& soldier) { return soldier->getRank() < maxRank; });
                if(it != troops.end())
                {
                    soldiersToUpgrade.push_back(std::move(*it));
                    troops.erase(it);
                }
            } else
            {
                // Soldaten befördern
                // Von hinten durchgehen
                // Wenn der nachfolgende (schwächere) Soldat einen niedrigeren Rang hat,
                // wird dieser ebenfalls befördert usw.!
                uint8_t lastRank = maxRank;
                for(auto it = troops.rbegin(); it != troops.rend();)
                {
                    auto& soldier = *it;
                    // Es wurde schon einer befördert, dieser Soldat muss nun einen niedrigeren Rang
                    // als der letzte haben, damit er auch noch befördert werden kann
                    if(soldier->getRank() < lastRank)
                    {
                        // Rang merken
                        lastRank = soldier->getRank();
                        // Remove from sorted container as changing it breaks sorting
                        soldiersToUpgrade.push_back(std::move(soldier));
                        it = helpers::eraseReverse(troops, it);
                    } else
                        ++it;
                }
            }

            // Wurde jemand befördert?
            if(!soldiersToUpgrade.empty())
            {
                // Reinsert upgraded soldiers
                for(auto& upgradedSoldier : soldiersToUpgrade)
                {
                    upgradedSoldier->upgrade();
                    troops.insert(std::move(upgradedSoldier));
                }

                // Goldmünze verbrauchen
                --numCoins;
                world->getPlayer(player).decreaseInventoryWare(GoodType::Coins, 1);

                regulateTroops();

                // Evtl neues Beförderungsevent anmelden
                prepareUpgrading();

                // Ggf. neue Goldmünzen bestellen
                searchCoins();
            }
        }
        break;
        // Order armor event
        case 3:
        {
            armor_order_event = nullptr;
            searchArmor();
        }
        break;
        // Armor upgrade event
        case 4:
        {
            armor_upgrade_event = nullptr;

            auto candidate = std::find_if(troops.rbegin(), troops.rend(),
                                          [](OwnedSortedTroops::value_type& troop) { return !troop->hasArmor(); });

            if(candidate != troops.rend())
            {
                auto& soldier = *candidate;
                soldier->setArmor(true);

                --numArmor;
                world->getPlayer(player).decreaseInventoryWare(GoodType::Armor, 1);
                world->getPlayer(player).increaseInventoryJob(jobEnumToAmoredSoldierEnum(soldier->getJobType()), 1);

                prepareArmorUpgrading();
                searchArmor();
            }
        }
        break;
    }
}

unsigned nobMilitary::getMilitaryRadius() const
{
    return MILITARY_RADIUS[size];
}

unsigned nobMilitary::getMaxCoinCt() const
{
    return NUM_GOLDS[nation][size];
}

unsigned nobMilitary::getMaxArmorCt() const
{
    return NUM_ARMOR[nation][size];
}

unsigned nobMilitary::getMaxTroopsCt() const
{
    return NUM_TROOPS[nation][size];
}

void nobMilitary::lookForEnemyBuildings(const nobBaseMilitary* const exception)
{
    // Umgebung nach Militärgebäuden absuchen
    sortedMilitaryBlds buildings = world->lookForMilitaryBuildings(pos, 3);
    frontier_distance = FrontierDistance::Far;

    const bool frontierDistanceCheck = world->getGGS().isEnabled(AddonId::FRONTIER_DISTANCE_REACHABLE);

    for(auto* building : buildings)
    {
        // feindliches Militärgebäude?
        if(building != exception && building->getPlayer() != player
           && world->getPlayer(building->getPlayer()).isAttackable(player))
        {
            unsigned distance = world->calcDistance(pos, building->getPos());
            FrontierDistance newFrontierDistance = FrontierDistance::Far;

            if(distance <= getMilitaryRadius() + building->getMilitaryRadius())
            {
                newFrontierDistance = FrontierDistance::Near;
            }
            // in mittlerem Umkreis, also theoretisch angreifbar?
            else if(distance < BASE_ATTACKING_DISTANCE + (getMaxTroopsCt() - 1) * EXTENDED_ATTACKING_DISTANCE)
            {
                newFrontierDistance = FrontierDistance::Mid;
            } else if(building->getGOT() == GO_Type::NobMilitary)
            {
                auto* mil = static_cast<nobMilitary*>(building);
                if(distance < BASE_ATTACKING_DISTANCE + (mil->getMaxTroopsCt() - 1) * EXTENDED_ATTACKING_DISTANCE)
                {
                    newFrontierDistance = FrontierDistance::Mid;
                }
            }

            // if new frontier distance is in military range, check if its reachable.
            if(frontierDistanceCheck && newFrontierDistance >= FrontierDistance::Mid
               && !doesReachablePathExist(*world, building->getPos(), pos, MAX_ATTACKING_RUN_DISTANCE))
            {
                // building is not reachable, so its "far" away.
                newFrontierDistance = FrontierDistance::Far;
            }

            // override own frontier distance, if its nearer to a border
            if(newFrontierDistance > frontier_distance)
                frontier_distance = newFrontierDistance;

            // set calculated frontier distance to checked building.
            if(BuildingProperties::isMilitary(building->getBuildingType()))
                static_cast<nobMilitary*>(building)->newEnemyMilitaryBuilding(newFrontierDistance);
        }
    }
    // check for harbor points
    if(frontier_distance <= FrontierDistance::Mid && world->getGGS().isEnabled(AddonId::SEA_ATTACK)
       && world->calcDistanceToNearestHarbor(pos) < SEAATTACK_DISTANCE + 2)
        frontier_distance = FrontierDistance::Harbor;

    // send troops
    regulateTroops();
}

void nobMilitary::newEnemyMilitaryBuilding(const FrontierDistance distance)
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
    regulateTroops();
}

void nobMilitary::regulateTroops()
{
    RTTR_Assert(helpers::contains(world->getPlayer(player).getBuildingRegister().getMilitaryBuildings(),
                                  this)); // If this fails, the building is Being destroyed!

    // Wenn das Gebäude eingenommen wird, erstmal keine neuen Truppen und warten, wieviele noch reinkommen
    if(isBeingCaptured())
        return;

    // Already regulate its troops => Don't call this method again
    if(is_regulating_troops)
        return;

    is_regulating_troops = true;

    GamePlayer& owner = world->getPlayer(player);
    std::array<int, NUM_SOLDIER_RANKS> hasReturnWarehouseByRank;
    hasReturnWarehouseByRank.fill(-1);
    auto canReturnHome = [&](const nofPassiveSoldier& soldier) {
        const unsigned rank = soldier.getRank();
        int& hasReturnWarehouse = hasReturnWarehouseByRank[rank];
        if(hasReturnWarehouse < 0)
        {
            hasReturnWarehouse =
              owner.findWarehouse(*this, FW::AcceptsFigure(soldier.getJobType()), true, false) ? 1 : 0;
        }
        return hasReturnWarehouse != 0;
    };

    // This only has an effect if the military control addon is in use and the troop limits have been lowered.
    std::array<unsigned, NUM_SOLDIER_RANKS> counts = getTotalSoldiersByRank();
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
                if((*it)->getRank() == rank && canReturnHome(**it))
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
                notNeededSoldier->notNeeded();
        } else
        {
            // This bit is for ordering troops later
            lack[rank] = troop_limits[rank] - counts[rank];
        }
        if(excess > 0)
        {
            for(auto it = troops.begin(); excess && it != troops.end() && troops.size() > 1;)
            {
                if((*it)->getRank() == rank && canReturnHome(**it))
                {
                    (*it)->leaveBuilding();
                    addLeavingFigure(std::move(*it));
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
    int diff = static_cast<int>(calcRequiredNumTroops()) - static_cast<int>(getTotalSoldiers());
    if(diff < 0)
    {
        // Zu viel --> überflüssige Truppen nach Hause schicken
        // Zuerst die bestellten Soldaten wegschicken
        // Weak ones first
        std::vector<nofPassiveSoldier*> notNeededSoldiers;
        if(owner.getMilitarySetting(1) > MILITARY_SETTINGS_SCALE[1] / 2)
        {
            for(auto it = ordered_troops.begin(); diff && it != ordered_troops.end();)
            {
                if(canReturnHome(**it))
                {
                    notNeededSoldiers.push_back(*it);
                    it = ordered_troops.erase(it);
                    ++diff;
                } else
                {
                    ++it;
                }
            }
        }
        // Strong ones first
        else
        {
            for(auto it = ordered_troops.rbegin(); diff && it != ordered_troops.rend();)
            {
                if(canReturnHome(**it))
                {
                    notNeededSoldiers.push_back(*it);
                    it = helpers::eraseReverse(ordered_troops, it);
                    ++diff;
                } else
                {
                    ++it;
                }
            }
        }

        // send the not-needed-soldiers away
        for(auto* notNeededSoldier : notNeededSoldiers)
        {
            notNeededSoldier->notNeeded();
        }

        // Dann den Rest (einer muss immer noch drinbleiben!)
        // erst die schwachen Soldaten raus
        if(owner.getMilitarySetting(1) > MILITARY_SETTINGS_SCALE[1] / 2)
        {
            for(auto it = troops.begin(); diff && it != troops.end() && troops.size() > 1;)
            {
                if(canReturnHome(**it))
                {
                    (*it)->leaveBuilding();
                    addLeavingFigure(std::move(*it));
                    it = troops.erase(it);
                    ++diff;
                } else
                {
                    ++it;
                }
            }
        }
        // erst die starken Soldaten raus
        else
        {
            for(auto it = troops.rbegin(); diff && it != troops.rend() && troops.size() > 1;)
            {
                if(canReturnHome(**it))
                {
                    (*it)->leaveBuilding();
                    addLeavingFigure(std::move(*it));
                    it = helpers::eraseReverse(troops, it);
                    ++diff;
                } else
                {
                    ++it;
                }
            }
        }
    } else if(diff > 0)
    {
        // Zu wenig Truppen

        // Gebäude wird angegriffen und
        // Addon aktiv, nur soviele Leute zum Nachbesetzen schicken wie Verteidiger eingestellt
        if(isUnderAttack() && world->getGGS().getSelection(AddonId::DEFENDER_BEHAVIOR) == 2)
        {
            diff = (world->getPlayer(player).getMilitarySetting(2) * diff) / MILITARY_SETTINGS_SCALE[2];
        }
        // only order new troops if there is a chance that there is a path - pathfinding from each warehouse with
        // soldiers to this mil building will start at the warehouse and cost time
        bool mightHaveRoad = false;
        for(const auto dir : helpers::enumRange<Direction>())
        {
            // every direction but 1 because 1 is the building connection so it doesn't count for this check
            if(dir == Direction::NorthWest)
                continue;
            if(getFlag()->getRoute(dir))
            {
                mightHaveRoad = true;
                break;
            }
        }
        if(mightHaveRoad)
            world->getPlayer(player).orderTroops(*this, lack, diff);
    }

    is_regulating_troops = false;
}

unsigned nobMilitary::calcRequiredNumTroops() const
{
    return calcRequiredNumTroops(frontier_distance,
                                 world->getPlayer(player).getMilitarySetting(4 + rttr::enumCast(frontier_distance)));
}

unsigned nobMilitary::calcRequiredNumTroops(FrontierDistance assumedFrontierDistance, unsigned settingValue) const
{
    return (getMaxTroopsCt() - 1) * settingValue / MILITARY_SETTINGS_SCALE[4 + rttr::enumCast(assumedFrontierDistance)]
           + 1;
}

void nobMilitary::SetTroopLimit(const unsigned rank, const unsigned limit)
{
    RTTR_Assert(rank < troop_limits.size());
    troop_limits[rank] = limit;
    regulateTroops();
}

bool nobMilitary::isUseless() const
{
    if(frontier_distance != FrontierDistance::Far || new_built)
        return false;
    return !world->doesDestructionChangeTerritory(*this);
}

bool nobMilitary::isAttackable(unsigned playerIdx) const
{
    // Cannot be attacked, if it is Being captured or not claimed yet (just built)
    return nobBaseMilitary::isAttackable(playerIdx) && !isBeingCaptured() && !isNewBuilt();
}

bool nobMilitary::isInTroops(const nofPassiveSoldier& soldier) const
{
    return helpers::containsPtr(troops, &soldier);
}

void nobMilitary::takeWare(Ware* ware)
{
    // Goldmünze in Bestellliste aufnehmen
    if(ware->type == GoodType::Coins)
    {
        RTTR_Assert(!helpers::contains(ordered_coins, ware));
        ordered_coins.push_back(ware);
    } else
    {
        RTTR_Assert(ware->type == GoodType::Armor);
        RTTR_Assert(!helpers::contains(ordered_armor, ware));
        ordered_armor.push_back(ware);
    }
}

void nobMilitary::addWare(std::unique_ptr<Ware> ware)
{
    if(ware->type == GoodType::Coins)
    {
        // Ein Golstück mehr
        ++numCoins;
        // aus der Bestellliste raushaun
        RTTR_Assert(helpers::contains(ordered_coins, ware.get()));
        ordered_coins.remove(ware.get());

        // Ware vernichten
        world->getPlayer(player).removeWare(*ware);
        ware.reset();

        // Evtl. Soldaten befördern
        prepareUpgrading();
    } else
    {
        ++numArmor;
        RTTR_Assert(helpers::contains(ordered_armor, ware.get()));
        ordered_armor.remove(ware.get());

        world->getPlayer(player).removeWare(*ware);
        ware.reset();

        prepareArmorUpgrading();
    }
}

void nobMilitary::wareLost(Ware& ware)
{
    if(ware.type == GoodType::Coins)
    {
        // Ein Goldstück konnte nicht kommen --> aus der Bestellliste entfernen
        RTTR_Assert(helpers::contains(ordered_coins, &ware));
        ordered_coins.remove(&ware);
    } else
    {
        RTTR_Assert(helpers::contains(ordered_armor, &ware));
        ordered_armor.remove(&ware);
    }
}

bool nobMilitary::freePlaceAtFlag()
{
    return false;
}
void nobMilitary::gotWorker(Job /*job*/, noFigure& worker)
{
    RTTR_Assert(dynamic_cast<nofPassiveSoldier*>(&worker));
    auto& soldier = static_cast<nofPassiveSoldier&>(worker);
    RTTR_Assert(soldier.getPlayer() == player);
    ordered_troops.insert(&soldier);
}

void nobMilitary::cancelOrders()
{
    // Soldaten zurückschicken
    for(auto* orderedTroop : ordered_troops)
        orderedTroop->notNeeded();

    ordered_troops.clear();

    // Goldmünzen zurückschicken
    for(auto* orderedCoin : ordered_coins)
        wareNotNeeded(orderedCoin);

    ordered_coins.clear();

    for(auto* ordered_armor : ordered_armor)
        wareNotNeeded(ordered_armor);

    ordered_armor.clear();
}

void nobMilitary::addActiveSoldier(std::unique_ptr<nofActiveSoldier> soldier)
{
    // aktiver Soldat, eingetroffen werden --> dieser muss erst in einen passiven Soldaten
    // umoperiert werden (neu erzeugt und alter zerstört) werden
    addPassiveSoldier(std::make_unique<nofPassiveSoldier>(*soldier));

    soldier->resetHome();

    RTTR_Assert(soldier->getPlayer() == player);

    // Returned home
    if(soldier.get() == defender_)
        resetDefender();
    else if(helpers::contains(troops_on_mission, soldier.get()))
    {
        troops_on_mission.remove(soldier.get());
    } else
    {
        RTTR_Assert(dynamic_cast<nofAttacker*>(soldier.get()));
        if(isBeingCaptured() || isFarAwayCapturer(static_cast<const nofAttacker&>(*soldier)))
        {
            getEvMgr().addToKillList(std::move(soldier));
            return;
        }
    }
    getEvMgr().addToKillList(std::move(soldier));
    // Do only if not capturing
    regulateTroops();
}

void nobMilitary::addPassiveSoldier(std::unique_ptr<nofPassiveSoldier> soldier)
{
    RTTR_Assert(soldier->getPlayer() == player);
    RTTR_Assert(troops.size() < getMaxTroopsCt());

    ordered_troops.erase(soldier.get());
    troops.insert(std::move(soldier));

    // Wurde dieses Gebäude zum ersten Mal besetzt?
    if(new_built)
    {
        sendPostMessage(
          player, std::make_unique<PostMsgWithBuilding>(getEvMgr().getCurrentGF(), _("Military building occupied"),
                                                        PostCategory::Military, *this, SoundEffect::Fanfare));
        // Ist nun besetzt
        new_built = false;
        // Landgrenzen verschieben
        world->recalcTerritory(*this, TerritoryChangeReason::Build);
        // Tür zumachen
        closeDoor();
        world->getNotifications().publish(BuildingNote(BuildingNote::Captured, player, pos, bldType_));
    } else
    {
        // Evtl. Soldaten befördern
        prepareUpgrading();
        prepareArmorUpgrading();
    }

    // Goldmünzen suchen, evtl sinds ja neue Soldaten
    searchCoins();
    searchArmor();
}

void nobMilitary::soldierLost(nofSoldier* soldier)
{
    // Soldat konnte nicht (mehr) kommen --> rauswerfen und ggf. neue Soldaten rufen
    if(soldier->getGOT() == GO_Type::NofPassivesoldier)
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
    regulateTroops();
}

void nobMilitary::sendAttacker(nofPassiveSoldier*& passiveSoldier, nobBaseMilitary& goal,
                               const nobHarborBuilding* harbor)
{
    auto attacker = std::make_unique<nofAttacker>(*passiveSoldier, goal, harbor);
    passiveSoldier->leftBuilding();
    helpers::extractPtr(troops, passiveSoldier)->destroy();
    passiveSoldier = nullptr;
    troops_on_mission.push_back(attacker.get());
    addLeavingFigure(std::move(attacker));
}

nofPassiveSoldier* nobMilitary::chooseSoldier()
{
    if(troops.empty())
        return nullptr;

    std::array<nofPassiveSoldier*, 5> candidates = {nullptr, nullptr, nullptr, nullptr, nullptr}; // candidates per rank

    // how many ranks
    unsigned rankCount = 0;

    for(auto& troop : troops)
    {
        if(!candidates[troop->getRank()])
        {
            ++rankCount;
            candidates[troop->getRank()] = troop.get();
        }
    }

    // ID ausrechnen
    unsigned rank = ((rankCount - 1) * world->getPlayer(player).getMilitarySetting(1)) / MILITARY_SETTINGS_SCALE[1];

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

nofAggressiveDefender* nobMilitary::sendAggressiveDefender(nofAttacker& attacker)
{
    // Don't send last soldier
    if(getNumTroops() <= 1)
        return nullptr;
    nofPassiveSoldier* soldier = chooseSoldier();
    if(soldier)
    {
        // neuen aggressiven Verteidiger daraus erzeugen
        auto defender = std::make_unique<nofAggressiveDefender>(*soldier, attacker);
        soldier->leftBuilding();
        helpers::extractPtr(troops, soldier)->destroy();
        troops_on_mission.push_back(defender.get());
        nofAggressiveDefender* result = defender.get();
        addLeavingFigure(std::move(defender));
        return result;
    } else
        return nullptr;
}

/// Gibt die Anzahl der Soldaten zurück, die für einen Angriff auf ein bestimmtes Ziel zur Verfügung stehen
unsigned nobMilitary::getNumSoldiersForAttack(const MapPoint dest) const
{
    // Soldaten ausrechnen, wie viel man davon nehmen könnte, je nachdem wie viele in den
    // Militäreinstellungen zum Angriff eingestellt wurden

    unsigned short soldiers_count =
      (getNumTroops() > 1) ?
        ((getNumTroops() - 1) * world->getPlayer(getPlayer()).getMilitarySetting(3) / MILITARY_SETTINGS_SCALE[3]) :
        0;

    unsigned distance = world->calcDistance(pos, dest);

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
    if(soldiers_count && world->findHumanPath(pos, dest, MAX_ATTACKING_RUN_DISTANCE))
        // Soldaten davon nehmen
        return soldiers_count;
    else
        return 0;
}

/// Gibt die Soldaten zurück, die für einen Angriff auf ein bestimmtes Ziel zur Verfügung stehen
std::vector<nofPassiveSoldier*> nobMilitary::getSoldiersForAttack(const MapPoint dest) const
{
    std::vector<nofPassiveSoldier*> soldiers;
    unsigned soldiers_count = getNumSoldiersForAttack(dest);
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
unsigned nobMilitary::getSoldiersStrengthForAttack(const MapPoint dest, unsigned& soldiers_count) const
{
    unsigned strength = 0;

    soldiers_count = getNumSoldiersForAttack(dest);
    unsigned numRemainingSoldiers = soldiers_count;

    for(const auto& sld : helpers::reverse(troops))
    {
        if(numRemainingSoldiers--)
            strength += HITPOINTS[sld->getRank()];
        else
            break;
    }

    return strength;
}

/// Gibt die Stärke eines Militärgebäudes zurück
unsigned nobMilitary::getSoldiersStrength() const
{
    unsigned strength = 0;
    for(const auto& troop : troops)
        strength += HITPOINTS[troop->getRank()];

    return strength;
}

/// is there a max rank soldier in the building?
bool nobMilitary::hasMaxRankSoldier() const
{
    const unsigned maxRank = world->getGGS().getMaxMilitaryRank();
    return helpers::containsIf(helpers::reverse(troops),
                               [maxRank](const auto& it) { return it->getRank() >= maxRank; });
}

std::unique_ptr<nofDefender> nobMilitary::provideDefender(nofAttacker& attacker)
{
    nofPassiveSoldier* soldier = chooseSoldier();
    if(!soldier)
        return nullptr;

    auto defender = std::make_unique<nofDefender>(*soldier, attacker);

    auto oldSoldier = helpers::extractPtr(troops, soldier);
    oldSoldier->leftBuilding();
    oldSoldier->destroy();

    return defender;
}

void nobMilitary::capture(const unsigned char new_owner)
{
    RTTR_Assert(isBeingCaptured());

    // Goldmünzen in der Inventur vom alten Spieler abziehen und dem neuen hinzufügen
    world->getPlayer(player).decreaseInventoryWare(GoodType::Coins, numCoins);
    world->getPlayer(new_owner).increaseInventoryWare(GoodType::Coins, numCoins);

    world->getPlayer(player).decreaseInventoryWare(GoodType::Armor, numArmor);
    world->getPlayer(new_owner).increaseInventoryWare(GoodType::Armor, numArmor);

    // Reset desired troop setting
    troop_limits.fill(getMaxTroopsCt());

    // Soldaten, die auf Mission sind, Bescheid sagen
    for(auto* it : troops_on_mission)
        it->homeDestroyed();

    // Bestellungen die hierher unterwegs sind canceln
    cancelOrders();

    // Tell agressive defending soldiers and far away capturers that their goal is gone
    for(auto* soldier : aggressive_defenders)
        soldier->attackedGoalDestroyed();
    for(auto* soldier : far_away_capturers)
        soldier->attackedGoalDestroyed();

    troops_on_mission.clear();
    aggressive_defenders.clear();
    far_away_capturers.clear();

    // Alten Besitzer merken
    unsigned char old_player = player;
    world->getPlayer(old_player).removeBuilding(this, bldType_);
    // neuer Spieler
    player = new_owner;
    // In der Wirtschaftsverwaltung dieses Gebäude jetzt zum neuen Spieler zählen und beim alten raushauen
    world->getPlayer(new_owner).addBuilding(this, bldType_);

    // Flagge davor auch übernehmen
    getFlag()->capture(new_owner);

    // Territorium neu berechnen
    world->recalcTerritory(*this, TerritoryChangeReason::Captured);

    // Sichtbarkeiten berechnen für alten Spieler
    world->recalcVisibilitiesAroundPoint(pos, getMilitaryRadius() + VISUALRANGE_MILITARY + 1, old_player, nullptr);

    // Grenzflagge entsprechend neu setzen von den Feinden
    lookForEnemyBuildings();
    // und von den Verbündeten (da ja ein Feindgebäude weg ist)!
    sortedMilitaryBlds buildings = world->lookForMilitaryBuildings(pos, 4);
    for(auto* building : buildings)
    {
        // verbündetes Gebäude?
        if(world->getPlayer(building->getPlayer()).isAttackable(old_player)
           && BuildingProperties::isMilitary(building->getBuildingType()))
            // Grenzflaggen von dem neu berechnen
            static_cast<nobMilitary*>(building)->lookForEnemyBuildings();
    }

    // ehemalige Leute dieses Gebäudes nach Hause schicken, die ggf. grad auf dem Weg rein/raus waren
    std::array<MapPoint, 2> coords = {pos, world->getNeighbour(pos, Direction::SouthEast)};
    for(const auto& coord : coords)
    {
        for(noBase& baseFigure : world->getFigures(coord))
        {
            if(baseFigure.getType() == NodalObjectType::Figure)
            {
                auto& figure = static_cast<noFigure&>(baseFigure);
                if(figure.getCurrentRoad() == getRoute(Direction::SouthEast) && figure.getPlayer() != new_owner)
                {
                    figure.abrogate();
                    figure.startWandering();
                }
            }
        }
    }

    // Send all allied aggressors home (we own the building now!)
    for(auto it = aggressors.begin(); it != aggressors.end();)
    {
        nofAttacker* attacker = *it;
        // dont remove attackers owned by players not allied with the new owner!
        unsigned char attPlayer = attacker->getPlayer();
        if(attPlayer != player && !world->getPlayer(attPlayer).isAttackable(player))
        {
            it = aggressors.erase(it);
            attacker->capturedBuildingFull();
        } else
            ++it;
    }

    // Post verschicken, an den alten Besitzer und an den neuen Besitzer
    sendPostMessage(old_player,
                    std::make_unique<PostMsgWithBuilding>(getEvMgr().getCurrentGF(), _("Military building lost"),
                                                          PostCategory::Military, *this));
    sendPostMessage(player,
                    std::make_unique<PostMsgWithBuilding>(getEvMgr().getCurrentGF(), _("Military building captured"),
                                                          PostCategory::Military, *this));

    world->getNotifications().publish(BuildingNote(BuildingNote::Captured, player, pos, bldType_));
    world->getNotifications().publish(BuildingNote(BuildingNote::Lost, old_player, pos, bldType_));

    // Check if we need to change the coin order

    switch(world->getGGS().getSelection(AddonId::COINS_CAPTURED_BLD))
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

    switch(world->getGGS().getSelection(AddonId::ARMOR_CAPTURED_BLD))
    {
        case 1: // enable armor order
            armorAllowed = true;
            armorAllowedVirtual = true;
            break;
        case 2: // disable armor order
            armorAllowed = false;
            armorAllowed = false;
            break;
    }
}

void nobMilitary::needOccupyingTroops()
{
    RTTR_Assert(isBeingCaptured()); // Only valid during capturing
    // Check if we need more soldiers from the attacking soldiers
    // Choose the closest ones first to avoid having them walk a long way

    nofAttacker* bestAttacker = nullptr;
    unsigned bestRadius = std::numeric_limits<unsigned>::max();

    unsigned needed_soldiers = calcRequiredNumTroops();
    unsigned currentSoldiers = troops.size() + capturing_soldiers + troops_on_mission.size();

    if(needed_soldiers > currentSoldiers)
    {
        // Soldaten absuchen
        for(auto* aggressor : aggressors)
        {
            // Is the soldier standing around and owned by the player?
            if(!aggressor->isAttackerReady() || aggressor->getPlayer() != player)
                continue;
            // Näher als der bisher beste?
            if(aggressor->getRadius() >= bestRadius)
                continue;
            // Und kommt er überhaupt zur Flagge (könnte ja in der 2. Reihe stehen, sodass die vor ihm ihn den Weg
            // versperren)?
            if(world->findHumanPath(aggressor->getPos(), world->getNeighbour(pos, Direction::SouthEast), 10, false))
            {
                // Dann is das der bisher beste
                bestAttacker = aggressor;
                bestRadius = bestAttacker->getRadius();
            }
        }

        // Einen gefunden?
        if(bestAttacker)
        {
            // Dann soll der hingehen
            bestAttacker->captureBuilding();
            ++capturing_soldiers;
            // und raus hier
            return;
        }

        // If necessary look for further soldiers who are not standing around the building
        for(auto it = aggressors.begin();
            it != aggressors.end() && needed_soldiers > currentSoldiers + far_away_capturers.size();)
        {
            nofAttacker* attacker = *it;

            if(attacker->getPlayer() == player)
            {
                // Ask attacker if this is possible
                if(attacker->canStartFarAwayCapturing(*this))
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
        if(attacker->getState() != nofActiveSoldier::SoldierState::AttackingCapturingNext
           && !world->getPlayer(attacker->getPlayer()).isAttackable(player))
        {
            it = aggressors.erase(it);
            attacker->capturedBuildingFull();
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
    if(GAMECLIENT.getPlayerId() != player || GAMECLIENT.isReplayModeOn())
        coinsDisabledVirtual = coinsDisabled;

    if(!coinsDisabled)
        searchCoins(); // Order coins if we just enabled it
    else
    {
        // send coins back if just deactivated
        for(auto it = ordered_coins.begin(); it != ordered_coins.end();)
        {
            // But only those, that are not just Being carried in
            if((*it)->getLocation() != this)
            {
                wareNotNeeded(*it);
                it = ordered_coins.erase(it);
            } else
                ++it;
        }
    }
}

void nobMilitary::SetArmorAllowed(const bool enabled)
{
    if(armorAllowed == enabled)
        return;

    armorAllowed = enabled;
    if(GAMECLIENT.getPlayerId() != player || GAMECLIENT.isReplayModeOn())
        armorAllowedVirtual = armorAllowed;

    if(armorAllowed)
        searchArmor();
    else
    {
        // send armor back if just deactivated
        for(auto it = ordered_armor.begin(); it != ordered_armor.end();)
        {
            // But only those, that are not just Being carried in
            if((*it)->getLocation() != this)
            {
                wareNotNeeded(*it);
                it = ordered_armor.erase(it);
            } else
                ++it;
        }
    }
}

unsigned nobMilitary::calcCoinsPoints() const
{
    // Will ich überhaupt Goldmünzen, wenn nich, sofort raus
    if(!wantCoins())
        return 0;

    // 10000 als Basis wählen, damit man auch noch was abziehen kann
    int points = 10000;

    // Wenn hier schon Münzen drin sind oder welche bestellt sind, wirkt sich das natürlich negativ auf die
    // "Wichtigkeit" aus
    points -= (numCoins + ordered_coins.size()) * 30;

    const unsigned maxRank = world->getGGS().getMaxMilitaryRank();
    // Beförderbare Soldaten zählen
    for(const auto& soldier : troops)
    {
        // Solange es kein Max Rank ist, kann der Soldat noch befördert werden
        if(soldier->getRank() < maxRank)
            points += 20;
    }

    if(points < 0)
        throw std::logic_error("Negative points are not allowed");

    return static_cast<unsigned>(points);
}

unsigned nobMilitary::calcArmorPoints() const
{
    if(!wantArmor())
        return 0;

    // choose 10000 as basic, so we can stil subtract something
    int points = 10000;

    // If we have already armor in house or ordered, our request is less important
    points -= (numArmor + ordered_armor.size()) * 30;

    auto numberOfUpgradeSoldier =
      helpers::countIf(troops, [](OwnedSortedTroops::value_type const& troop) { return !troop->hasArmor(); });

    points += static_cast<int>(numberOfUpgradeSoldier * 20);

    if(points < 0)
        throw std::logic_error("Negative points are not allowed");

    return static_cast<unsigned>(points);
}

bool nobMilitary::wantCoins() const
{
    // Wenn die Goldzufuhr gestoppt wurde oder Münzvorrat voll ist, will ich gar keine Goldmünzen
    return (!coinsDisabled && numCoins + ordered_coins.size() != getMaxCoinCt() && !new_built);
}

void nobMilitary::searchCoins()
{
    // Brauche ich überhaupt Goldmünzen bzw. hab ich vielleicht schon ein Event angemeldet?
    if(wantCoins() && !gold_order_event)
    {
        // Lagerhaus mit Goldmünzen suchen
        nobBaseWarehouse* wh =
          world->getPlayer(player).findWarehouse(*this, FW::HasMinWares(GoodType::Coins), false, false);
        if(wh)
        {
            // Wenns eins gibt, dort eine Goldmünze bestellen
            Ware* ware = wh->orderWare(GoodType::Coins, *this);

            if(!ware)
            {
                RTTR_Assert(false);
                // Ware dürfte nicht 0 werden, da ja ein Lagerhaus MIT GOLDMÜNZEN bereits gesucht wird
                LOG.write("nobMilitary::SearchCoins: WARNING: ware = nullptr. Bug alarm!\n");
                return;
            }

            RTTR_Assert(helpers::contains(ordered_coins, ware));

            // Nach einer Weile nochmal nach evtl neuen Goldmünzen gucken
            gold_order_event = getEvMgr().addEvent(this, 200 + RANDOM_RAND(400), 1);
        }
    }
}

bool nobMilitary::wantArmor() const
{
    // If armor delivery stopped or stock already full, we do not want any armor
    return (armorAllowed && numArmor + ordered_armor.size() < getMaxArmorCt() && !new_built);
}

void nobMilitary::searchArmor()
{
    if(!leatheraddon::isAddonActive(world->getPlayer(player).getGameWorld()))
        return;

    if(wantArmor() && !armor_order_event)
    {
        nobBaseWarehouse* wh =
          world->getPlayer(player).findWarehouse(*this, FW::HasMinWares(GoodType::Armor), false, false);
        if(wh)
        {
            Ware* ware = wh->orderWare(GoodType::Armor, *this);

            if(!ware)
            {
                RTTR_Assert(false);
                LOG.write("nobMilitary::SearchArmor: WARNING: ware = nullptr. Bug alarm!\n");
                return;
            }

            RTTR_Assert(helpers::contains(ordered_armor, ware));

            // After some time, try to order new armor
            armor_order_event = getEvMgr().addEvent(this, 200 + RANDOM_RAND(400), 3);
        }
    }
}

void nobMilitary::prepareUpgrading()
{
    // Goldmünzen da?
    if(!numCoins)
        return;

    // Gibts auch noch kein Beförderungsevent?
    if(upgrade_event)
        return;

    // Noch Soldaten, die befördert werden können?
    bool soldiersAvailable = false;

    const unsigned maxRank = world->getGGS().getMaxMilitaryRank();
    for(auto& troop : troops)
    {
        if(troop->getRank() < maxRank)
        {
            // es wurde ein Soldat gefunden, der befördert werden kann
            soldiersAvailable = true;
            break;
        }
    }

    if(!soldiersAvailable)
        return;

    // Alles da --> Beförderungsevent anmelden
    upgrade_event = getEvMgr().addEvent(this, UPGRADE_TIME + RANDOM_RAND(UPGRADE_TIME_RANDOM), 2);
}

void nobMilitary::prepareArmorUpgrading()
{
    if(!numArmor)
        return;

    if(armor_upgrade_event)
        return;

    if(std::none_of(troops.begin(), troops.end(),
                    [](OwnedSortedTroops::value_type& troop) { return !troop->hasArmor(); }))
        return;

    // All here --> Trigger armor upgrade event
    armor_upgrade_event = getEvMgr().addEvent(this, UPGRADE_TIME + RANDOM_RAND(UPGRADE_TIME_RANDOM), 4);
}

void nobMilitary::hitOfCatapultStone()
{
    // Ein Soldat weniger, falls es noch welche gibt
    if(!troops.empty())
    {
        std::unique_ptr<nofPassiveSoldier> soldier = std::move(*troops.begin());
        helpers::popFront(troops);

        // 30 percent chance to survive if the soldier has an armor
        const bool survived = soldier->hasArmor() && (RANDOM_RAND(100) < 30);
        if(!survived)
        {
            // Shortcut for die(): No need to remove from world as it is inside and we can delete it right away
            soldier->removeFromInventory();
            soldier->leftBuilding();
            soldier->destroy();
        } else
        {
            soldier->setArmor(false);
            world->getPlayer(player).decreaseInventoryJob(jobEnumToAmoredSoldierEnum(soldier->getJobType()), 1);
            troops.insert(std::move(soldier));
        }
    }

    // If there are troops left, order some more, else this will be destroyed
    if(!troops.empty())
        regulateTroops();

    // Post verschicken
    sendPostMessage(player,
                    std::make_unique<PostMsgWithBuilding>(getEvMgr().getCurrentGF(), _("A catapult is firing upon us!"),
                                                          PostCategory::Military, *this));
}

/**
 *  Darf das Militärgebäude abgerissen werden (Abriss-Verbot berücksichtigen)?
 */
bool nobMilitary::isDemolitionAllowed() const
{
    switch(world->getGGS().getSelection(AddonId::DEMOLITION_PROHIBITION))
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

void nobMilitary::unlinkAggressor(nofAttacker& soldier)
{
    RTTR_Assert(isAggressor(soldier) || isFarAwayCapturer(soldier));
    aggressors.remove(&soldier);
    far_away_capturers.remove(&soldier);

    if(aggressors.empty())
        regulateTroops();
}

void nobMilitary::capturingSoldierArrived()
{
    RTTR_Assert(isBeingCaptured());
    RTTR_Assert(capturing_soldiers > 0);
    --capturing_soldiers;
    if(capturing_soldiers == 0)
    {
        // Search again
        needOccupyingTroops();
        if(capturing_soldiers > 0)
            return; // Found more
        // Einnahme beendet
        capturing = false;
        // Nun die Besetzung prüfen
        regulateTroops();
    }
}

void nobMilitary::farAwayCapturerReachedGoal(nofAttacker& attacker, bool walkingIntoBld)
{
    RTTR_Assert(isFarAwayCapturer(attacker));
    if(isBeingCaptured())
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
        callNextFarAwayCapturer(attacker);
    }
}

bool nobMilitary::isFarAwayCapturer(const nofAttacker& attacker)
{
    return helpers::contains(far_away_capturers, &attacker);
}

void nobMilitary::callNextFarAwayCapturer(nofAttacker& attacker)
{
    const MapPoint flagPos = getFlagPos();
    unsigned minLength = std::numeric_limits<unsigned>::max();
    nofAttacker* bestAttacker = nullptr;
    for(auto* farAwayCapturer : far_away_capturers)
    {
        // Skip us and possible capturers at the building
        if(farAwayCapturer == &attacker || farAwayCapturer->getPos() == pos)
            continue;
        if(!farAwayCapturer->isAttackerReady())
            continue;
        RTTR_Assert(farAwayCapturer->getPos() != flagPos); // Impossible. This should be the current attacker
        unsigned length;
        if(!world->findHumanPath(farAwayCapturer->getPos(), flagPos, MAX_FAR_AWAY_CAPTURING_DISTANCE, false, &length))
            continue;
        if(length < minLength)
        {
            minLength = length;
            bestAttacker = farAwayCapturer;
        }
    }
    if(bestAttacker)
        bestAttacker->attackFlag();
}
