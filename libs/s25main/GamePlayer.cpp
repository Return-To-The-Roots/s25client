// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GamePlayer.h"
#include "AddonHelperFunctions.h"
#include "Cheats.h"
#include "EventManager.h"
#include "FindWhConditions.h"
#include "GameInterface.h"
#include "GlobalGameSettings.h"
#include "LeatherLoader.h"
#include "RoadSegment.h"
#include "SerializedGameData.h"
#include "TradePathCache.h"
#include "Ware.h"
#include "WineLoader.h"
#include "addons/const_addons.h"
#include "buildings/noBuildingSite.h"
#include "buildings/nobHQ.h"
#include "buildings/nobHarborBuilding.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobUsual.h"
#include "figures/nofCarrier.h"
#include "figures/nofFlagWorker.h"
#include "helpers/containerUtils.h"
#include "helpers/mathFuncs.h"
#include "lua/LuaInterfaceGame.h"
#include "notifications/ToolNote.h"
#include "pathfinding/RoadPathFinder.h"
#include "postSystem/DiplomacyPostQuestion.h"
#include "postSystem/PostManager.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include "world/TradeRoute.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noShip.h"
#include "gameTypes/BuildingCount.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/JobTypes.h"
#include "gameTypes/PactTypes.h"
#include "gameTypes/VisualSettings.h"
#include "gameData/BuildingConsts.h"
#include "gameData/BuildingProperties.h"
#include "gameData/GoodConsts.h"
#include "gameData/SettingTypeConv.h"
#include "gameData/ShieldConsts.h"
#include "gameData/ToolConsts.h"
#include "s25util/Log.h"
#include <limits>
#include <numeric>

GamePlayer::GamePlayer(unsigned playerId, const PlayerInfo& playerInfo, GameWorld& world)
    : GamePlayerInfo(playerId, playerInfo), world(world), hqPos(MapPoint::invalid()), emergency(false)
{
    std::fill(building_enabled.begin(), building_enabled.end(), true);

    loadStandardDistribution();
    useCustomBuildOrder_ = false;
    build_order = getStandardBuildOrder();
    transportPrio = STD_TRANSPORT_PRIO;
    loadStandardMilitarySettings();
    loadStandardToolSettings();

    // Inventur nullen
    global_inventory.clear();

    // Statistiken mit 0en füllen
    statistic = {};
    statisticCurrentData = {};
    statisticCurrentMerchandiseData = {};

    recalcDistribution();
}

void GamePlayer::loadStandardToolSettings()
{
    // metalwork tool request

    // manually
    std::fill(tools_ordered.begin(), tools_ordered.end(), 0u);
    std::fill(tools_ordered_delta.begin(), tools_ordered_delta.end(), 0);

    // percentage (tool-settings-window-slider, in 10th percent)
    toolsSettings_[Tool::Tongs] = 1;
    toolsSettings_[Tool::Hammer] = 4;
    toolsSettings_[Tool::Axe] = 2;
    toolsSettings_[Tool::Saw] = 5;
    toolsSettings_[Tool::PickAxe] = 7;
    toolsSettings_[Tool::Shovel] = 1;
    toolsSettings_[Tool::Crucible] = 3;
    toolsSettings_[Tool::RodAndLine] = 1;
    toolsSettings_[Tool::Scythe] = 2;
    toolsSettings_[Tool::Cleaver] = 1;
    toolsSettings_[Tool::Rollingpin] = 2;
    toolsSettings_[Tool::Bow] = 1;
}

void GamePlayer::loadStandardMilitarySettings()
{
    // military settings (military-window-slider, in 10th percent)
    militarySettings_[0] = MILITARY_SETTINGS_SCALE[0]; //-V525
    militarySettings_[1] = 3;
    militarySettings_[2] = MILITARY_SETTINGS_SCALE[2];
    militarySettings_[3] = 3;
    militarySettings_[4] = 0;
    militarySettings_[5] = 1;
    militarySettings_[6] = MILITARY_SETTINGS_SCALE[6];
    militarySettings_[7] = MILITARY_SETTINGS_SCALE[7];
}

BuildOrders GamePlayer::getStandardBuildOrder()
{
    BuildOrders ordering;

    // Baureihenfolge füllen
    unsigned curPrio = 0;
    for(const auto bld : helpers::enumRange<BuildingType>())
    {
        if(bld == BuildingType::Headquarters || !BuildingProperties::isValid(bld))
            continue;

        RTTR_Assert(curPrio < ordering.size());
        ordering[curPrio] = bld;
        ++curPrio;
    }
    RTTR_Assert(curPrio == ordering.size());
    return ordering;
}

void GamePlayer::loadStandardDistribution()
{
    // Verteilung mit Standardwerten füllen bei Waren mit nur einem Ziel (wie z.B. Mehl, Holz...)
    distribution[GoodType::Flour].client_buildings.push_back(BuildingType::Bakery);
    distribution[GoodType::Gold].client_buildings.push_back(BuildingType::Mint);
    distribution[GoodType::IronOre].client_buildings.push_back(BuildingType::Ironsmelter);
    distribution[GoodType::Ham].client_buildings.push_back(BuildingType::Slaughterhouse);
    distribution[GoodType::Stones].client_buildings.push_back(
      BuildingType::Headquarters); // BuildingType::Headquarters = Baustellen!
    distribution[GoodType::Stones].client_buildings.push_back(BuildingType::Catapult);
    distribution[GoodType::Grapes].client_buildings.push_back(BuildingType::Winery);
    distribution[GoodType::Wine].client_buildings.push_back(BuildingType::Temple);
    distribution[GoodType::Skins].client_buildings.push_back(BuildingType::Tannery);
    distribution[GoodType::Leather].client_buildings.push_back(BuildingType::LeatherWorks);

    // Waren mit mehreren möglichen Zielen erstmal nullen, kann dann im Fenster eingestellt werden
    for(const auto i : helpers::enumRange<GoodType>())
    {
        std::fill(distribution[i].percent_buildings.begin(), distribution[i].percent_buildings.end(), 0);
        distribution[i].selected_goal = 0;
    }

    // Standardverteilung der Waren
    for(const DistributionMapping& mapping : distributionMap)
    {
        distribution[std::get<0>(mapping)].percent_buildings[std::get<1>(mapping)] = std::get<2>(mapping);
    }
}

GamePlayer::~GamePlayer() = default;

void GamePlayer::serialize(SerializedGameData& sgd) const
{
    // PlayerStatus speichern, ehemalig
    sgd.pushEnum<uint8_t>(ps);

    // Nur richtige Spieler serialisieren
    if(ps != PlayerState::Occupied && ps != PlayerState::AI)
        return;

    sgd.pushBool(isDefeated_);

    buildings.serialize(sgd);

    sgd.pushObjectContainer(roads, true);

    sgd.pushUnsignedInt(jobs_wanted.size());
    for(const JobNeeded& job : jobs_wanted)
    {
        sgd.pushEnum<uint8_t>(job.job);
        sgd.pushObject(job.workplace);
    }

    sgd.pushObjectContainer(ware_list, true);
    sgd.pushObjectContainer(flagworkers);
    sgd.pushObjectContainer(ships, true);

    helpers::pushContainer(sgd, shouldSendDefenderList);
    helpers::pushPoint(sgd, hqPos);

    for(const Distribution& dist : distribution)
    {
        helpers::pushContainer(sgd, dist.percent_buildings);
        helpers::pushContainer(sgd, dist.client_buildings);
        helpers::pushContainer(sgd, dist.goals);
        sgd.pushUnsignedInt(dist.selected_goal);
    }

    sgd.pushBool(useCustomBuildOrder_);
    helpers::pushContainer(sgd, build_order);
    helpers::pushContainer(sgd, transportPrio);
    helpers::pushContainer(sgd, militarySettings_);
    helpers::pushContainer(sgd, toolsSettings_);
    helpers::pushContainer(sgd, tools_ordered);
    helpers::pushContainer(sgd, global_inventory.goods);
    helpers::pushContainer(sgd, global_inventory.people);
    helpers::pushContainer(sgd, global_inventory.armoredSoldiers);

    // für Statistik
    for(const Statistic& curStatistic : statistic)
    {
        // normale Statistik
        for(const auto& curData : curStatistic.data)
            helpers::pushContainer(sgd, curData);

        // Warenstatistik
        for(unsigned j = 0; j < NUM_STAT_MERCHANDISE_TYPES; ++j)
            helpers::pushContainer(sgd, curStatistic.merchandiseData[j]);

        sgd.pushUnsignedShort(curStatistic.currentIndex);
        sgd.pushUnsignedShort(curStatistic.counter);
    }
    helpers::pushContainer(sgd, statisticCurrentData);
    helpers::pushContainer(sgd, statisticCurrentMerchandiseData);

    // Serialize Pacts:
    for(const auto& playerPacts : pacts)
    {
        for(const Pact& pact : playerPacts)
            pact.serialize(sgd);
    }

    sgd.pushBool(emergency);
}

void GamePlayer::deserialize(SerializedGameData& sgd)
{
    std::fill(building_enabled.begin(), building_enabled.end(), true);

    // Ehemaligen PS auslesen
    auto origin_ps = sgd.pop<PlayerState>();
    // Nur richtige Spieler serialisieren
    if(origin_ps != PlayerState::Occupied && origin_ps != PlayerState::AI)
        return;

    isDefeated_ = sgd.popBool();
    buildings.deserialize(sgd);

    sgd.popObjectContainer(roads, GO_Type::Roadsegment);

    jobs_wanted.resize(sgd.popUnsignedInt());
    for(JobNeeded& job : jobs_wanted)
    {
        job.job = sgd.pop<Job>();
        job.workplace = sgd.popObject<noRoadNode>();
    }

    if(sgd.getGameDataVersion() < 2)
        buildings.deserialize2(sgd);

    sgd.popObjectContainer(ware_list, GO_Type::Ware);
    sgd.popObjectContainer(flagworkers);
    sgd.popObjectContainer(ships, GO_Type::Ship);

    sgd.popContainer(shouldSendDefenderList);

    hqPos = sgd.popMapPoint();

    for(const auto i : helpers::enumRange<GoodType>())
    {
        if(sgd.getGameDataVersion() < 11 && wineaddon::isWineAddonGoodType(i))
            continue;

        if(sgd.getGameDataVersion() < 12 && leatheraddon::isLeatherAddonGoodType(i))
            continue;

        Distribution& dist = distribution[i];
        helpers::popContainer(sgd, dist.percent_buildings);
        // Set standard value otherwise its zero and Slaughterhouse never gets ham
        // because the ham distribution was not there in earlier versions
        if(sgd.getGameDataVersion() < 12 && i == GoodType::Ham)
            dist.percent_buildings[BuildingType::Slaughterhouse] = 8;

        if(sgd.getGameDataVersion() < 7)
        {
            dist.client_buildings.resize(sgd.popUnsignedInt());
            helpers::popContainer(sgd, dist.client_buildings, true);
            dist.goals.resize(sgd.popUnsignedInt());
            helpers::popContainer(sgd, dist.goals, true);
        } else
        {
            helpers::popContainer(sgd, dist.client_buildings);
            helpers::popContainer(sgd, dist.goals);
        }
        dist.selected_goal = sgd.popUnsignedInt();
    }

    useCustomBuildOrder_ = sgd.popBool();

    if(sgd.getGameDataVersion() < 12)
    {
        auto countOfNotAvailableBuildingsInSaveGame =
          sgd.getGameDataVersion() < 11 ? numWineAndLeatherAddonBuildings : numLeatherAddonBuildings;
        std::vector<BuildingType> build_order_raw(build_order.size() - countOfNotAvailableBuildingsInSaveGame);
        helpers::popContainer(sgd, build_order_raw, true);

        if(sgd.getGameDataVersion() < 11)
        {
            build_order_raw.insert(build_order_raw.end(),
                                   {BuildingType::Vineyard, BuildingType::Winery, BuildingType::Temple});
        }

        if(sgd.getGameDataVersion() < 12)
        {
            build_order_raw.insert(build_order_raw.end(),
                                   {BuildingType::Skinner, BuildingType::Tannery, BuildingType::LeatherWorks});
        }

        std::copy(build_order_raw.begin(), build_order_raw.end(), build_order.begin());

        auto countOfNotAvailableGoodsInSaveGame =
          sgd.getGameDataVersion() < 11 ? numWineAndLeatherAddonGoods : numLeatherAddonGoods;
        std::vector<uint8_t> transportPrio_raw(transportPrio.size() - countOfNotAvailableGoodsInSaveGame);
        helpers::popContainer(sgd, transportPrio_raw, true);
        std::copy(transportPrio_raw.begin(), transportPrio_raw.end(), transportPrio.begin());
        std::transform(transportPrio.begin(), transportPrio.end() - countOfNotAvailableGoodsInSaveGame,
                       transportPrio.begin(),
                       [](uint8_t& prio) { return prio < transportPrioOfLeatherworks ? prio : prio + 1; });
    } else
    {
        helpers::popContainer(sgd, build_order);
        helpers::popContainer(sgd, transportPrio);
    }

    helpers::popContainer(sgd, militarySettings_);
    helpers::popContainer(sgd, toolsSettings_);

    // qx:tools
    helpers::popContainer(sgd, tools_ordered);
    tools_ordered_delta = {};

    if(sgd.getGameDataVersion() < 12)
    {
        auto countOfNotAvailableGoodsInSaveGame =
          sgd.getGameDataVersion() < 11 ? numWineAndLeatherAddonGoods : numLeatherAddonGoods;
        std::vector<unsigned int> global_inventory_good_raw(global_inventory.goods.size()
                                                            - countOfNotAvailableGoodsInSaveGame);
        helpers::popContainer(sgd, global_inventory_good_raw, true);
        std::copy(global_inventory_good_raw.begin(), global_inventory_good_raw.end(), global_inventory.goods.begin());

        auto countOfNotAvailableJobsInSaveGame =
          sgd.getGameDataVersion() < 11 ? numWineAndLeatherAddonJobs : numLeatherAddonJobs;
        std::vector<unsigned int> global_inventory_people_raw(global_inventory.people.size()
                                                              - countOfNotAvailableJobsInSaveGame);
        helpers::popContainer(sgd, global_inventory_people_raw, true);
        std::copy(global_inventory_people_raw.begin(), global_inventory_people_raw.end(),
                  global_inventory.people.begin());

        std::fill(global_inventory.armoredSoldiers.begin(), global_inventory.armoredSoldiers.end(), 0);
    } else
    {
        helpers::popContainer(sgd, global_inventory.goods);
        helpers::popContainer(sgd, global_inventory.people);
        helpers::popContainer(sgd, global_inventory.armoredSoldiers);
    }

    // Visuelle Einstellungen festlegen

    // für Statistik
    for(Statistic& curStatistic : statistic)
    {
        // normale Statistik
        for(auto& curData : curStatistic.data)
            helpers::popContainer(sgd, curData);

        // Warenstatistik
        for(unsigned j = 0; j < NUM_STAT_MERCHANDISE_TYPES; ++j)
            helpers::popContainer(sgd, curStatistic.merchandiseData[j]);

        curStatistic.currentIndex = sgd.popUnsignedShort();
        curStatistic.counter = sgd.popUnsignedShort();
    }
    helpers::popContainer(sgd, statisticCurrentData);
    helpers::popContainer(sgd, statisticCurrentMerchandiseData);

    // Deserialize Pacts:
    for(auto& playerPacts : pacts)
    {
        for(Pact& pact : playerPacts)
            pact = GamePlayer::Pact(sgd);
    }

    emergency = sgd.popBool();
}

template<class T_IsWarehouseGood>
nobBaseWarehouse* GamePlayer::findWarehouse(const noRoadNode& start, const T_IsWarehouseGood& isWarehouseGood,
                                            bool to_wh, bool use_boat_roads, unsigned* length,
                                            const RoadSegment* forbidden) const
{
    nobBaseWarehouse* best = nullptr;

    unsigned best_length = std::numeric_limits<unsigned>::max();

    for(nobBaseWarehouse* wh : buildings.getStorehouses())
    {
        // Lagerhaus geeignet?
        if(!isWarehouseGood(assertNonNull(wh)))
            continue;

        if(start.getPos() == wh->getPos())
        {
            // We are already there -> Take it
            if(length)
                *length = 0;
            return wh;
        }

        // now check if there is at least a chance that the next wh is closer than current best because pathfinding
        // takes time
        if(world.calcDistance(start.getPos(), wh->getPos()) > best_length)
            continue;
        // Bei der erlaubten Benutzung von Bootsstraßen Waren-Pathfinding benutzen wenns zu nem Lagerhaus gehn soll
        // start <-> ziel tauschen bei der wegfindung
        unsigned tlength;
        if(world.getRoadPathFinder().findPath(to_wh ? start : *wh, to_wh ? *wh : start, use_boat_roads, best_length,
                                              forbidden, &tlength))
        {
            if(tlength < best_length || !best)
            {
                best_length = tlength;
                best = wh;
            }
        }
    }

    if(length)
        *length = best_length;

    return best;
}

void GamePlayer::addBuildingSite(noBuildingSite* bldSite)
{
    RTTR_Assert(bldSite->getPlayer() == getPlayerId());
    buildings.add(bldSite);
}

void GamePlayer::removeBuildingSite(noBuildingSite* bldSite)
{
    RTTR_Assert(bldSite->getPlayer() == getPlayerId());
    buildings.remove(bldSite);
}

bool GamePlayer::isHQTent() const
{
    if(const nobHQ* hq = getHQ())
        return hq->isTent();
    return false;
}

void GamePlayer::setHQIsTent(bool isTent)
{
    if(nobHQ* hq = getHQ())
        hq->setIsTent(isTent);
}

void GamePlayer::addBuilding(noBuilding* bld, BuildingType bldType)
{
    RTTR_Assert(bld->getPlayer() == getPlayerId());
    buildings.add(bld, bldType);
    changeStatisticValue(StatisticType::Buildings, 1);

    // Order a worker if needed
    const auto& description = BLD_WORK_DESC[bldType];
    if(description.job && !isSoldier(*description.job))
    {
        addJobWanted(*description.job, bld);
    }

    if(bldType == BuildingType::HarborBuilding)
    {
        // Schiff durchgehen und denen Bescheid sagen
        for(noShip* ship : ships)
            ship->newHarborBuilt(static_cast<nobHarborBuilding*>(bld));
    } else if(bldType == BuildingType::Headquarters)
    {
        // If there is more than one HQ, keep the original position.
        if(!hqPos.isValid())
            hqPos = bld->getPos();
    } else if(BuildingProperties::isMilitary(bldType))
    {
        auto* milBld = static_cast<nobMilitary*>(bld);
        // New built? -> Calculate frontier distance
        if(milBld->isNewBuilt())
            milBld->lookForEnemyBuildings();
    }
}

void GamePlayer::removeBuilding(noBuilding* bld, BuildingType bldType)
{
    RTTR_Assert(bld->getPlayer() == getPlayerId());
    buildings.remove(bld, bldType);
    changeStatisticValue(StatisticType::Buildings, -1);
    if(bldType == BuildingType::HarborBuilding)
    { // Schiffen Bescheid sagen
        for(noShip* ship : ships)
            ship->harborDestroyed(static_cast<nobHarborBuilding*>(bld));
    } else if(bldType == BuildingType::Headquarters && bld->getPos() == hqPos)
    {
        hqPos = MapPoint::invalid();
        for(const noBaseBuilding* bld : buildings.getStorehouses())
        {
            if(bld->getBuildingType() == BuildingType::Headquarters)
            {
                hqPos = bld->getPos();
                break;
            }
        }
    }
    if(BuildingProperties::isWareHouse(bldType) || BuildingProperties::isMilitary(bldType))
        testDefeat();
}

void GamePlayer::newRoadConnection(RoadSegment* rs)
{
    // Zu den Straßen hinzufgen, da's ja ne neue ist
    roads.push_back(rs);

    // Alle Straßen müssen nun gucken, ob sie einen Weg zu einem Warehouse finden
    findCarrierForAllRoads();

    // Alle Straßen müssen gucken, ob sie einen Esel bekommen können
    for(RoadSegment* rs : roads)
        rs->tryGetDonkey();

    // Alle Arbeitsplätze müssen nun gucken, ob sie einen Weg zu einem Lagerhaus mit entsprechender Arbeitskraft finden
    findWarehouseForAllJobs();

    // Alle Baustellen müssen nun gucken, ob sie ihr benötigtes Baumaterial bekommen (evtl war vorher die Straße zum
    // Lagerhaus unterbrochen
    findMaterialForBuildingSites();

    // Alle Lost-Wares müssen gucken, ob sie ein Lagerhaus finden
    findClientForLostWares();

    // Alle Militärgebäude müssen ihre Truppen überprüfen und können nun ggf. neue bestellen
    // und müssen prüfen, ob sie evtl Gold bekommen
    for(nobMilitary* mil : buildings.getMilitaryBuildings())
    {
        mil->regulateTroops();
        mil->searchCoins();
    }
}

void GamePlayer::addRoad(RoadSegment* rs)
{
    roads.push_back(rs);
}

void GamePlayer::deleteRoad(RoadSegment* rs)
{
    RTTR_Assert(helpers::contains(roads, rs));
    roads.remove(rs);
}

void GamePlayer::findClientForLostWares()
{
    // Alle Lost-Wares müssen gucken, ob sie ein Lagerhaus finden
    for(Ware* ware : ware_list)
    {
        if(ware->isLostWare())
        {
            if(ware->findRouteToWarehouse() && ware->isWaitingAtFlag())
                ware->callCarrier();
        }
    }
}

void GamePlayer::roadDestroyed()
{
    // Alle Waren, die an Flagge liegen und in Lagerhäusern, müssen gucken, ob sie ihr Ziel noch erreichen können, jetzt
    // wo eine Straße fehlt
    for(auto it = ware_list.begin(); it != ware_list.end();)
    {
        Ware* ware = *it;
        if(ware->isWaitingAtFlag()) // Liegt die Flagge an einer Flagge, muss ihr Weg neu berechnet werden
        {
            RoadPathDirection last_next_dir = ware->getNextDir();
            ware->recalcRoute();
            // special case: ware was lost some time ago and the new goal is at this flag and not a warehouse,hq,harbor
            // and the "flip-route" picked so a carrier would pick up the ware carry it away from goal then back and
            // drop  it off at the goal was just destroyed?
            // -> try to pick another flip route or tell the goal about failure.
            noRoadNode& wareLocation = *ware->getLocation();
            noBaseBuilding* wareGoal = ware->getGoal();
            if(wareGoal && ware->getNextDir() == RoadPathDirection::NorthWest
               && wareLocation.getPos() == wareGoal->getFlagPos()
               && ((wareGoal->getBuildingType() != BuildingType::Storehouse
                    && wareGoal->getBuildingType() != BuildingType::Headquarters
                    && wareGoal->getBuildingType() != BuildingType::HarborBuilding)
                   || wareGoal->getType() == NodalObjectType::Buildingsite))
            {
                Direction newWareDir = Direction::NorthWest;
                for(auto dir : helpers::EnumRange<Direction>{})
                {
                    dir += 2u; // Need to skip Direction::NorthWest and we used to start with an offset of 2. TODO:
                               // Increase gameDataVersion and just skip NW
                    if(wareLocation.getRoute(dir))
                    {
                        newWareDir = dir;
                        break;
                    }
                }
                if(newWareDir != Direction::NorthWest)
                {
                    ware->setNextDir(toRoadPathDirection(newWareDir));
                } else // no route to goal -> notify goal, try to send ware to a warehouse
                {
                    ware->notifyGoalAboutLostWare();
                    ware->findRouteToWarehouse();
                }
            }
            // end of special case

            // notify carriers/flags about news if there are any
            if(ware->getNextDir() != last_next_dir)
            {
                // notify current flag that transport in the old direction might not longer be required
                ware->removeWareJobForDir(last_next_dir);
                if(ware->getNextDir() != RoadPathDirection::None)
                    ware->callCarrier();
            }
        } else if(ware->isWaitingInWarehouse())
        {
            if(!ware->isRouteToGoal())
            {
                // Das Ziel wird nun nich mehr beliefert
                ware->notifyGoalAboutLostWare();
                // Ware aus der Warteliste des Lagerhauses entfernen
                static_cast<nobBaseWarehouse*>(ware->getLocation())->cancelWare(ware);
                // Ware aus der Liste raus
                it = ware_list.erase(it);
                continue;
            }
        } else if(ware->isWaitingForShip())
        {
            // Weg neu berechnen
            ware->recalcRoute();
        }

        ++it;
    }

    // Alle Häfen müssen ihre Figuren den Weg überprüfen lassen
    for(nobHarborBuilding* hb : buildings.getHarbors())
    {
        hb->examineShipRouteOfPeople();
    }
}

bool GamePlayer::findCarrierForRoad(RoadSegment& rs) const
{
    RTTR_Assert(rs.getF1() != nullptr && rs.getF2() != nullptr);
    std::array<unsigned, 2> length;
    std::array<nobBaseWarehouse*, 2> best;

    // Braucht der ein Boot?
    if(rs.getRoadType() == RoadType::Water)
    {
        // dann braucht man Träger UND Boot
        best[0] = findWarehouse(*rs.getF1(), FW::HasWareAndFigure(GoodType::Boat, Job::Helper, false), false, false,
                                length.data(), &rs);
        // 2. Flagge des Weges
        best[1] = findWarehouse(*rs.getF2(), FW::HasWareAndFigure(GoodType::Boat, Job::Helper, false), false, false,
                                &length[1], &rs);
    } else
    {
        // 1. Flagge des Weges
        best[0] = findWarehouse(*rs.getF1(), FW::HasFigure(Job::Helper, false), false, false, length.data(), &rs);
        // 2. Flagge des Weges
        best[1] = findWarehouse(*rs.getF2(), FW::HasFigure(Job::Helper, false), false, false, &length[1], &rs);
    }

    // überhaupt nen Weg gefunden?
    // Welche Flagge benutzen?
    if(best[0] && (!best[1] || length[0] < length[1]))
        best[0]->orderCarrier(*rs.getF1(), rs);
    else if(best[1])
        best[1]->orderCarrier(*rs.getF2(), rs);
    else
        return false;
    return true;
}

bool GamePlayer::isWarehouseValid(nobBaseWarehouse* wh) const
{
    return helpers::contains(buildings.getStorehouses(), wh);
}

void GamePlayer::recalcDistribution()
{
    GoodType lastWare = GoodType::Nothing;
    for(const DistributionMapping& mapping : distributionMap)
    {
        if(lastWare == std::get<0>(mapping))
            continue;
        lastWare = std::get<0>(mapping);
        recalcDistributionOfWare(std::get<0>(mapping));
    }
}

void GamePlayer::recalcDistributionOfWare(const GoodType ware)
{
    // Punktesystem zur Verteilung, in der Liste alle Gebäude sammeln, die die Ware wollen
    distribution[ware].client_buildings.clear();

    // 1. Anteile der einzelnen Waren ausrechnen

    /// Mapping of buildings that want the current ware to its percentage
    using BldEntry = std::pair<BuildingType, uint8_t>;
    std::vector<BldEntry> bldPercentageMap;

    unsigned goal_count = 0;

    for(const auto bld : helpers::enumRange<BuildingType>())
    {
        uint8_t percentForCurBld = distribution[ware].percent_buildings[bld];
        if(percentForCurBld)
        {
            distribution[ware].client_buildings.push_back(bld);
            goal_count += percentForCurBld;
            bldPercentageMap.emplace_back(bld, percentForCurBld);
        }
    }

    // TODO: evtl noch die counts miteinander kürzen (ggt berechnen)

    // Array für die Gebäudtypen erstellen

    std::vector<BuildingType>& wareGoals = distribution[ware].goals;
    wareGoals.clear();
    wareGoals.reserve(goal_count);

    // just drop them in the list, the distribution will be handled by going through this list using a prime as step
    // (see GameClientPlayer::FindClientForWare)
    for(const BldEntry& bldEntry : bldPercentageMap)
    {
        for(unsigned char i = 0; i < bldEntry.second; ++i)
            wareGoals.push_back(bldEntry.first);
    }

    distribution[ware].selected_goal = 0;
}

void GamePlayer::findCarrierForAllRoads()
{
    for(RoadSegment* rs : roads)
    {
        if(!rs->hasCarrier(0))
            findCarrierForRoad(*rs);
    }
}

void GamePlayer::findMaterialForBuildingSites()
{
    for(noBuildingSite* bldSite : buildings.getBuildingSites())
        bldSite->orderConstructionMaterial();
}

void GamePlayer::addJobWanted(const Job job, noRoadNode* workplace)
{
    if(!findWarehouseForJob(job, *workplace))
    {
        JobNeeded jn = {job, workplace};
        jobs_wanted.push_back(jn);
    }
}

void GamePlayer::jobNotWanted(noRoadNode* workplace, bool all)
{
    for(auto it = jobs_wanted.begin(); it != jobs_wanted.end();)
    {
        if(it->workplace == workplace)
        {
            it = jobs_wanted.erase(it);
            if(!all)
                return;
        } else
        {
            ++it;
        }
    }
}

void GamePlayer::oneJobNotWanted(const Job job, noRoadNode* workplace)
{
    const auto it = helpers::findIf(
      jobs_wanted, [workplace, job](const auto& it) { return it.workplace == workplace && it.job == job; });
    if(it != jobs_wanted.end())
        jobs_wanted.erase(it);
}

void GamePlayer::sendPostMessage(std::unique_ptr<PostMsg> msg)
{
    world.getPostMgr().sendMsg(getPlayerId(), std::move(msg));
}

unsigned GamePlayer::getToolsOrderedVisual(Tool tool) const
{
    return std::max(0, int(tools_ordered[tool] + tools_ordered_delta[tool]));
}

unsigned GamePlayer::getToolsOrdered(Tool tool) const
{
    return tools_ordered[tool];
}

bool GamePlayer::changeToolOrderVisual(Tool tool, int changeAmount) const
{
    if(std::abs(changeAmount) > 100)
        return false;
    int newOrderAmount = int(getToolsOrderedVisual(tool)) + changeAmount;
    if(newOrderAmount < 0 || newOrderAmount > 100)
        return false;
    tools_ordered_delta[tool] += changeAmount;
    return true;
}

unsigned GamePlayer::getToolPriority(Tool tool) const
{
    return toolsSettings_[tool];
}

void GamePlayer::toolOrderProcessed(Tool tool)
{
    if(tools_ordered[tool])
    {
        --tools_ordered[tool];
        world.getNotifications().publish(ToolNote(ToolNote::OrderCompleted, getPlayerId()));
    }
}

bool GamePlayer::findWarehouseForJob(const Job job, noRoadNode& goal) const
{
    // Optimization: return early if building is isolated
    if(goal.getType() == NodalObjectType::Building || goal.getType() == NodalObjectType::Buildingsite)
    {
        if(!static_cast<noBaseBuilding&>(goal).isConnected())
            return false;
    }

    nobBaseWarehouse* wh = findWarehouse(goal, FW::HasFigure(job, true), false, false);

    if(wh)
    {
        // Es wurde ein Lagerhaus gefunden, wo es den geforderten Beruf gibt, also den Typen zur Arbeit rufen
        wh->orderJob(job, goal, true);
        return true;
    }

    return false;
}

void GamePlayer::findWarehouseForAllJobs()
{
    for(auto it = jobs_wanted.begin(); it != jobs_wanted.end();)
    {
        if(findWarehouseForJob(it->job, *it->workplace))
            it = jobs_wanted.erase(it);
        else
            ++it;
    }
}

void GamePlayer::findWarehouseForAllJobs(const Job job)
{
    for(auto it = jobs_wanted.begin(); it != jobs_wanted.end();)
    {
        if(it->job == job)
        {
            if(findWarehouseForJob(it->job, *it->workplace))
                it = jobs_wanted.erase(it);
            else
                ++it;
        } else
            ++it;
    }
}

static bool isWareFineWithEmergencyProtocol(GoodType goodType, const noBaseBuilding& goal)
{
    return (goodType != GoodType::Boards && goodType != GoodType::Stones)
           || goal.getBuildingType() == BuildingType::Woodcutter || goal.getBuildingType() == BuildingType::Sawmill;
}

Ware* GamePlayer::orderWare(const GoodType ware, noBaseBuilding& goal)
{
    /// Gibt es ein Lagerhaus mit dieser Ware?
    nobBaseWarehouse* wh = findWarehouse(goal, FW::HasMinWares(ware, 1), false, true);

    if(wh)
    {
        // Prüfe ob Notfallprogramm aktiv
        if(!emergency)
            return wh->orderWare(ware, goal);
        else
        {
            // Wenn Notfallprogramm aktiv nur an Holzfäller und Sägewerke Bretter/Steine liefern
            if(isWareFineWithEmergencyProtocol(ware, goal))
                return wh->orderWare(ware, goal);
            else
                return nullptr;
        }
    } else // no warehouse can deliver the ware -> check all our wares for lost wares that might match the order
    {
        unsigned bestLength = std::numeric_limits<unsigned>::max();
        Ware* bestWare = nullptr;
        for(Ware* curWare : ware_list)
        {
            if(curWare->isLostWare() && curWare->type == ware)
            {
                // got a lost ware with a road to goal -> find best
                unsigned curLength = curWare->checkNewGoalForLostWare(goal);
                if(curLength < bestLength)
                {
                    bestLength = curLength;
                    bestWare = curWare;
                }
            }
        }
        if(bestWare)
        {
            bestWare->setNewGoalForLostWare(goal);
            return bestWare;
        }
    }
    return nullptr;
}

nofCarrier* GamePlayer::orderDonkey(RoadSegment& road) const
{
    std::array<unsigned, 2> length;
    std::array<nobBaseWarehouse*, 2> best;

    // 1. Flagge des Weges
    best[0] = findWarehouse(*road.getF1(), FW::HasFigure(Job::PackDonkey, false), false, false, length.data(), &road);
    // 2. Flagge des Weges
    best[1] = findWarehouse(*road.getF2(), FW::HasFigure(Job::PackDonkey, false), false, false, &length[1], &road);

    // überhaupt nen Weg gefunden?
    // Welche Flagge benutzen?
    if(best[0] && (!best[1] || length[0] < length[1]))
        return best[0]->orderDonkey(road, *road.getF1());
    else if(best[1])
        return best[1]->orderDonkey(road, *road.getF2());
    else
        return nullptr;
}

RoadSegment* GamePlayer::findRoadForDonkey(noRoadNode& start, noRoadNode** goal)
{
    // Bisher höchste Trägerproduktivität und die entsprechende Straße dazu
    unsigned best_productivity = 0;
    RoadSegment* best_road = nullptr;
    // Beste Flagge dieser Straße
    *goal = nullptr;

    for(RoadSegment* roadSeg : roads)
    {
        // Braucht die Straße einen Esel?
        if(roadSeg->needDonkey())
        {
            // Beste Flagge von diesem Weg, und beste Wegstrecke
            noRoadNode* current_best_goal = nullptr;
            // Weg zu beiden Flaggen berechnen
            unsigned length1, length2;
            bool isF1Reachable = world.findHumanPathOnRoads(start, *roadSeg->getF1(), &length1, nullptr, roadSeg)
                                 != RoadPathDirection::None;
            bool isF2Reachable = world.findHumanPathOnRoads(start, *roadSeg->getF2(), &length2, nullptr, roadSeg)
                                 != RoadPathDirection::None;

            // Wenn man zu einer Flagge nich kommt, die jeweils andere nehmen
            if(!isF1Reachable)
                current_best_goal = (isF2Reachable) ? roadSeg->getF2() : nullptr;
            else if(!isF2Reachable)
                current_best_goal = roadSeg->getF1();
            else
            {
                // ansonsten die kürzeste von beiden
                current_best_goal = (length1 < length2) ? roadSeg->getF1() : roadSeg->getF2();
            }

            // Kein Weg führt hin, nächste Straße bitte
            if(!current_best_goal)
                continue;

            // Jeweiligen Weg bestimmen
            unsigned current_best_way = (roadSeg->getF1() == current_best_goal) ? length1 : length2;

            // Produktivität ausrechnen, *10 die Produktivität + die Wegstrecke, damit die
            // auch noch mit einberechnet wird
            unsigned current_productivity = 10 * roadSeg->getCarrier(0)->getProductivity() + current_best_way;

            // Besser als der bisher beste?
            if(current_productivity > best_productivity)
            {
                // Dann wird der vom Thron gestoßen
                best_productivity = current_productivity;
                best_road = roadSeg;
                *goal = current_best_goal;
            }
        }
    }

    return best_road;
}

struct ClientForWare
{
    noBaseBuilding* bld;
    unsigned estimate; // points minus half the optimal distance
    unsigned points;

    ClientForWare(noBaseBuilding* bld, unsigned estimate, unsigned points)
        : bld(bld), estimate(estimate), points(points)
    {}

    bool operator<(const ClientForWare& b) const noexcept
    {
        // use estimate, points and object id (as tie breaker) for sorting
        if(estimate != b.estimate)
            return estimate > b.estimate;
        else if(points != b.points)
            return points > b.points;
        else
            return bld->getObjId() > b.bld->getObjId();
    }
};

noBaseBuilding* GamePlayer::findClientForWare(const Ware& ware)
{
    // If the ware is a coin or an armor, the goal is determined by another logic
    if(ware.type == GoodType::Coins)
        return findClientForCoin(ware);
    else if(ware.type == GoodType::Armor)
        return findClientForArmor(ware);

    // Warentyp herausfinden
    GoodType gt = ware.type;
    // All food is considered fish in the distribution table
    Distribution& wareDistribution =
      (gt == GoodType::Bread || gt == GoodType::Meat) ? distribution[GoodType::Fish] : distribution[gt];

    std::vector<ClientForWare> possibleClients;

    const noRoadNode* start = ware.getLocation();

    // Bretter und Steine können evtl. auch Häfen für Expeditionen gebrauchen
    if(gt == GoodType::Stones || gt == GoodType::Boards)
    {
        for(nobHarborBuilding* harbor : buildings.getHarbors())
        {
            unsigned points = harbor->calcDistributionPoints(gt);
            if(!points)
                continue;

            points += 10 * 30; // Verteilung existiert nicht, Expeditionen haben allerdings hohe Priorität
            unsigned distance = world.calcDistance(start->getPos(), harbor->getPos()) / 2;
            possibleClients.push_back(ClientForWare(harbor, points > distance ? points - distance : 0, points));
        }
    }

    for(const auto bldType : wareDistribution.client_buildings)
    {
        // BuildingType::Headquarters sind Baustellen!!, da HQs ja sowieso nicht gebaut werden können
        if(bldType == BuildingType::Headquarters)
        {
            // Bei Baustellen die Extraliste abfragen
            for(noBuildingSite* bldSite : buildings.getBuildingSites())
            {
                // Optimization: Ignore if unconnected
                if(!bldSite->isConnected())
                    continue;

                unsigned points = bldSite->calcDistributionPoints(gt);
                if(!points)
                    continue;

                points += wareDistribution.percent_buildings[BuildingType::Headquarters] * 30;
                unsigned distance = world.calcDistance(start->getPos(), bldSite->getPos()) / 2;
                possibleClients.push_back(ClientForWare(bldSite, points > distance ? points - distance : 0, points));
            }
        } else
        {
            // Für übrige Gebäude
            for(nobUsual* bld : buildings.getBuildings(bldType))
            {
                // Optimization: Ignore if unconnected
                if(!bld->isConnected())
                    continue;

                unsigned points = bld->calcDistributionPoints(gt);
                if(!points)
                    continue; // Ware not needed

                if(!wareDistribution.goals.empty())
                {
                    if(bld->getBuildingType()
                       == static_cast<BuildingType>(wareDistribution.goals[wareDistribution.selected_goal]))
                        points += 300;
                    else if(points >= 300) // avoid overflows (async!)
                        points -= 300;
                    else
                        points = 0;
                }

                unsigned distance = world.calcDistance(start->getPos(), bld->getPos()) / 2;
                possibleClients.push_back(ClientForWare(bld, points > distance ? points - distance : 0, points));
            }
        }
    }

    // sort our clients, highest score first
    helpers::sort(possibleClients);

    noBaseBuilding* lastBld = nullptr;
    noBaseBuilding* bestBld = nullptr;
    unsigned best_points = 0;
    for(auto& possibleClient : possibleClients)
    {
        unsigned path_length;

        // If our estimate is worse (or equal) best_points, the real value cannot be better.
        // As our list is sorted, further entries cannot be better either, so stop searching.
        if(possibleClient.estimate <= best_points)
            break;

        // get rid of double building entries. TODO: why are there double entries!?
        if(possibleClient.bld == lastBld)
            continue;

        lastBld = possibleClient.bld;

        // Just to be sure no underflow happens...
        if(possibleClient.points < best_points + 1)
            continue;

        // Find path ONLY if it may be better. Pathfinding is limited to the worst path score that would lead to a
        // better score. This eliminates the worst case scenario where all nodes in a split road network would be hit by
        // the pathfinding only to conclude that there is no possible path.
        if(world.findPathForWareOnRoads(*start, *possibleClient.bld, &path_length, nullptr,
                                        (possibleClient.points - best_points) * 2 - 1)
           != RoadPathDirection::None)
        {
            unsigned score = possibleClient.points - (path_length / 2);

            // As we have limited our pathfinding to take a maximum of (points - best_points) * 2 - 1 steps,
            // path_length / 2 can at most be points - best_points - 1, so the score will be greater than best_points.
            // :)
            RTTR_Assert(score > best_points);

            best_points = score;
            bestBld = possibleClient.bld;
        }
    }

    if(bestBld && !wareDistribution.goals.empty())
        wareDistribution.selected_goal =
          (wareDistribution.selected_goal + 907) % unsigned(wareDistribution.goals.size());

    // Wenn kein Abnehmer gefunden wurde, muss es halt in ein Lagerhaus
    if(!bestBld)
        bestBld = findWarehouseForWare(ware);

    return bestBld;
}

nobBaseWarehouse* GamePlayer::findWarehouseForWare(const Ware& ware) const
{
    // Check whs that collect this ware
    nobBaseWarehouse* wh = findWarehouse(*ware.getLocation(), FW::CollectsWare(ware.type), true, true);
    // If there is none, check those that accept it
    if(!wh)
    {
        // First find the ones, that do not send it right away (IMPORTANT: This avoids sending a ware to the wh that is
        // sending the ware out)
        wh = findWarehouse(*ware.getLocation(), FW::AcceptsWareButNoSend(ware.type), true, true);
        // The others only if this fails
        if(!wh)
            wh = findWarehouse(*ware.getLocation(), FW::AcceptsWare(ware.type), true, true);
    }
    return wh;
}

template<class T_GetPriority, class T_Buildings>
typename T_Buildings::value_type GamePlayer::findClientImpl(const Ware& ware, T_GetPriority&& getPriority,
                                                            const T_Buildings& buildings) const
{
    // TODO(replay): Unify with FindClientForWare: Sort by estimated priority first for less path finding.
    // See PR #1720
    typename T_Buildings::value_type bestClient = nullptr;
    unsigned bestPoints = 0;
    const auto& wareLocation = *ware.getLocation();
    const MapPoint warePos = wareLocation.getPos();
    for(auto* bld : buildings)
    {
        // Optimization: Ignore if unconnected
        if(!bld->isConnected())
            continue;
        const auto points = getPriority(*bld);

        // Consider costs for reaching the building
        // We want only: points - pathCosts > bestPoints, i.e. 0 <= pathCosts < points - bestPoints
        // So only check if difference is strictly positive, which covers the points==0 case
        if(points > bestPoints)
        {
            const unsigned maxPathCosts = points - bestPoints - 1;
            const unsigned distance = world.calcDistance(warePos, bld->getPos());
            if(distance > maxPathCosts)
                continue;
            unsigned pathCosts;
            if(world.findPathForWareOnRoads(wareLocation, *bld, &pathCosts, nullptr, maxPathCosts)
               != RoadPathDirection::None)
            {
                RTTR_Assert(points > pathCosts && points - pathCosts > bestPoints);
                bestPoints = points - pathCosts;
                bestClient = bld;
            }
        }
    }
    return bestClient;
}

nobBaseMilitary* GamePlayer::findClientForCoin(const Ware& ware) const
{
    nobBaseMilitary* result = findClientImpl(
      ware, [](const nobMilitary& bld) { return bld.calcCoinsPoints(); }, buildings.getMilitaryBuildings());
    // Send to warehouse if no military building needs the coins
    if(!result)
        result = findWarehouseForWare(ware);

    return result;
}

nobBaseMilitary* GamePlayer::findClientForArmor(const Ware& ware) const
{
    nobBaseMilitary* result = findClientImpl(
      ware, [](const nobMilitary& bld) { return bld.calcArmorPoints(); }, buildings.getMilitaryBuildings());
    // Send to warehouse if no military building needs the coins
    if(!result)
        result = findWarehouseForWare(ware);

    return result;
}

unsigned GamePlayer::getBuidingSitePriority(const noBuildingSite* building_site)
{
    if(useCustomBuildOrder_)
    {
        // Spezielle Reihenfolge

        // Typ in der Reihenfolge suchen und Position als Priorität zurückgeben
        for(unsigned i = 0; i < build_order.size(); ++i)
        {
            if(building_site->getBuildingType() == build_order[i])
                return i;
        }
    } else
    {
        // Reihenfolge der Bauaufträge, also was zuerst in Auftrag gegeben wurde, wird zuerst gebaut
        unsigned i = 0;
        for(noBuildingSite* bldSite : buildings.getBuildingSites())
        {
            if(building_site == bldSite)
                return i;
            i++;
        }
    }

    LOG.write("GameClientPlayer::GetBuidingSitePriority: ERROR: Buildingsite or type of it not found in the list!\n");
    RTTR_Assert(false);
    // We may want to multiply this value so don't return the absolute max value
    return std::numeric_limits<unsigned>::max() / 1000;
}

void GamePlayer::convertTransportData(const TransportOrders& transport_data)
{
    for(const auto ware : helpers::EnumRange<GoodType>{})
        transportPrio[ware] = getTransportPrioFromOrdering(transport_data, ware);
}

bool GamePlayer::isAlly(const unsigned char playerId) const
{
    // Der Spieler ist ja auch zu sich selber verbündet
    if(getPlayerId() == playerId)
        return true;
    else
        return getPactState(PactType::TreatyOfAlliance, playerId) == PactState::Accepted;
}

bool GamePlayer::isAttackable(const unsigned char playerId) const
{
    // Verbündete dürfen nicht angegriffen werden
    if(isAlly(playerId))
        return false;
    else
        // Ansonsten darf bei bestehendem Nichtangriffspakt ebenfalls nicht angegriffen werden
        return getPactState(PactType::NonAgressionPact, playerId) != PactState::Accepted;
}

void GamePlayer::orderTroops(nobMilitary& goal, std::array<unsigned, NUM_SOLDIER_RANKS> counts,
                             unsigned total_max) const
{
    // Solange Lagerhäuser nach Soldaten absuchen, bis entweder keins mehr übrig ist oder alle Soldaten bestellt sind
    nobBaseWarehouse* wh;
    unsigned sum = 0;
    do
    {
        std::array<bool, NUM_SOLDIER_RANKS> desiredRanks;
        for(unsigned i = 0; i < NUM_SOLDIER_RANKS; i++)
            desiredRanks[i] = counts[i] > 0;

        wh = findWarehouse(goal, FW::HasAnyMatchingSoldier(desiredRanks), false, false);
        if(wh)
        {
            wh->orderTroops(goal, counts, total_max);
            sum = std::accumulate(counts.begin(), counts.end(), 0u);
        }
    } while(total_max && sum && wh);
}

void GamePlayer::regulateAllTroops()
{
    for(nobMilitary* milBld : buildings.getMilitaryBuildings())
        milBld->regulateTroops();
}

void GamePlayer::recalcMilitaryFlags()
{
    for(nobMilitary* milBld : buildings.getMilitaryBuildings())
        milBld->lookForEnemyBuildings(nullptr);
}

void GamePlayer::newSoldiersAvailable(const unsigned& soldier_count)
{
    RTTR_Assert(soldier_count > 0);
    // solange laufen lassen, bis soldier_count = 0, d.h. der Soldat irgendwohin geschickt wurde
    // Zuerst nach unbesetzten Militärgebäude schauen
    for(nobMilitary* milBld : buildings.getMilitaryBuildings())
    {
        if(milBld->isNewBuilt())
        {
            milBld->regulateTroops();
            // Used that soldier? Go out
            if(!soldier_count)
                return;
        }
    }

    // Als nächstes Gebäude in Grenznähe
    for(nobMilitary* milBld : buildings.getMilitaryBuildings())
    {
        if(milBld->getFrontierDistance() == FrontierDistance::Near)
        {
            milBld->regulateTroops();
            // Used that soldier? Go out
            if(!soldier_count)
                return;
        }
    }

    // Und den Rest ggf.
    for(nobMilitary* milBld : buildings.getMilitaryBuildings())
    {
        // already checked? -> skip
        if(milBld->getFrontierDistance() == FrontierDistance::Near || milBld->isNewBuilt())
            continue;
        milBld->regulateTroops();
        if(!soldier_count) // used the soldier?
            return;
    }
}

void GamePlayer::callFlagWorker(const MapPoint pt, const Job job)
{
    auto* flag = world.getSpecObj<noFlag>(pt);
    if(!flag)
        return;
    /// Find wh with given job type (e.g. geologist, scout, ...)
    nobBaseWarehouse* wh = findWarehouse(*flag, FW::HasFigure(job, true), false, false);

    if(wh)
        wh->orderJob(job, *flag, true);
}

bool GamePlayer::isFlagWorker(const nofFlagWorker* flagworker)
{
    return helpers::contains(flagworkers, flagworker);
}

void GamePlayer::flagDestroyed(noFlag* flag)
{
    // Alle durchgehen und ggf. sagen, dass sie keine Flagge mehr haben, wenn das ihre Flagge war, die zerstört wurde
    for(auto it = flagworkers.begin(); it != flagworkers.end();)
    {
        if((*it)->getFlag() == flag)
        {
            (*it)->lostWork();
            it = flagworkers.erase(it);
        } else
            ++it;
    }
}

void GamePlayer::refreshDefenderList()
{
    shouldSendDefenderList.clear();
    // Add as many true values as set in the settings, the rest will be false
    for(unsigned i = 0; i < MILITARY_SETTINGS_SCALE[2]; ++i)
        shouldSendDefenderList.push_back(i < militarySettings_[2]);
    // und ordentlich schütteln
    RANDOM_SHUFFLE2(shouldSendDefenderList, 0);
}

void GamePlayer::changeMilitarySettings(const MilitarySettings& military_settings)
{
    for(unsigned i = 0; i < military_settings.size(); ++i)
    {
        // Sicherstellen, dass im validen Bereich
        RTTR_Assert(military_settings[i] <= MILITARY_SETTINGS_SCALE[i]);
        this->militarySettings_[i] = military_settings[i];
    }
    /// Truppen müssen neu kalkuliert werden
    regulateAllTroops();
    /// Die Verteidigungsliste muss erneuert werden
    refreshDefenderList();
}

void GamePlayer::changeToolsSettings(const ToolSettings& tools_settings,
                                     const helpers::EnumArray<int8_t, Tool>& orderChanges)
{
    const bool settingsChanged = toolsSettings_ != tools_settings;
    toolsSettings_ = tools_settings;
    if(settingsChanged)
        world.getNotifications().publish(ToolNote(ToolNote::SettingsChanged, getPlayerId()));

    for(const auto tool : helpers::enumRange<Tool>())
    {
        tools_ordered[tool] = helpers::clamp(tools_ordered[tool] + orderChanges[tool], 0, 100);
        tools_ordered_delta[tool] -= orderChanges[tool];

        if(orderChanges[tool] != 0)
        {
            LOG.write(">> Committing an order of %1% for tool #%2%(%3%)\n", LogTarget::File) % (int)orderChanges[tool]
              % static_cast<unsigned>(tool) % _(WARE_NAMES[TOOL_TO_GOOD[tool]]);
            world.getNotifications().publish(ToolNote(ToolNote::OrderPlaced, getPlayerId()));
        }
    }
}

void GamePlayer::ChangeDistribution(const Distributions& distribution_settings)
{
    unsigned idx = 0;
    for(const DistributionMapping& mapping : distributionMap)
    {
        distribution[std::get<0>(mapping)].percent_buildings[std::get<1>(mapping)] = distribution_settings[idx++];
    }

    recalcDistribution();
}

void GamePlayer::ChangeBuildOrder(bool useCustomBuildOrder, const BuildOrders& order_data)
{
    this->useCustomBuildOrder_ = useCustomBuildOrder;
    this->build_order = order_data;
}

bool GamePlayer::shouldSendDefender()
{
    // Wenn wir schon am Ende sind, muss die Verteidgungsliste erneuert werden
    if(shouldSendDefenderList.empty())
        refreshDefenderList();

    bool result = shouldSendDefenderList.back();
    shouldSendDefenderList.pop_back();
    return result;
}

void GamePlayer::testDefeat()
{
    // Nicht schon besiegt?
    // Keine Militärgebäude, keine Lagerhäuser (HQ,Häfen) -> kein Land --> verloren
    if(!isDefeated_ && buildings.getMilitaryBuildings().empty() && buildings.getStorehouses().empty())
        Surrender();
}

const nobHQ* GamePlayer::getHQ() const
{
    return hqPos.isValid() ? getGameWorld().getSpecObj<nobHQ>(hqPos) : nullptr;
}

nobHQ* GamePlayer::getHQ()
{
    return const_cast<nobHQ*>(std::as_const(*this).getHQ());
}

void GamePlayer::Surrender()
{
    if(isDefeated_)
        return;

    isDefeated_ = true;

    // GUI Bescheid sagen
    if(world.getGameInterface())
        world.getGameInterface()->GI_PlayerDefeated(getPlayerId());
}

void GamePlayer::setStatisticValue(StatisticType type, unsigned value)
{
    statisticCurrentData[type] = value;
}

void GamePlayer::changeStatisticValue(StatisticType type, int change)
{
    RTTR_Assert(change >= 0 || statisticCurrentData[type] >= static_cast<unsigned>(-change));
    statisticCurrentData[type] += change;
}

void GamePlayer::increaseMerchandiseStatistic(GoodType type)
{
    // Einsortieren...
    switch(type)
    {
        case GoodType::Wood: statisticCurrentMerchandiseData[0]++; break;
        case GoodType::Boards: statisticCurrentMerchandiseData[1]++; break;
        case GoodType::Stones: statisticCurrentMerchandiseData[2]++; break;
        case GoodType::Fish:
        case GoodType::Bread:
        case GoodType::Meat: statisticCurrentMerchandiseData[3]++; break;
        case GoodType::Water: statisticCurrentMerchandiseData[4]++; break;
        case GoodType::Beer: statisticCurrentMerchandiseData[5]++; break;
        case GoodType::Coal: statisticCurrentMerchandiseData[6]++; break;
        case GoodType::IronOre: statisticCurrentMerchandiseData[7]++; break;
        case GoodType::Gold: statisticCurrentMerchandiseData[8]++; break;
        case GoodType::Iron: statisticCurrentMerchandiseData[9]++; break;
        case GoodType::Coins: statisticCurrentMerchandiseData[10]++; break;
        case GoodType::Tongs:
        case GoodType::Axe:
        case GoodType::Saw:
        case GoodType::PickAxe:
        case GoodType::Hammer:
        case GoodType::Shovel:
        case GoodType::Crucible:
        case GoodType::RodAndLine:
        case GoodType::Scythe:
        case GoodType::Cleaver:
        case GoodType::Rollingpin:
        case GoodType::Bow: statisticCurrentMerchandiseData[11]++; break;
        case GoodType::ShieldVikings:
        case GoodType::ShieldAfricans:
        case GoodType::ShieldRomans:
        case GoodType::ShieldJapanese:
        case GoodType::Sword: statisticCurrentMerchandiseData[12]++; break;
        case GoodType::Boat: statisticCurrentMerchandiseData[13]++; break;
        default: break;
    }
}

void GamePlayer::calcStatistics()
{
    // Waren aus der Inventur zählen
    statisticCurrentData[StatisticType::Merchandise] = 0;
    for(const auto i : helpers::enumRange<GoodType>())
        statisticCurrentData[StatisticType::Merchandise] += global_inventory[i];

    // Bevölkerung aus der Inventur zählen
    statisticCurrentData[StatisticType::Inhabitants] = 0;
    for(const auto i : helpers::enumRange<Job>())
        statisticCurrentData[StatisticType::Inhabitants] += global_inventory[i];

    // Militär aus der Inventur zählen
    statisticCurrentData[StatisticType::Military] =
      global_inventory.people[Job::Private] + global_inventory.people[Job::PrivateFirstClass] * 2
      + global_inventory.people[Job::Sergeant] * 3 + global_inventory.people[Job::Officer] * 4
      + global_inventory.people[Job::General] * 5;

    // Produktivität berechnen
    statisticCurrentData[StatisticType::Productivity] = buildings.calcAverageProductivity();

    // Total points for tournament games
    statisticCurrentData[StatisticType::Tournament] =
      statisticCurrentData[StatisticType::Military] + 3 * statisticCurrentData[StatisticType::Vanquished];
}

void GamePlayer::statisticStep()
{
    calcStatistics();

    // 15-min-Statistik ein Feld weiterschieben
    for(const auto i : helpers::enumRange<StatisticType>())
    {
        statistic[StatisticTime::T15Minutes].data[i][incrStatIndex(statistic[StatisticTime::T15Minutes].currentIndex)] =
          statisticCurrentData[i];
    }
    for(unsigned i = 0; i < NUM_STAT_MERCHANDISE_TYPES; ++i)
    {
        statistic[StatisticTime::T15Minutes]
          .merchandiseData[i][incrStatIndex(statistic[StatisticTime::T15Minutes].currentIndex)] =
          statisticCurrentMerchandiseData[i];
    }
    statistic[StatisticTime::T15Minutes].currentIndex =
      incrStatIndex(statistic[StatisticTime::T15Minutes].currentIndex);

    statistic[StatisticTime::T15Minutes].counter++;

    // Prüfen ob 4mal 15-min-Statistik weitergeschoben wurde, wenn ja: 1-h-Statistik weiterschieben
    // und aktuellen Wert der 15min-Statistik benutzen
    // gleiches für die 4h und 16h Statistik
    for(const auto t : helpers::enumRange<StatisticTime>())
    {
        if(t == StatisticTime(helpers::MaxEnumValue_v<StatisticTime>))
            break;
        const auto nextT = StatisticTime(rttr::enumCast(t) + 1);
        if(statistic[t].counter == 4)
        {
            statistic[t].counter = 0;
            for(const auto i : helpers::enumRange<StatisticType>())
            {
                statistic[nextT].data[i][incrStatIndex(statistic[nextT].currentIndex)] = statisticCurrentData[i];
            }

            // Summe für den Zeitraum berechnen (immer 4 Zeitschritte der jeweils kleineren Statistik)
            for(unsigned i = 0; i < NUM_STAT_MERCHANDISE_TYPES; ++i)
            {
                statistic[nextT].merchandiseData[i][incrStatIndex(statistic[nextT].currentIndex)] =
                  statisticCurrentMerchandiseData[i]
                  + statistic[t].merchandiseData[i][decrStatIndex(statistic[t].currentIndex, 1)]
                  + statistic[t].merchandiseData[i][decrStatIndex(statistic[t].currentIndex, 2)]
                  + statistic[t].merchandiseData[i][decrStatIndex(statistic[t].currentIndex, 3)];
            }

            statistic[nextT].currentIndex = incrStatIndex(statistic[nextT].currentIndex);
            statistic[nextT].counter++;
        }
    }

    // Warenstatistikzähler nullen
    statisticCurrentMerchandiseData.fill(0);
}

GamePlayer::Pact::Pact(SerializedGameData& sgd)
    : duration(sgd.popUnsignedInt()), start(sgd.popUnsignedInt()), accepted(sgd.popBool()), want_cancel(sgd.popBool())
{}

void GamePlayer::Pact::serialize(SerializedGameData& sgd) const
{
    sgd.pushUnsignedInt(duration);
    sgd.pushUnsignedInt(start);
    sgd.pushBool(accepted);
    sgd.pushBool(want_cancel);
}

void GamePlayer::pactChanged(const PactType pt)
{
    // Recheck military flags as the border (to an enemy) might have changed
    recalcMilitaryFlags();

    // Ggf. den GUI Bescheid sagen, um Sichtbarkeiten etc. neu zu berechnen
    if(pt == PactType::TreatyOfAlliance)
    {
        if(world.getGameInterface())
            world.getGameInterface()->GI_TreatyOfAllianceChanged(getPlayerId());
    }
}

void GamePlayer::SuggestPact(const unsigned char targetPlayerId, const PactType pt, const unsigned duration)
{
    // Don't try to make pact with self
    if(targetPlayerId == getPlayerId())
        return;

    if(!pacts[targetPlayerId][pt].accepted && duration > 0)
    {
        pacts[targetPlayerId][pt].duration = duration;
        pacts[targetPlayerId][pt].start = world.getEvMgr().getCurrentGF();
        GamePlayer targetPlayer = world.getPlayer(targetPlayerId);
        if(targetPlayer.isHuman())
            targetPlayer.sendPostMessage(std::make_unique<DiplomacyPostQuestion>(
              world.getEvMgr().getCurrentGF(), pt, pacts[targetPlayerId][pt].start, *this, duration));
        else if(world.hasLua())
            world.getLua().eventSuggestPact(pt, getPlayerId(), targetPlayerId, duration);
    }
}

void GamePlayer::AcceptPact(const unsigned id, const PactType pt, const unsigned char targetPlayer)
{
    if(!pacts[targetPlayer][pt].accepted && pacts[targetPlayer][pt].duration > 0 && pacts[targetPlayer][pt].start == id)
    {
        makePact(pt, targetPlayer, pacts[targetPlayer][pt].duration);
        world.getPlayer(targetPlayer).makePact(pt, getPlayerId(), pacts[targetPlayer][pt].duration);
        pactChanged(pt);
        world.getPlayer(targetPlayer).pactChanged(pt);
        if(world.hasLua())
            world.getLua().eventPactCreated(pt, getPlayerId(), targetPlayer, pacts[targetPlayer][pt].duration);
    }
}

void GamePlayer::makePact(const PactType pt, const unsigned char other_player, const unsigned duration)
{
    pacts[other_player][pt].accepted = true;
    pacts[other_player][pt].start = world.getEvMgr().getCurrentGF();
    pacts[other_player][pt].duration = duration;
    pacts[other_player][pt].want_cancel = false;

    sendPostMessage(
      std::make_unique<PostMsg>(world.getEvMgr().getCurrentGF(), pt, world.getPlayer(other_player), true));
}

PactState GamePlayer::getPactState(const PactType pt, const unsigned char other_player) const
{
    // Prüfen, ob Bündnis in Kraft ist
    if(pacts[other_player][pt].duration)
    {
        if(!pacts[other_player][pt].accepted)
            return PactState::InProgress;

        if(pacts[other_player][pt].duration == DURATION_INFINITE
           || world.getEvMgr().getCurrentGF() < pacts[other_player][pt].start + pacts[other_player][pt].duration)
            return PactState::Accepted;
    }

    return PactState::None;
}

void GamePlayer::NotifyAlliesOfLocation(const MapPoint pt)
{
    for(unsigned i = 0; i < world.getNumPlayers(); ++i)
    {
        if(i != getPlayerId() && isAlly(i))
            world.getPlayer(i).sendPostMessage(std::make_unique<PostMsg>(
              world.getEvMgr().getCurrentGF(), _("Your ally wishes to notify you of this location"),
              PostCategory::Diplomacy, pt));
    }
}

unsigned GamePlayer::getRemainingPactTime(const PactType pt, const unsigned char other_player) const
{
    if(pacts[other_player][pt].duration)
    {
        if(pacts[other_player][pt].accepted)
        {
            if(pacts[other_player][pt].duration == DURATION_INFINITE)
                return DURATION_INFINITE;
            else if(world.getEvMgr().getCurrentGF() <= pacts[other_player][pt].start + pacts[other_player][pt].duration)
                return ((pacts[other_player][pt].start + pacts[other_player][pt].duration)
                        - world.getEvMgr().getCurrentGF());
        }
    }

    return 0;
}

void GamePlayer::CancelPact(const PactType pt, const unsigned char otherPlayerIdx)
{
    // Don't try to cancel pact with self
    if(otherPlayerIdx == getPlayerId())
        return;

    // Besteht bereits ein Bündnis?
    if(pacts[otherPlayerIdx][pt].accepted)
    {
        // Vermerken, dass der Spieler das Bündnis auflösen will
        pacts[otherPlayerIdx][pt].want_cancel = true;

        // Will der andere Spieler das Bündnis auch auflösen?
        GamePlayer& otherPlayer = world.getPlayer(otherPlayerIdx);
        if(otherPlayer.pacts[getPlayerId()][pt].want_cancel)
        {
            // Dann wird das Bündnis aufgelöst
            pacts[otherPlayerIdx][pt].accepted = false;
            pacts[otherPlayerIdx][pt].duration = 0;
            pacts[otherPlayerIdx][pt].want_cancel = false;

            otherPlayer.pacts[getPlayerId()][pt].accepted = false;
            otherPlayer.pacts[getPlayerId()][pt].duration = 0;
            otherPlayer.pacts[getPlayerId()][pt].want_cancel = false;

            // Den Spielern eine Informationsnachricht schicken
            world.getPlayer(otherPlayerIdx)
              .sendPostMessage(std::make_unique<PostMsg>(world.getEvMgr().getCurrentGF(), pt, *this, false));
            sendPostMessage(
              std::make_unique<PostMsg>(world.getEvMgr().getCurrentGF(), pt, world.getPlayer(otherPlayerIdx), false));
            pactChanged(pt);
            otherPlayer.pactChanged(pt);
            if(world.hasLua())
                world.getLua().eventPactCanceled(pt, getPlayerId(), otherPlayerIdx);
        } else
        {
            // Ansonsten den anderen Spieler fragen, ob der das auch so sieht
            if(otherPlayer.isHuman())
                otherPlayer.sendPostMessage(std::make_unique<DiplomacyPostQuestion>(
                  world.getEvMgr().getCurrentGF(), pt, pacts[otherPlayerIdx][pt].start, *this));
            else if(!world.hasLua() || world.getLua().eventCancelPactRequest(pt, getPlayerId(), otherPlayerIdx))
            {
                // AI accepts cancels, if there is no lua-interace
                pacts[otherPlayerIdx][pt].accepted = false;
                pacts[otherPlayerIdx][pt].duration = 0;
                pacts[otherPlayerIdx][pt].want_cancel = false;

                otherPlayer.pacts[getPlayerId()][pt].accepted = false;
                otherPlayer.pacts[getPlayerId()][pt].duration = 0;
                otherPlayer.pacts[getPlayerId()][pt].want_cancel = false;

                if(world.hasLua())
                    world.getLua().eventPactCanceled(pt, getPlayerId(), otherPlayerIdx);
            }
        }
    } else
    {
        // Es besteht kein Bündnis, also unseren Bündnisvorschlag wieder zurücknehmen
        pacts[otherPlayerIdx][pt].duration = 0;
    }
}

void GamePlayer::makeStartPacts()
{
    // Reset pacts
    for(unsigned i = 0; i < world.getNumPlayers(); ++i)
    {
        for(const auto z : helpers::enumRange<PactType>())
            pacts[i][z] = Pact();
    }

    // No team -> No pacts
    if(team == Team::None)
        return;
    RTTR_Assert(isTeam(team));

    // Create ally- and non-aggression-pact for all players of same team
    for(unsigned i = 0; i < world.getNumPlayers(); ++i)
    {
        if(team != world.getPlayer(i).team)
            continue;
        for(const auto z : helpers::enumRange<PactType>())
        {
            pacts[i][z].duration = DURATION_INFINITE;
            pacts[i][z].start = 0;
            pacts[i][z].accepted = true;
            pacts[i][z].want_cancel = false;
        }
    }
}

bool GamePlayer::isWareRegistred(const Ware& ware)
{
    return helpers::contains(ware_list, &ware);
}

bool GamePlayer::isWareDependent(const Ware& ware)
{
    for(nobBaseWarehouse* wh : buildings.getStorehouses())
    {
        if(wh->isWareDependent(ware))
            return true;
    }

    return false;
}

void GamePlayer::increaseInventoryWare(const GoodType ware, const unsigned count)
{
    global_inventory.add(convertShields(ware), count);
}

void GamePlayer::decreaseInventoryWare(const GoodType ware, const unsigned count)
{
    global_inventory.remove(convertShields(ware), count);
}

void GamePlayer::registerShip(noShip& ship)
{
    ships.push_back(&ship);
    getJobForShip(ship);
}

struct ShipForHarbor
{
    noShip* ship;
    uint32_t estimate;

    ShipForHarbor(noShip* ship, uint32_t estimate) : ship(ship), estimate(estimate) {}

    bool operator<(const ShipForHarbor& b) const noexcept
    {
        return (estimate < b.estimate) || (estimate == b.estimate && ship->getObjId() < b.ship->getObjId());
    }
};

bool GamePlayer::orderShip(nobHarborBuilding& hb)
{
    std::vector<ShipForHarbor> sfh;

    // we need more ships than those that are already on their way? limit search to idle ships
    if(getShipsToHarbor(hb) < hb.getNumNeededShips())
    {
        for(noShip* ship : ships)
        {
            if(ship->isIdling() && world.isHarborAtSea(world.getHarborPointID(hb.getPos()), ship->getSeaID()))
                sfh.push_back(ShipForHarbor(ship, world.calcDistance(hb.getPos(), ship->getPos())));
        }
    } else
    {
        for(noShip* ship : ships)
        {
            if((ship->isIdling() && world.isHarborAtSea(world.getHarborPointID(hb.getPos()), ship->getSeaID()))
               || ship->isGoingToHarbor(hb))
            {
                sfh.push_back(ShipForHarbor(ship, world.calcDistance(hb.getPos(), ship->getPos())));
            }
        }
    }

    helpers::sort(sfh);

    noShip* best_ship = nullptr;
    uint32_t best_distance = std::numeric_limits<uint32_t>::max();
    std::vector<Direction> best_route;

    for(auto& it : sfh)
    {
        uint32_t distance;
        std::vector<Direction> route;

        // the estimate (air-line distance) for this and all other ships in the list is already worse than what we
        // found? disregard the rest
        if(it.estimate >= best_distance)
            break;

        noShip& ship = *it.ship;

        if(world.findShipPathToHarbor(ship.getPos(), hb.getHarborPosID(), ship.getSeaID(), &route, &distance))
        {
            // ship already there?
            if(distance == 0)
            {
                hb.shipArrived(ship);
                return true;
            }

            if(distance < best_distance)
            {
                best_ship = &ship;
                best_distance = distance;
                best_route = route;
            }
        }
    }

    // only order ships not already on their way
    if(best_ship && best_ship->isIdling())
    {
        best_ship->goToHarbor(hb, best_route);

        return true;
    }

    return false;
}

void GamePlayer::removeShip(noShip* ship)
{
    auto it = helpers::find(ships, ship);
    RTTR_Assert(it != ships.end());
    ships.erase(it);
}

void GamePlayer::getJobForShip(noShip& ship)
{
    nobHarborBuilding* best = nullptr;
    int best_points = 0;
    std::vector<Direction> best_route;

    // Beste Weglänge, die ein Schiff zurücklegen muss, welches gerade nichts zu tun hat
    for(nobHarborBuilding* harbor : buildings.getHarbors())
    {
        // Braucht der Hafen noch Schiffe?
        if(harbor->getNumNeededShips() == 0)
            continue;

        // Anzahl der Schiffe ermitteln, die diesen Hafen bereits anfahren
        unsigned ships_coming = getShipsToHarbor(*harbor);

        // Evtl. kommen schon genug?
        if(harbor->getNumNeededShips() <= ships_coming)
            continue;

        // liegen wir am gleichen Meer?
        if(world.isHarborAtSea(harbor->getHarborPosID(), ship.getSeaID()))
        {
            unsigned length;
            std::vector<Direction> route;

            if(world.findShipPathToHarbor(ship.getPos(), harbor->getHarborPosID(), ship.getSeaID(), &route, &length))
            {
                // ship already there?
                if(length == 0)
                {
                    harbor->shipArrived(ship);
                    return;
                }

                // Punkte ausrechnen
                int points = harbor->getNeedForShip(ships_coming) - length;
                if(points > best_points || !best)
                {
                    best = harbor;
                    best_points = points;
                    best_route = route;
                }
            }
        }
    }

    // Einen Hafen gefunden?
    if(best)
        ship.goToHarbor(*best, best_route);
}

unsigned GamePlayer::getShipID(const noShip& ship) const
{
    return static_cast<unsigned>(helpers::indexOf(ships, &ship));
}

noShip* GamePlayer::getShipByID(const unsigned ship_id) const
{
    if(ship_id >= ships.size())
        return nullptr;
    else
        return ships[ship_id];
}

unsigned GamePlayer::getShipsToHarbor(const nobHarborBuilding& hb) const
{
    unsigned count = 0;
    for(const auto* ship : ships)
    {
        if(ship->isGoingToHarbor(hb))
            ++count;
    }

    return count;
}

bool GamePlayer::findHarborForUnloading(noShip* ship, const MapPoint start, HarborId* goalHarborId,
                                        std::vector<Direction>* route, nobHarborBuilding* exception)
{
    nobHarborBuilding* best = nullptr;
    unsigned best_distance = 0xffffffff;

    for(nobHarborBuilding* hb : buildings.getHarbors())
    {
        // Bestimmten Hafen ausschließen
        if(hb == exception)
            continue;

        // Prüfen, ob Hafen an das Meer, wo sich das Schiff gerade befindet, angrenzt
        if(!world.isHarborAtSea(hb->getHarborPosID(), ship->getSeaID()))
            continue;

        // Distanz ermitteln zwischen Schiff und Hafen, Schiff kann natürlich auch über Kartenränder fahren
        unsigned distance = world.calcDistance(ship->getPos(), hb->getPos());

        // Kürzerer Weg als bisher bestes Ziel?
        if(distance < best_distance)
        {
            best_distance = distance;
            best = hb;
        }
    }

    // Hafen gefunden?
    if(best)
    {
        // Weg dorthin suchen
        route->clear();
        *goalHarborId = best->getHarborPosID();
        if(world.findShipPathToHarbor(start, best->getHarborPosID(), ship->getSeaID(), route, nullptr))
            return true;
    }

    return false;
}

void GamePlayer::cancelWaresForEmergencyProtocol()
{
    for(auto it = ware_list.begin(); it != ware_list.end();)
    {
        Ware* ware = *it;
        if(ware->isWaitingInWarehouse() && ware->getGoal()
           && !isWareFineWithEmergencyProtocol(ware->type, *ware->getGoal()))
        {
            ware->notifyGoalAboutLostWare();
            static_cast<nobBaseWarehouse*>(ware->getLocation())->cancelWare(ware);
            it = ware_list.erase(it);
            continue;
        }
        it++;
    }
}

void GamePlayer::testForEmergencyProgramm()
{
    // we are already defeated, do not even think about an emergency program - it's too late :-(
    if(isDefeated_)
        return;

    // In Lagern vorhandene Bretter und Steine zählen
    unsigned boards = 0;
    unsigned stones = 0;
    for(nobBaseWarehouse* wh : buildings.getStorehouses())
    {
        boards += wh->getInventory().goods[GoodType::Boards];
        stones += wh->getInventory().goods[GoodType::Stones];
    }

    // Emergency happens, if we have less than 10 boards or stones...
    bool isNewEmergency = boards <= 10 || stones <= 10;
    // ...and no woddcutter or sawmill
    isNewEmergency &=
      buildings.getBuildings(BuildingType::Woodcutter).empty() || buildings.getBuildings(BuildingType::Sawmill).empty();

    // Wenn nötig, Notfallprogramm auslösen
    if(isNewEmergency)
    {
        if(!emergency)
        {
            emergency = true;
            sendPostMessage(std::make_unique<PostMsg>(
              world.getEvMgr().getCurrentGF(), _("The emergency program has been activated."), PostCategory::Economy));

            // Handle wares already ordered
            cancelWaresForEmergencyProtocol();
        }
    } else
    {
        // Sobald Notfall vorbei, Notfallprogramm beenden, evtl. Baustellen wieder mit Kram versorgen
        if(emergency)
        {
            emergency = false;
            sendPostMessage(std::make_unique<PostMsg>(world.getEvMgr().getCurrentGF(),
                                                      _("The emergency program has been deactivated."),
                                                      PostCategory::Economy));
            findMaterialForBuildingSites();
        }
    }
}

void GamePlayer::testPacts()
{
    for(unsigned i = 0; i < world.getNumPlayers(); ++i)
    {
        if(i == getPlayerId())
            continue;

        for(const auto pact : helpers::enumRange<PactType>())
        {
            // Pact not running
            if(pacts[i][pact].duration == 0)
                continue;
            if(getPactState(pact, i) == PactState::None)
            {
                // Pact was running but is expired -> Cancel for both players
                pacts[i][pact].duration = 0;
                pacts[i][pact].accepted = false;
                GamePlayer& otherPlayer = world.getPlayer(i);
                RTTR_Assert(otherPlayer.pacts[getPlayerId()][pact].duration);
                RTTR_Assert(otherPlayer.pacts[getPlayerId()][pact].accepted);
                otherPlayer.pacts[getPlayerId()][pact].duration = 0;
                otherPlayer.pacts[getPlayerId()][pact].accepted = false;
                // And notify
                pactChanged(pact);
                otherPlayer.pactChanged(pact);
            }
        }
    }
}

bool GamePlayer::canBuildCatapult() const
{
    // Wenn AddonId::LIMIT_CATAPULTS nicht aktiv ist, bauen immer erlaubt
    if(!world.getGGS().isEnabled(AddonId::LIMIT_CATAPULTS)) //-V807
        return true;

    BuildingCount bc = buildings.getBuildingNums();

    unsigned max = 0;
    // proportional?
    if(world.getGGS().getSelection(AddonId::LIMIT_CATAPULTS) == 1)
    {
        max = int(bc.buildings[BuildingType::Barracks] * 0.125 + bc.buildings[BuildingType::Guardhouse] * 0.25
                  + bc.buildings[BuildingType::Watchtower] * 0.5 + bc.buildings[BuildingType::Fortress]
                  + 0.111); // to avoid rounding errors
    } else if(world.getGGS().getSelection(AddonId::LIMIT_CATAPULTS) < 8)
    {
        const std::array<unsigned, 6> limits = {{0, 3, 5, 10, 20, 30}};
        max = limits[world.getGGS().getSelection(AddonId::LIMIT_CATAPULTS) - 2];
    }

    return bc.buildings[BuildingType::Catapult] + bc.buildingSites[BuildingType::Catapult] < max;
}

bool GamePlayer::shipDiscoveredHostileTerritory(const MapPoint location)
{
    // Prüfen, ob Abstand zu bisherigen Punkten nicht zu klein
    for(const auto& enemies_discovered_by_ship : enemies_discovered_by_ships)
    {
        if(world.calcDistance(enemies_discovered_by_ship, location) < 30)
            return false;
    }

    // Nein? Dann haben wir ein neues Territorium gefunden
    enemies_discovered_by_ships.push_back(location);

    return true;
}

bool GamePlayer::isDependentFigure(const noFigure& fig)
{
    for(const nobBaseWarehouse* wh : buildings.getStorehouses())
    {
        if(wh->isDependentFigure(fig))
            return true;
    }
    return false;
}

std::vector<nobBaseWarehouse*> GamePlayer::getWarehousesForTrading(const nobBaseWarehouse& goalWh) const
{
    std::vector<nobBaseWarehouse*> result;

    // Don't try to trade with us!
    if(goalWh.getPlayer() == getPlayerId())
        return result;

    const MapPoint goalFlagPos = goalWh.getFlagPos();

    TradePathCache& tradePathCache = world.getTradePathCache();
    for(nobBaseWarehouse* wh : buildings.getStorehouses())
    {
        // Is there a trade path from this warehouse to wh? (flag to flag)
        if(tradePathCache.pathExists(wh->getFlagPos(), goalFlagPos, getPlayerId()))
            result.push_back(wh);
    }

    return result;
}

struct WarehouseDistanceComparator
{
    // Reference warehouse position, to which we want to calc the distance
    const MapPoint refWareHousePos_;
    /// GameWorld
    const GameWorld& gwg_;

    WarehouseDistanceComparator(const nobBaseWarehouse& refWareHouse, const GameWorld& world)
        : refWareHousePos_(refWareHouse.getPos()), gwg_(world)
    {}

    bool operator()(nobBaseWarehouse* const wh1, nobBaseWarehouse* const wh2) const
    {
        unsigned dist1 = gwg_.calcDistance(wh1->getPos(), refWareHousePos_);
        unsigned dist2 = gwg_.calcDistance(wh2->getPos(), refWareHousePos_);
        return (dist1 < dist2) || (dist1 == dist2 && wh1->getObjId() < wh2->getObjId());
    }
};

void GamePlayer::trade(nobBaseWarehouse* goalWh, const boost_variant2<GoodType, Job>& what, unsigned count) const
{
    if(!world.getGGS().isEnabled(AddonId::TRADE))
        return;

    if(count == 0)
        return;

    // Don't try to trade with us!
    if(goalWh->getPlayer() == getPlayerId())
        return;

    // No trades with enemies
    if(!isAlly(goalWh->getPlayer()))
        return;

    const MapPoint goalFlagPos = goalWh->getFlagPos();

    std::vector<nobBaseWarehouse*> whs(buildings.getStorehouses().begin(), buildings.getStorehouses().end());
    helpers::sort(whs, WarehouseDistanceComparator(*goalWh, world));
    TradePathCache& tradePathCache = world.getTradePathCache();
    for(nobBaseWarehouse* wh : whs)
    {
        // Get available wares
        const unsigned available =
          boost::variant2::visit(composeVisitor([wh](GoodType gt) { return wh->getAvailableWaresForTrading(gt); },
                                                [wh](Job job) { return wh->getAvailableFiguresForTrading(job); }),
                                 what);
        if(available == 0)
            continue;

        const unsigned actualCount = std::min(available, count);

        // Find a trade path from flag to flag
        TradeRoute tr(world, getPlayerId(), wh->getFlagPos(), goalFlagPos);

        // Found a path?
        if(tr.isValid())
        {
            // Add to cache for future searches
            tradePathCache.addEntry(tr.getTradePath(), getPlayerId());

            wh->startTradeCaravane(what, actualCount, tr, goalWh);
            count -= actualCount;
            if(count == 0)
                return;
        }
    }
}

bool GamePlayer::isBuildingEnabled(BuildingType type) const
{
    // No game interface without a GUI (headless runs, tests): no cheats either
    return building_enabled[type]
           || (isHuman() && world.getGameInterface()
               && world.getGameInterface()->GI_GetCheats().areAllBuildingsEnabled());
}

void GamePlayer::fillVisualSettings(VisualSettings& visualSettings) const
{
    Distributions& visDistribution = visualSettings.distribution;
    unsigned visIdx = 0;
    for(const DistributionMapping& mapping : distributionMap)
    {
        visDistribution[visIdx++] = distribution[std::get<0>(mapping)].percent_buildings[std::get<1>(mapping)];
    }

    visualSettings.useCustomBuildOrder = useCustomBuildOrder_;
    visualSettings.build_order = build_order;

    visualSettings.transport_order = getOrderingFromTransportPrio(transportPrio);

    visualSettings.military_settings = militarySettings_;
    visualSettings.tools_settings = toolsSettings_;
}

#define INSTANTIATE_FINDWH(Cond)                                                                                \
    template nobBaseWarehouse* GamePlayer::findWarehouse(const noRoadNode&, const Cond&, bool, bool, unsigned*, \
                                                         const RoadSegment*) const

INSTANTIATE_FINDWH(FW::HasMinWares);
INSTANTIATE_FINDWH(FW::HasFigure);
INSTANTIATE_FINDWH(FW::HasWareAndFigure);
INSTANTIATE_FINDWH(FW::HasAnyMatchingSoldier);
INSTANTIATE_FINDWH(FW::AcceptsWare);
INSTANTIATE_FINDWH(FW::AcceptsFigure);
INSTANTIATE_FINDWH(FW::CollectsWare);
INSTANTIATE_FINDWH(FW::CollectsFigure);
INSTANTIATE_FINDWH(FW::HasWareButNoCollect);
INSTANTIATE_FINDWH(FW::HasFigureButNoCollect);
INSTANTIATE_FINDWH(FW::AcceptsFigureButNoSend);
INSTANTIATE_FINDWH(FW::NoCondition);

#undef INSTANTIATE_FINDWH
