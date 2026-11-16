// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AIPlayerJH.h"
#include "AIConstruction.h"
#include "BuildingPlanner.h"
#include "FindWhConditions.h"
#include "GamePlayer.h"
#include "Jobs.h"
#include "RttrForeachPt.h"
#include "addons/const_addons.h"
#include "ai/AIEvents.h"
#include "ai/random.h"
#include "boost/filesystem/fstream.hpp"
#include "buildings/noBuildingSite.h"
#include "buildings/nobHarborBuilding.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobUsual.h"
#include "figures/nofHunter.h"
#include "helpers/EnumArray.h"
#include "helpers/IdRange.h"
#include "helpers/MaxEnumValue.h"
#include "helpers/containerUtils.h"
#include "network/GameMessages.h"
#include "notifications/BuildingNote.h"
#include "notifications/ExpeditionNote.h"
#include "notifications/NodeNote.h"
#include "notifications/ResourceNote.h"
#include "notifications/RoadNote.h"
#include "notifications/ShipNote.h"
#include "pathfinding/PathConditionRoad.h"
#include "nodeObjs/noAnimal.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noShip.h"
#include "nodeObjs/noTree.h"
#include "gameTypes/MineResourceBehavior.h"
#include "gameData/BuildingConsts.h"
#include "gameData/BuildingProperties.h"
#include "gameData/JobConsts.h"
#include "gameData/TerrainDesc.h"
#include "gameData/ToolConsts.h"
#include <algorithm>
#include <array>
#include <memory>
#include <stdexcept>
#include <type_traits>

namespace {
void HandleBuildingNote(AIEventManager& eventMgr, const BuildingNote& note)
{
    std::unique_ptr<AIEvent::Base> ev;
    using namespace AIEvent;
    switch(note.type)
    {
        case BuildingNote::Constructed:
            ev = std::make_unique<AIEvent::Building>(EventType::BuildingFinished, note.pos, note.bld);
            break;
        case BuildingNote::Destroyed:
            ev = std::make_unique<AIEvent::Building>(EventType::BuildingDestroyed, note.pos, note.bld);
            break;
        case BuildingNote::Captured:
            ev = std::make_unique<AIEvent::Building>(EventType::BuildingConquered, note.pos, note.bld);
            break;
        case BuildingNote::Lost:
            ev = std::make_unique<AIEvent::Building>(EventType::BuildingLost, note.pos, note.bld);
            break;
        case BuildingNote::LostLand:
            ev = std::make_unique<AIEvent::Building>(EventType::LostLand, note.pos, note.bld);
            break;
        case BuildingNote::NoRessources:
            ev = std::make_unique<AIEvent::Building>(EventType::NoMoreResourcesReachable, note.pos, note.bld);
            break;
        case BuildingNote::LuaOrder:
            ev = std::make_unique<AIEvent::Building>(EventType::LuaConstructionOrder, note.pos, note.bld);
            break;
        default: RTTR_Assert(false); return;
    }
    eventMgr.addAIEvent(std::move(ev));
}
void HandleExpeditionNote(AIEventManager& eventMgr, const ExpeditionNote& note)
{
    switch(note.type)
    {
        case ExpeditionNote::Waiting:
            eventMgr.addAIEvent(std::make_unique<AIEvent::Location>(AIEvent::EventType::ExpeditionWaiting, note.pos));
            break;
        case ExpeditionNote::ColonyFounded:
            eventMgr.addAIEvent(std::make_unique<AIEvent::Location>(AIEvent::EventType::NewColonyFounded, note.pos));
            break;
    }
}
void HandleResourceNote(AIEventManager& eventMgr, const ResourceNote& note)
{
    eventMgr.addAIEvent(std::make_unique<AIEvent::Resource>(AIEvent::EventType::ResourceFound, note.pos, note.res));
}
void HandleRoadNote(AIEventManager& eventMgr, const RoadNote& note)
{
    switch(note.type)
    {
        case RoadNote::Constructed:
            eventMgr.addAIEvent(std::make_unique<AIEvent::Direction>(AIEvent::EventType::RoadConstructionComplete,
                                                                     note.pos, note.route.front()));
            break;
        case RoadNote::ConstructionFailed:
            eventMgr.addAIEvent(std::make_unique<AIEvent::Direction>(AIEvent::EventType::RoadConstructionFailed,
                                                                     note.pos, note.route.front()));
            break;
    }
}
void HandleShipNote(AIEventManager& eventMgr, const ShipNote& note)
{
    if(note.type == ShipNote::Constructed)
        eventMgr.addAIEvent(std::make_unique<AIEvent::Location>(AIEvent::EventType::ShipBuilt, note.pos));
}
} // namespace

namespace AIJH {

Subscription recordBQsToUpdate(const GameWorldBase& gw, std::vector<MapPoint>& bqsToUpdate)
{
    auto addToBqsToUpdate = [&bqsToUpdate](const MapPoint pt, unsigned) {
        bqsToUpdate.push_back(pt);
        return false;
    };
    return gw.getNotifications().subscribe<NodeNote>([&gw, addToBqsToUpdate](const NodeNote& note) {
        if(note.type == NodeNote::BQ)
        {
            // Need to check surrounding nodes for possible/impossible flags (e.g. near border)
            gw.checkPointsInRadius(note.pos, 1, addToBqsToUpdate, true);
        } else if(note.type == NodeNote::Owner)
        {
            // Owner changes border, which changes where buildings can be placed next to it
            // And as flags are need for buildings we need range 2 (e.g. range 1 is flag, range 2 building)
            gw.checkPointsInRadius(note.pos, 2, addToBqsToUpdate, true);
        }
    });
}

static bool isUnlimitedResource(const AIResource res, const GlobalGameSettings& ggs)
{
    switch(res)
    {
        case AIResource::Gold: return !isMineResourceDepletable(ggs, BuildingType::GoldMine);
        case AIResource::Ironore: return !isMineResourceDepletable(ggs, BuildingType::IronMine);
        case AIResource::Coal: return !isMineResourceDepletable(ggs, BuildingType::CoalMine);
        case AIResource::Granite: return !isMineResourceDepletable(ggs, BuildingType::GraniteMine);
        case AIResource::Fish: return ggs.isEnabled(AddonId::INEXHAUSTIBLE_FISH);
        default: return false;
    }
}

// Needed because AIResourceMap is not default initializable
template<size_t... I>
static auto createResourceMaps(const AIInterface& aii, const AIMap& aiMap, std::index_sequence<I...>)
{
    return helpers::EnumArray<AIResourceMap, AIResource>{AIResourceMap(
      static_cast<AIResource>(I), isUnlimitedResource(static_cast<AIResource>(I), aii.gwb.getGGS()), aii, aiMap)...};
}
static auto createResourceMaps(const AIInterface& aii, const AIMap& aiMap)
{
    return createResourceMaps(aii, aiMap, std::make_index_sequence<helpers::NumEnumValues_v<AIResource>>{});
}

AIPlayerJH::AIPlayerJH(const unsigned char playerId, const GameWorldBase& gwb, const AI::Level level)
    : AIPlayer(playerId, gwb, level), UpgradeBldPos(MapPoint::invalid()), resourceMaps(createResourceMaps(aii, aiMap)),
      isInitGfCompleted(false), defeated(player.IsDefeated()), bldPlanner(std::make_unique<BuildingPlanner>(*this)),
      construction(std::make_unique<AIConstruction>(*this))
{
    initNodes();
    initResourceMaps();
#ifdef DEBUG_AI
    saveResourceMapsToFile();
#endif

    switch(level)
    {
        case AI::Level::Easy:
            attack_interval = 2500;
            build_interval = 1000;
            break;
        case AI::Level::Medium:
            attack_interval = 750;
            build_interval = 400;
            break;
        case AI::Level::Hard:
            attack_interval = 100;
            build_interval = 200;
            break;
        default: throw std::invalid_argument("Invalid AI level!");
    }
    // TODO: Maybe remove the AIEvents where possible and call the handler functions directly
    NotificationManager& notifications = gwb.getNotifications();
    subBuilding = notifications.subscribe<BuildingNote>([this, playerId](const BuildingNote& note) {
        if(note.player == playerId)
            HandleBuildingNote(eventManager, note);
    });
    subExpedition = notifications.subscribe<ExpeditionNote>([this, playerId](const ExpeditionNote& note) {
        if(note.player == playerId)
            HandleExpeditionNote(eventManager, note);
    });
    subResource = notifications.subscribe<ResourceNote>([this, playerId](const ResourceNote& note) {
        if(note.player == playerId)
            HandleResourceNote(eventManager, note);
    });
    subRoad = notifications.subscribe<RoadNote>([this, playerId](const RoadNote& note) {
        if(note.player == playerId)
            HandleRoadNote(eventManager, note);
    });
    subShip = notifications.subscribe<ShipNote>([this, playerId](const ShipNote& note) {
        if(note.player == playerId)
            HandleShipNote(eventManager, note);
    });
    subBQ = recordBQsToUpdate(this->gwb, this->nodesWithOutdatedBQ);
}

AIPlayerJH::~AIPlayerJH() = default;

/// Wird jeden GF aufgerufen und die KI kann hier entsprechende Handlungen vollziehen
void AIPlayerJH::runGF(const unsigned gf, bool gfisnwf)
{
    if(defeated)
        return;

    if(testDefeat())
        return;
    if(!isInitGfCompleted)
    {
        initStoreAndMilitarylists();
        initDistribution();
    }
    if(isInitGfCompleted < 10)
    {
        isInitGfCompleted++;
        return; //  1 init -> 2 test defeat -> 3 do other ai stuff -> goto 2
    }
    if(gf == 100)
    {
        if(aii.getMilitaryBuildings().empty() && aii.getStorehouses().size() < 2)
            aii.chat(_("Hi, I'm an artifical player and I'm not very good yet!"));
    }

    if(!nodesWithOutdatedBQ.empty())
    {
        helpers::makeUnique(nodesWithOutdatedBQ, MapPointLess());
        for(const MapPoint pt : nodesWithOutdatedBQ)
            aiMap[pt].bq = aii.getBuildingQuality(pt);
        nodesWithOutdatedBQ.clear();
    }

    bldPlanner->update(gf, *this);

    if(gfisnwf) // nwf -> now the orders have been executed -> new constructions can be started
        construction->constructionsExecuted();

    // LOG.write(("ai doing stuff %i \n",playerId);
    if(gf % 100 == 0)
        bldPlanner->updateBuildingsWanted(*this);
    executeAIJob();

    if((gf + playerId * 17) % attack_interval == 0)
    {
        // CheckExistingMilitaryBuildings();
        tryToAttack();
    }
    if(((gf + playerId * 17) % 73 == 0) && (level != AI::Level::Easy))
    {
        milUpgradeOptim();
    }

    if((gf + 41 + playerId * 17) % attack_interval == 0)
    {
        if(ggs.isEnabled(AddonId::SEA_ATTACK))
            trySeaAttack();
    }

    if((gf + playerId * 13) % 1500 == 0)
    {
        checkExpeditions();
        checkForester();
        checkGranitMine();
    }

    if((gf + playerId * 11) % 150 == 0)
    {
        adjustSettings();
        // check for useless sawmills
        const std::list<nobUsual*>& sawMills = aii.getBuildings(BuildingType::Sawmill);
        if(sawMills.size() > 3)
        {
            int burns = 0;
            for(const nobUsual* sawmill : sawMills)
            {
                if(sawmill->getProductivity() < 1 && sawmill->hasWorker() && sawmill->getNumWares(0) < 1
                   && (sawMills.size() - burns) > 3 && !sawmill->areThereAnyOrderedWares())
                {
                    aii.DestroyBuilding(sawmill);
                    removeUnusedRoad(*sawmill->getFlag(), Direction::NorthWest, true);
                    burns++;
                }
            }
        }
    }

    if((gf + playerId * 7) % build_interval == 0) // plan new buildings
    {
        checkForUnconnectedBuildingSites();
        planNewBuildings(gf);
    }
}

void AIPlayerJH::onChatMessage(unsigned /*sendPlayerId*/, ChatDestination, const std::string& /*msg*/) {}

void AIPlayerJH::planNewBuildings(const unsigned gf)
{
    bldPlanner->updateBuildingsWanted(*this);

    // pick a random storehouse and try to build one of these buildings around it (checks if we actually want more of
    // the building type)
    std::array<BuildingType, 24> bldToTest = {
      {BuildingType::HarborBuilding, BuildingType::Shipyard,   BuildingType::Sawmill,
       BuildingType::Forester,       BuildingType::Farm,       BuildingType::Fishery,
       BuildingType::Woodcutter,     BuildingType::Quarry,     BuildingType::GoldMine,
       BuildingType::IronMine,       BuildingType::CoalMine,   BuildingType::GraniteMine,
       BuildingType::Hunter,         BuildingType::Charburner, BuildingType::Ironsmelter,
       BuildingType::Mint,           BuildingType::Armory,     BuildingType::Metalworks,
       BuildingType::Brewery,        BuildingType::Mill,       BuildingType::PigFarm,
       BuildingType::Slaughterhouse, BuildingType::Bakery,     BuildingType::DonkeyBreeder}};
    const unsigned numResGatherBlds = 14; /* The first n buildings in the above list, that gather resources */

    // LOG.write(("new buildorders %i whs and %i mil for player %i
    // \n",aii.GetStorehouses().size(),aii.GetMilitaryBuildings().size(),playerId);

    const std::list<nobBaseWarehouse*>& storehouses = aii.getStorehouses();
    if(!storehouses.empty())
    {
        // collect swords,shields,helpers,privates and beer in first storehouse or whatever is closest to the
        // upgradebuilding if we have one!
        nobBaseWarehouse* wh = getUpgradeBuildingWarehouse();
        setGatheringForUpgradeWarehouse(wh);

        if(ggs.getMaxMilitaryRank() > 0) // there is more than 1 rank available -> distribute
            distributeMaxRankSoldiersByBlocking(5, wh);
        // 30 boards amd 50 stones for each warehouse - block after that - should speed up expansion and limit losses in
        // case a warehouse is destroyed unlimited when every warehouse has at least that amount
        distributeGoodsByBlocking(GoodType::Boards, 30);
        distributeGoodsByBlocking(GoodType::Stones, 50);
        // go to the picked random warehouse and try to build around it
        const auto* storehouse = AI::randomElement(storehouses);
        const MapPoint whPos = storehouse->getPos();
        updateNodesAround(whPos, 15); // update the area we want to build in first
        for(const BuildingType i : bldToTest)
        {
            if(construction->wanted(i))
            {
                addBuildJobAroundEveryWarehouse(i); // add a buildorder for the picked buildingtype at every warehouse
            }
        }
        if(gf > 1500 || aii.getInventory().goods[GoodType::Boards] > 11)
            addMilitaryBuildJob(whPos);
    }
    // end of construction around & orders for warehouses

    // now pick a random military building and try to build around that as well
    const std::list<nobMilitary*>& militaryBuildings = aii.getMilitaryBuildings();
    if(militaryBuildings.empty())
        return;
    const auto* randomMiliBld = AI::randomElement(militaryBuildings);
    MapPoint bldPos = randomMiliBld->getPos();
    updateNodesAround(bldPos, 15);
    // resource gathering buildings only around military; processing only close to warehouses
    for(unsigned i = 0; i < numResGatherBlds; i++)
    {
        if(construction->wanted(bldToTest[i]))
        {
            addBuildJobAroundEveryMilBld(bldToTest[i]);
        }
    }
    addMilitaryBuildJob(bldPos);
    if(randomMiliBld->isUseless() && randomMiliBld->isDemolitionAllowed() && randomMiliBld != updateUpgradeBuilding())
    {
        aii.DestroyBuilding(bldPos);
    }
}

bool AIPlayerJH::testDefeat()
{
    if(isInitGfCompleted >= 10 && aii.getStorehouses().empty())
    {
        // LOG.write(("ai defeated player %i \n",playerId);
        defeated = true;
        aii.Surrender();
        aii.chat(_("You win"));
        return true;
    }
    return false;
}

unsigned AIPlayerJH::getNumJobs() const
{
    return eventManager.getEventNum() + construction->getBuildJobNum() + construction->getConnectJobNum();
}

/// returns the warehouse closest to the upgrade building or if it cant find a way the first warehouse and if there is
/// no warehouse left null
nobBaseWarehouse* AIPlayerJH::getUpgradeBuildingWarehouse()
{
    const std::list<nobBaseWarehouse*>& storehouses = aii.getStorehouses();
    if(storehouses.empty())
        return nullptr;
    const auto* upgradeBld = updateUpgradeBuilding();

    if(upgradeBld) // upgrade building exists -> find warehouse closest to the upgrade building,
                   // gather stuff there and deactivate gathering in the previous one
    {
        // which warehouse is closest to the upgrade building? -> train troops there and block max ranks
        auto* wh = aii.findWarehouse(*upgradeBld, FW::NoCondition(), false, false);
        if(wh)
            return wh;
    }
    return storehouses.front();
}

void AIPlayerJH::addMilitaryBuildJob(MapPoint pt)
{
    const auto milBld = construction->chooseMilitaryBuilding(pt);
    if(milBld)
        addBuildJob(*milBld, pt);
}

void AIPlayerJH::addBuildJob(BuildingType type, const MapPoint pt, bool front, bool searchPosition)
{
    construction->addBuildJob(
      std::make_unique<BuildJob>(*this, type, pt, searchPosition ? SearchMode::Radius : SearchMode::None), front);
}

void AIPlayerJH::addBuildJobAroundEveryWarehouse(BuildingType bt)
{
    for(const nobBaseWarehouse* wh : aii.getStorehouses())
    {
        addBuildJob(bt, wh->getPos(), false);
    }
}

void AIPlayerJH::addBuildJobAroundEveryMilBld(BuildingType bt)
{
    for(const nobMilitary* milBld : aii.getMilitaryBuildings())
    {
        addBuildJob(bt, milBld->getPos(), false);
    }
}

void AIPlayerJH::setGatheringForUpgradeWarehouse(nobBaseWarehouse* upgradewarehouse)
{
    for(const nobBaseWarehouse* wh : aii.getStorehouses())
    {
        // deactivate gathering for all warehouses that are NOT the one next to the upgradebuilding
        const MapPoint whPos = wh->getPos();
        if(upgradewarehouse->getPos() != whPos)
        {
            if(wh->isInventorySetting(GoodType::Beer, EInventorySetting::Collect)) // collecting beer? -> stop it
                aii.SetInventorySetting(whPos, GoodType::Beer, InventorySetting());

            if(wh->isInventorySetting(GoodType::Sword, EInventorySetting::Collect)) // collecting swords? -> stop it
                aii.SetInventorySetting(whPos, GoodType::Sword, InventorySetting());

            if(wh->isInventorySetting(GoodType::ShieldRomans,
                                      EInventorySetting::Collect)) // collecting shields? -> stop it
                aii.SetInventorySetting(whPos, GoodType::ShieldRomans, InventorySetting());

            if(wh->isInventorySetting(Job::Private, EInventorySetting::Collect)) // collecting privates? -> stop it
                aii.SetInventorySetting(whPos, Job::Private, InventorySetting());

            if(wh->isInventorySetting(Job::Helper, EInventorySetting::Collect)) // collecting helpers? -> stop it
                aii.SetInventorySetting(whPos, Job::Helper, InventorySetting());
        } else // activate gathering in the closest warehouse
        {
            if(!wh->isInventorySetting(GoodType::Beer, EInventorySetting::Collect)) // not collecting beer? -> start it
                aii.SetInventorySetting(whPos, GoodType::Beer, EInventorySetting::Collect);

            if(!wh->isInventorySetting(GoodType::Sword,
                                       EInventorySetting::Collect)) // not collecting swords? -> start it
                aii.SetInventorySetting(whPos, GoodType::Sword, EInventorySetting::Collect);

            if(!wh->isInventorySetting(GoodType::ShieldRomans,
                                       EInventorySetting::Collect)) // not collecting shields? -> start it
                aii.SetInventorySetting(whPos, GoodType::ShieldRomans, EInventorySetting::Collect);

            if(!wh->isInventorySetting(Job::Private, EInventorySetting::Collect)
               && ggs.getMaxMilitaryRank()
                    > 0) // not collecting privates AND we can actually upgrade soldiers? -> start it
                aii.SetInventorySetting(whPos, Job::Private, EInventorySetting::Collect);

            // less than 50 helpers - collect them: more than 50 stop collecting
            if(wh->getInventory().people[Job::Helper] < 50)
            {
                if(!wh->isInventorySetting(Job::Helper, EInventorySetting::Collect))
                    aii.SetInventorySetting(whPos, Job::Helper, EInventorySetting::Collect);
            } else
            {
                if(wh->isInventorySetting(Job::Helper, EInventorySetting::Collect))
                    aii.SetInventorySetting(whPos, Job::Helper, InventorySetting());
            }
        }
    }
}

AINodeResource AIPlayerJH::calcResource(MapPoint pt)
{
    const AISubSurfaceResource subRes = aii.getSubsurfaceResource(pt);
    const AISurfaceResource surfRes = aii.getSurfaceResource(pt);

    // no resources underground
    if(subRes == AISubSurfaceResource::Nothing)
    {
        // also no resource on the ground: plant space or unusable?
        if(surfRes == AISurfaceResource::Nothing)
        {
            // already road, really no resources here
            if(gwb.isOnRoad(pt))
                return AINodeResource::Nothing;
            // check for vital plant space
            if(!gwb.isOfTerrain(pt, [](const TerrainDesc& desc) { return desc.isVital(); }))
                return AINodeResource::Nothing;
            return AINodeResource::Plantspace;
        } else
            return convertToNodeResource(surfRes);
    } else // resources in underground
    {
        switch(surfRes)
        {
            case AISurfaceResource::Stones:
            case AISurfaceResource::Wood: return AINodeResource::Multiple;
            case AISurfaceResource::Blocked: break;
            case AISurfaceResource::Nothing: return convertToNodeResource(subRes);
        }
        return AINodeResource::Nothing;
    }
}

void AIPlayerJH::initReachableNodes()
{
    std::queue<MapPoint> toCheck;

    // Alle auf not reachable setzen
    RTTR_FOREACH_PT(MapPoint, aiMap.getSize())
    {
        Node& node = aiMap[pt];
        node.reachable = false;
        node.failed_penalty = 0;
        const auto* myFlag = gwb.getSpecObj<noFlag>(pt);
        if(myFlag && myFlag->getPlayer() == playerId)
        {
            node.reachable = true;
            toCheck.push(pt);
        }
    }

    iterativeReachableNodeChecker(toCheck);
}

void AIPlayerJH::iterativeReachableNodeChecker(std::queue<MapPoint> toCheck)
{
    // TODO auch mal bootswege bauen können

    PathConditionRoad<GameWorldBase> roadPathChecker(gwb, false);
    while(!toCheck.empty())
    {
        // Reachable coordinate
        MapPoint curPt = toCheck.front();

        // Coordinates to test around this reachable coordinate
        for(const MapPoint curNeighbour : aiMap.getNeighbours(curPt))
        {
            Node& node = aiMap[curNeighbour];

            // already reached, don't test again
            if(node.reachable)
                continue;

            // Test whether point is reachable; yes->add to check list
            if(roadPathChecker.isNodeOk(curNeighbour))
            {
                if(node.failed_penalty == 0)
                {
                    node.reachable = true;
                    toCheck.push(curNeighbour);
                } else
                {
                    node.failed_penalty--;
                }
            }
        }
        toCheck.pop();
    }
}

void AIPlayerJH::updateReachableNodes(const std::vector<MapPoint>& pts)
{
    std::queue<MapPoint> toCheck;

    for(const MapPoint& curPt : pts)
    {
        const auto* flag = gwb.getSpecObj<noFlag>(curPt);
        if(flag && flag->getPlayer() == playerId)
        {
            aiMap[curPt].reachable = true;
            toCheck.push(curPt);
        } else
            aiMap[curPt].reachable = false;
    }
    iterativeReachableNodeChecker(toCheck);
}

void AIPlayerJH::initNodes()
{
    aiMap.resize(gwb.getSize());

    initReachableNodes();

    RTTR_FOREACH_PT(MapPoint, aiMap.getSize())
    {
        Node& node = aiMap[pt];

        node.bq = aii.getBuildingQuality(pt);
        node.res = calcResource(pt);
        node.owned = aii.isOwnTerritory(pt);
        node.border = aii.isBorder(pt);
        node.farmed = false;
    }
}

void AIPlayerJH::updateNodesAround(const MapPoint pt, unsigned radius)
{
    std::vector<MapPoint> pts = gwb.getPointsInRadius(pt, radius);
    updateReachableNodes(pts);
    for(const MapPoint& pt : pts)
    {
        Node& node = aiMap[pt];
        // Change of ownership might change bq
        node.bq = aii.getBuildingQuality(pt);
        node.owned = aii.isOwnTerritory(pt);
        node.border = aii.isBorder(pt);
    }
}

void AIPlayerJH::initResourceMaps()
{
    for(auto& resMap : resourceMaps)
        resMap.init();
}

void AIPlayerJH::setFarmedNodes(const MapPoint pt, bool set)
{
    // Radius in dem Bauplatz für Felder blockiert wird
    const unsigned radius = 3;

    aiMap[pt].farmed = set;
    std::vector<MapPoint> pts = gwb.getPointsInRadius(pt, radius);
    for(const MapPoint& curPt : pts)
        aiMap[curPt].farmed = set;
}

MapPoint AIPlayerJH::findBestPosition(const MapPoint& pt, AIResource res, BuildingQuality size, unsigned radius,
                                      int minimum)
{
    resourceMaps[res].updateAround(pt, radius);
    return resourceMaps[res].findBestPosition(pt, size, radius, minimum);
}

void AIPlayerJH::executeAIJob()
{
    // Check whether current job is finished...
    /*if (currentJob)
    {
        if (currentJob->GetStatus() == JobState::Finished)
        {
            delete currentJob;
            currentJob = 0;
        }
    }

    // ... or it failed
    if (currentJob)
    {
        if (currentJob->GetStatus() == JobState::Failed)
        {
            // TODO fehlerbehandlung?
            //std::cout << "Job failed." << std::endl;
            delete currentJob;
            currentJob = 0;
        }
    }*/
    unsigned quota = 10; // limit the amount of events to handle
    while(eventManager.eventAvailable()
          && quota) // handle all new events - some will add new orders but they can all be handled instantly
    {
        quota--;
        currentJob = std::make_unique<EventJob>(*this, eventManager.getEvent());
        currentJob->executeJob();
    }
    // how many construction & connect jobs the ai will attempt every gf, the ai gets new orders from events and every
    // 200 gf
    quota = (aii.getStorehouses().size() + aii.getMilitaryBuildings().size()) * 1;
    if(quota > 40)
        quota = 40;

    construction->executeJobs(quota); // try to execute up to quota connect & construction jobs
    /*
    // if no current job available, take next one! events first, then constructions
    if (!currentJob)
    {
        if (construction->BuildJobAvailable())
        {
            currentJob = construction->GetBuildJob();
        }
    }
    // Something to do? Do it!
    if (currentJob)
        currentJob->ExecuteJob();
        */
}

void AIPlayerJH::distributeGoodsByBlocking(const GoodType good, unsigned limit)
{
    const std::list<nobBaseWarehouse*>& storehouses = aii.getStorehouses();
    if(aii.getHarbors().size() >= storehouses.size() / 2)
    {
        // dont distribute on maps that are mostly sea maps - harbors are too difficult to defend and have to handle
        // quite a lot of traffic already So unblock everywhere
        for(nobBaseWarehouse* wh : storehouses)
        {
            if(wh->isInventorySetting(good, EInventorySetting::Stop)) // not unblocked then issue command to unblock
                aii.SetInventorySetting(wh->getPos(), good,
                                        wh->getInventorySetting(good).toggle(EInventorySetting::Stop));
        }
        return;
    }

    RTTR_Assert(storehouses.size() >= 2); // Should be assured by condition above
    // We can only distribute between reachable warehouses, so divide them
    std::vector<std::vector<const nobBaseWarehouse*>> whsByReachability;
    for(const nobBaseWarehouse* wh : storehouses)
    {
        // See to which other whs this is connected
        bool foundConnectedWh = false;
        for(std::vector<const nobBaseWarehouse*>& whGroup : whsByReachability)
        {
            if(aii.findPathOnRoads(*wh, *whGroup.front()))
            {
                whGroup.push_back(wh);
                foundConnectedWh = true;
                break;
            }
        }
        // Not connected to any other -> Add new group
        if(!foundConnectedWh)
            whsByReachability.push_back(std::vector<const nobBaseWarehouse*>(1, wh));
    }

    // Now check each group individually
    for(const std::vector<const nobBaseWarehouse*>& whGroup : whsByReachability)
    {
        // First check if all WHs have more than limit goods (or better: if one does not)
        bool allWHsHaveLimit = true;
        for(const nobBaseWarehouse* wh : whGroup)
        {
            if(wh->getNumVisualWares(good) <= limit)
            {
                allWHsHaveLimit = false;
                break;
            }
        }
        if(allWHsHaveLimit)
        {
            // So unblock everywhere
            for(const nobBaseWarehouse* wh : whGroup)
            {
                if(wh->isInventorySetting(good, EInventorySetting::Stop)) // not unblocked then issue command to unblock
                    aii.SetInventorySetting(wh->getPos(), good,
                                            wh->getInventorySetting(good).toggle(EInventorySetting::Stop));
            }
        } else
        {
            // At least 1 WH needs wares
            for(const nobBaseWarehouse* wh : whGroup)
            {
                if(wh->getNumVisualWares(good) <= limit) // not at limit - unblock it
                {
                    if(wh->isInventorySetting(good,
                                              EInventorySetting::Stop)) // not unblocked then issue command to unblock
                        aii.SetInventorySetting(wh->getPos(), good,
                                                wh->getInventorySetting(good).toggle(EInventorySetting::Stop));
                } else // at limit - block it
                {
                    if(!wh->isInventorySetting(good,
                                               EInventorySetting::Stop)) // not blocked then issue command to block
                        aii.SetInventorySetting(wh->getPos(), good,
                                                wh->getInventorySetting(good).toggle(EInventorySetting::Stop));
                }
            }
        }
    }
}

void AIPlayerJH::distributeMaxRankSoldiersByBlocking(unsigned limit, nobBaseWarehouse* upwh)
{
    const std::list<nobBaseWarehouse*>& storehouses = aii.getStorehouses();
    unsigned numCompleteWh = storehouses.size();

    if(numCompleteWh < 1) // no warehouses -> no job
        return;

    ::Job maxRankJob = SOLDIER_JOBS[ggs.getMaxMilitaryRank()];

    if(numCompleteWh == 1) // only 1 warehouse? dont block max ranks here
    {
        nobBaseWarehouse& wh = *storehouses.front();
        if(wh.isInventorySetting(maxRankJob, EInventorySetting::Stop))
            aii.SetInventorySetting(wh.getPos(), maxRankJob,
                                    wh.getInventorySetting(maxRankJob).toggle(EInventorySetting::Stop));
        return;
    }
    // rest applies for at least 2 complete warehouses!
    std::list<const nobMilitary*> frontierMils; // make a list containing frontier military buildings
    for(const nobMilitary* wh : aii.getMilitaryBuildings())
    {
        if(wh->getFrontierDistance() != FrontierDistance::Far && !wh->isNewBuilt())
            frontierMils.push_back(wh);
    }
    std::list<const nobBaseWarehouse*>
      frontierWhs; // make a list containing all warehouses near frontier military buildings
    for(const nobBaseWarehouse* wh : storehouses)
    {
        for(const nobMilitary* milBld : frontierMils)
        {
            if(gwb.calcDistance(wh->getPos(), milBld->getPos()) < 12)
            {
                frontierWhs.push_back(wh);
                break;
            }
        }
    }
    // have frontier warehouses?
    if(!frontierWhs.empty())
    {
        // LOG.write(("distribute maxranks - got frontierwhs for player %i \n",playerId);
        bool hasUnderstaffedWh = false;
        // try to gather limit maxranks in each - if we have that many unblock for all frontier whs,
        // check if there is at least one with less than limit first
        for(const nobBaseWarehouse* wh : frontierWhs)
        {
            if(wh->getInventory().people[maxRankJob] < limit)
            {
                hasUnderstaffedWh = true;
                break;
            }
        }
        // if understaffed was found block in all with >=limit else unblock in all
        for(const nobBaseWarehouse* wh : storehouses)
        {
            const bool shouldBlock = !helpers::contains(frontierWhs, wh) // Not a frontier wh or:
                                     || (hasUnderstaffedWh && wh->getInventory().people[maxRankJob] >= limit);
            if(shouldBlock != wh->isInventorySetting(maxRankJob, EInventorySetting::Stop))
                aii.SetInventorySetting(wh->getPos(), maxRankJob,
                                        wh->getInventorySetting(maxRankJob).toggle(EInventorySetting::Stop));
        }
    } else // there are no frontier whs!
    {
        // LOG.write(("distribute maxranks - got NO frontierwhs for player %i \n",playerId);
        bool hasUnderstaffedWh = false;
        // try to gather limit maxranks in each - if we have that many unblock for all  whs,
        // check if there is at least one with less than limit first
        for(const nobBaseWarehouse* wh : storehouses)
        {
            if(wh->getInventory().people[maxRankJob] < limit
               && wh->getPos() != upwh->getPos()) // warehouse next to upgradebuilding is special case
            {
                hasUnderstaffedWh = true;
                break;
            }
        }
        for(const nobBaseWarehouse* wh : storehouses)
        {
            bool shouldBlock;
            if(wh->getPos()
               == upwh->getPos()) // warehouse next to upgradebuilding should block when there is more than 1 wh
            {
                // LOG.write(("distribute maxranks - got NO frontierwhs for player %i , block at hq \n",playerId);
                shouldBlock = true;
            } else if(hasUnderstaffedWh)
            {
                shouldBlock = wh->getInventory().people[maxRankJob] >= limit;
            } else // no understaffedwh
                shouldBlock = false;
            if(shouldBlock != wh->isInventorySetting(maxRankJob, EInventorySetting::Stop))
                aii.SetInventorySetting(wh->getPos(), maxRankJob,
                                        wh->getInventorySetting(maxRankJob).toggle(EInventorySetting::Stop));
        }
    }
}
MapPoint AIPlayerJH::simpleFindPosition(const MapPoint& pt, BuildingQuality size, unsigned radius) const
{
    std::vector<MapPoint> pts = gwb.getPointsInRadius(pt, radius);
    for(const MapPoint& curPt : pts)
    {
        if(!aiMap[curPt].reachable || aiMap[curPt].farmed || !aii.isOwnTerritory(curPt))
            continue;
        if(aii.isHarborPosClose(curPt, 2, true))
        {
            if(size != BuildingQuality::Harbor)
                continue;
        }
        RTTR_Assert(aii.getBuildingQuality(curPt) == getAINode(curPt).bq);
        if(canUseBq(aii.getBuildingQuality(curPt), size)) //(*nodes)[idx].bq; TODO: Update nodes BQ and use that
            return curPt;
    }

    return MapPoint::invalid();
}

MapPoint AIPlayerJH::findPositionForBuildingAround(BuildingType type, const MapPoint& around)
{
    constexpr unsigned searchRadius = 11;
    MapPoint foundPos = MapPoint::invalid();
    switch(type)
    {
        case BuildingType::Woodcutter:
        {
            foundPos = findBestPosition(around, AIResource::Wood, BUILDING_SIZE[type], searchRadius, 20);
            break;
        }
        case BuildingType::Forester:
            // ensure some distance to other foresters and an minimal amount of plantspace
            if(!construction->otherUsualBuildingInRadius(around, 12, BuildingType::Forester)
               && getDensity(around, AIResource::Plantspace, 7) > 15)
                foundPos = findBestPosition(around, AIResource::Wood, BUILDING_SIZE[type], searchRadius, 0);
            break;
        case BuildingType::Hunter:
        {
            // check if there are any animals in range
            if(huntablesInRange(around, (2 << getBldPlanner().getNumBuildings(BuildingType::Hunter))))
                foundPos = simpleFindPosition(around, BUILDING_SIZE[type], searchRadius);
            break;
        }
        case BuildingType::Quarry:
        {
            unsigned numQuarries = getBldPlanner().getNumBuildings(BuildingType::Quarry);
            foundPos = findBestPosition(around, AIResource::Stones, BUILDING_SIZE[type], searchRadius,
                                        std::min(40u, 1 + numQuarries * 10));
            if(foundPos.isValid() && !validStoneinRange(foundPos))
            {
                resourceMaps[AIResource::Stones].avoidPosition(foundPos);
                foundPos = MapPoint::invalid();
            }
            break;
        }
        case BuildingType::Barracks:
        case BuildingType::Guardhouse:
        case BuildingType::Watchtower:
        case BuildingType::Fortress:
            foundPos = findBestPosition(around, AIResource::Borderland, BUILDING_SIZE[type], searchRadius);
            break;
        case BuildingType::GoldMine:
            foundPos = findBestPosition(around, AIResource::Gold, BuildingQuality::Mine, searchRadius);
            break;
        case BuildingType::CoalMine:
            foundPos = findBestPosition(around, AIResource::Coal, BuildingQuality::Mine, searchRadius);
            break;
        case BuildingType::IronMine:
            foundPos = findBestPosition(around, AIResource::Ironore, BuildingQuality::Mine, searchRadius);
            break;
        case BuildingType::GraniteMine:
            foundPos = findBestPosition(around, AIResource::Granite, BuildingQuality::Mine, searchRadius);
            break;
        case BuildingType::Fishery:
            foundPos = findBestPosition(around, AIResource::Fish, BUILDING_SIZE[type], searchRadius);
            if(foundPos.isValid() && !validFishInRange(foundPos))
            {
                resourceMaps[AIResource::Fish].avoidPosition(foundPos);
                foundPos = MapPoint::invalid();
            }
            break;
        case BuildingType::Storehouse:
            if(!construction->otherStoreInRadius(around, 15))
                foundPos = simpleFindPosition(around, BUILDING_SIZE[type], searchRadius);
            break;
        case BuildingType::HarborBuilding:
            foundPos = simpleFindPosition(around, BUILDING_SIZE[type], searchRadius);
            if(foundPos.isValid()
               && !harborPosRelevant(getWorld().getHarborPointID(foundPos))) // bad harborspot detected DO NOT USE
                foundPos = MapPoint::invalid();
            break;
        case BuildingType::Shipyard:
            foundPos = simpleFindPosition(around, BUILDING_SIZE[type], searchRadius);
            if(foundPos.isValid() && isInvalidShipyardPosition(foundPos))
                foundPos = MapPoint::invalid();
            break;
        case BuildingType::Farm:
            foundPos = findBestPosition(around, AIResource::Plantspace, BUILDING_SIZE[type], searchRadius, 85);
            if(foundPos.isValid())
                foundPos = findBestPosition(around, AIResource::Plantspace, BUILDING_SIZE[type], searchRadius, 85);
            break;
        case BuildingType::Catapult:
            foundPos = simpleFindPosition(around, BUILDING_SIZE[type], searchRadius);
            if(foundPos.isValid() && aii.isBuildingNearby(BuildingType::Catapult, foundPos, 7))
                foundPos = MapPoint::invalid();
            break;
        default: foundPos = simpleFindPosition(around, BUILDING_SIZE[type], searchRadius); break;
    }
    return foundPos;
}

unsigned AIPlayerJH::getDensity(MapPoint pt, AIResource res, int radius)
{
    RTTR_Assert(pt.x < aiMap.getWidth() && pt.y < aiMap.getHeight());

    std::vector<MapPoint> pts = gwb.getPointsInRadius(pt, radius);
    const unsigned numAllPTs = pts.size();
    RTTR_Assert(numAllPTs > 0);

    const auto hasResource = [this, res](const MapPoint& curPt) {
        // TODO: Fix
        // return aiMap[curPt].res == res;
        return calcResource(curPt) == res;
    };
    const unsigned numGoodPts = helpers::count_if(pts, hasResource);
    return (numGoodPts * 100) / numAllPTs;
}

void AIPlayerJH::handleNewMilitaryBuildingOccupied(const MapPoint pt)
{
    // kill bad flags we find
    removeAllUnusedRoads(pt);
    bldPlanner->updateBuildingsWanted(*this);
    const auto* mil = gwb.getSpecObj<nobMilitary>(pt);
    if(!mil)
        return;
    // if near border and gold disabled (by addon): enable it
    if(mil->getFrontierDistance() != FrontierDistance::Far)
    {
        if(mil->isGoldDisabled())
            aii.SetCoinsAllowed(pt, true);
    } else if((mil->getBuildingType() == BuildingType::Barracks || mil->getBuildingType() == BuildingType::Guardhouse)
              && mil->getBuildingType() != construction->getBiggestAllowedMilBuilding())
    {
        if(!mil->isGoldDisabled())
            aii.SetCoinsAllowed(pt, false);
    }

    addBuildJob(BuildingType::HarborBuilding, pt);
    if(!isInvalidShipyardPosition(pt))
        addBuildJob(BuildingType::Shipyard, pt);
    if(soldierAvailable())
        addMilitaryBuildJob(pt);

    // try to build one the following buildings around the new military building

    std::array<BuildingType, 11> bldToTest = {
      BuildingType::Storehouse, BuildingType::Woodcutter, BuildingType::Quarry,      BuildingType::GoldMine,
      BuildingType::CoalMine,   BuildingType::IronMine,   BuildingType::GraniteMine, BuildingType::Fishery,
      BuildingType::Farm,       BuildingType::Hunter,     BuildingType::Forester};
    unsigned bldToTestStartIdx = 0;
    // remove the storehouse from the building test list if we are close to another storehouse already
    for(const nobBaseWarehouse* bldSite : aii.getStorehouses())
    {
        if(gwb.calcDistance(bldSite->getPos(), pt) < 20)
        {
            bldToTestStartIdx = 1;
            break;
        }
    }
    // same is true for warehouses which are still in production
    for(const noBuildingSite* bldSite : aii.getBuildingSites())
    {
        if(BuildingProperties::isWareHouse(bldSite->getBuildingType()))
        {
            if(gwb.calcDistance(bldSite->getPos(), pt) < 20)
            {
                bldToTestStartIdx = 1;
                break;
            }
        }
    }

    for(unsigned i = bldToTestStartIdx; i < bldToTest.size(); ++i)
    {
        if(construction->wanted(bldToTest[i]))
        {
            addBuildJob(bldToTest[i], pt);
        }
    }
}

void AIPlayerJH::handleBuilingDestroyed(MapPoint pt, BuildingType bld)
{
    switch(bld)
    {
        case BuildingType::Charburner:
        case BuildingType::Farm: setFarmedNodes(pt, false); break;
        case BuildingType::HarborBuilding:
        {
            // destroy all other buildings around the harborspot in range 2 so we can rebuild the harbor ...
            for(const MapPoint curPt : gwb.getPointsInRadius(pt, 2))
            {
                const auto* const bb = gwb.getSpecObj<noBaseBuilding>(curPt);
                if(bb)
                    aii.DestroyBuilding(curPt);
                else
                {
                    const auto* const bs = gwb.getSpecObj<noBuildingSite>(curPt);
                    if(bs)
                        aii.DestroyFlag(gwb.getNeighbour(curPt, Direction::SouthEast));
                }
            }
            break;
        }
        default: break;
    }
}

void AIPlayerJH::handleRoadConstructionComplete(MapPoint pt, Direction dir)
{
    // todo: detect "bad" roads and handle them
    const auto* flag = gwb.getSpecObj<noFlag>(pt);
    // does the flag still exist?
    if(!flag)
        return;
    // does the roadsegment still exist?
    const RoadSegment* const roadSeg = flag->getRoute(dir);
    if(!roadSeg || roadSeg->GetLength() < 4) // road too short to need flags
        return;
    // check if this road leads to a warehouseflag and if it does start setting flags from the warehouseflag else from
    // the new flag goal is to move roadsegments with a length of more than 2 away from the warehouse
    const noFlag& otherFlag = roadSeg->getOtherFlag(*flag);
    MapPoint bldPos = gwb.getNeighbour(otherFlag.getPos(), Direction::NorthWest);
    if(aii.isBuildingOnNode(bldPos, BuildingType::Storehouse)
       || aii.isBuildingOnNode(bldPos, BuildingType::HarborBuilding)
       || aii.isBuildingOnNode(bldPos, BuildingType::Headquarters))
        construction->setFlagsAlongRoad(otherFlag, roadSeg->getOtherFlagDir(*flag) + 3u);
    else
    {
        // set flags on our new road starting from the new flag
        construction->setFlagsAlongRoad(*flag, dir);
    }
}

void AIPlayerJH::handleRoadConstructionFailed(const MapPoint pt, Direction)
{
    const auto* flag = gwb.getSpecObj<noFlag>(pt);
    // does the flag still exist?
    if(!flag)
        return;
    // is it our flag?
    if(flag->getPlayer() != playerId)
        return;
    // if it isnt a useless flag AND it has no current road connection then retry to build a road.
    if(removeUnusedRoad(*flag, std::nullopt, true, false))
        construction->addConnectFlagJob(flag);
}

void AIPlayerJH::handleMilitaryBuilingLost(const MapPoint pt)
{
    // For now, this is the same as losing land.
    handleLostLand(pt);
}

void AIPlayerJH::handleBuildingFinished(const MapPoint pt, BuildingType bld)
{
    switch(bld)
    {
        case BuildingType::HarborBuilding:
            updateNodesAround(pt, 8); // todo: fix radius
            removeAllUnusedRoads(
              pt); // repair & reconnect road system - required when a colony gets a new harbor by expedition
            aii.ChangeReserve(pt, 0, 1); // order 1 defender to stay in the harborbuilding

            // if there are positions free start an expedition!
            if(harborPosRelevant(gwb.getHarborPointID(pt), true))
            {
                aii.StartStopExpedition(pt, true);
            }
            break;

        case BuildingType::Shipyard: aii.SetShipYardMode(pt, true); break;

        case BuildingType::Storehouse: break;
        case BuildingType::Woodcutter: addBuildJob(BuildingType::Sawmill, pt); break;
        default: break;
    }
}

void AIPlayerJH::handleNewColonyFounded(const MapPoint pt)
{
    construction->addConnectFlagJob(gwb.getSpecObj<noFlag>(gwb.getNeighbour(pt, Direction::SouthEast)));
}

void AIPlayerJH::handleExpedition(const noShip& ship)
{
    if(!ship.isWaitingForExpeditionInstructions())
        return;
    if(ship.isAbleToFoundColony())
        aii.foundColony(ship);
    else
    {
        for(auto dir : helpers::enumRange(AI::randomEnum<ShipDirection>()))
        {
            if(aii.isExplorationDirectionPossible(ship.getPos(), ship.getCurrentHarbor(), dir))
            {
                aii.travelToNextSpot(dir, ship);
                return;
            }
        }
        // no direction possible, sad, stop it
        aii.CancelExpedition(ship);
    }
}

void AIPlayerJH::handleExpedition(const MapPoint pt)
{
    const noShip* ship = nullptr;

    for(const noBase& obj : gwb.getFigures(pt))
    {
        if(obj.getGOT() == GO_Type::Ship)
        {
            const auto& curShip = static_cast<const noShip&>(obj);
            if(curShip.getPlayerId() == playerId && curShip.isWaitingForExpeditionInstructions())
            {
                ship = &curShip;
                break;
            }
        }
    }
    if(ship)
    {
        handleExpedition(*ship);
    }
}

void AIPlayerJH::handleTreeChopped(const MapPoint pt)
{
    // std::cout << "Tree chopped." << std::endl;

    aiMap[pt].reachable = true;

    updateNodesAround(pt, 3);

    if(AI::randomChance())
        addMilitaryBuildJob(pt);
    else // if (random % 12 == 0)
        addBuildJob(BuildingType::Woodcutter, pt);
}

void AIPlayerJH::handleNoMoreResourcesReachable(const MapPoint pt, BuildingType bld)
{
    // Destroy old building (once)

    if(!aii.isObjectTypeOnNode(pt, NodalObjectType::Building))
        return;
    // keep 2 woodcutters for each forester even if they sometimes run out of trees
    if(bld == BuildingType::Woodcutter)
    {
        for(const nobUsual* forester : aii.getBuildings(BuildingType::Forester))
        {
            // is the forester somewhat close?
            if(gwb.calcDistance(pt, forester->getPos()) <= RES_RADIUS[AIResource::Wood])
            {
                // then find it's 2 woodcutters
                unsigned maxdist = gwb.calcDistance(pt, forester->getPos());
                int betterwoodcutters = 0;
                for(const nobUsual* woodcutter : aii.getBuildings(BuildingType::Woodcutter))
                {
                    // dont count the woodcutter in question
                    if(pt == woodcutter->getPos())
                        continue;
                    // TODO: We currently don't take the distance to the forester into account when placing a woodcutter
                    // This leads to points beeing equally good for placing but later it will be destroyed. Avoid that
                    // by checking only close woddcutters
                    if(gwb.calcDistance(woodcutter->getPos(), pt) > RES_RADIUS[AIResource::Wood])
                        continue;
                    // closer or equally close to forester than woodcutter in question?
                    if(gwb.calcDistance(woodcutter->getPos(), forester->getPos()) <= maxdist)
                    {
                        betterwoodcutters++;
                        if(betterwoodcutters >= 2)
                            break;
                    }
                }
                // couldnt find 2 closer woodcutter -> keep it alive
                if(betterwoodcutters < 2)
                    return;
            }
        }
    }
    aii.DestroyBuilding(pt);
    // fishery cant find fish? set fish value at location to 0 so we dont have to calculate the value for this location
    // again
    if(bld == BuildingType::Fishery)
        resourceMaps[AIResource::Fish].avoidPosition(pt);

    updateNodesAround(pt, 11); // todo: fix radius
    removeUnusedRoad(*gwb.getSpecObj<noFlag>(gwb.getNeighbour(pt, Direction::SouthEast)), Direction::NorthWest, true);

    // try to expand, maybe res blocked a passage
    addMilitaryBuildJob(pt);

    // and try to rebuild the same building
    if(bld != BuildingType::Hunter)
        addBuildJob(bld, pt);

    // farm is always good!
    addBuildJob(BuildingType::Farm, pt);
}

void AIPlayerJH::handleShipBuilt(const MapPoint pt)
{
    // Stop building ships if reached a maximum (TODO: make variable)
    const std::list<nobUsual*>& shipyards = aii.getBuildings(BuildingType::Shipyard);
    bool wantMoreShips;
    unsigned numRelevantSeas = getNumAIRelevantSeaIds();
    if(numRelevantSeas == 0)
        wantMoreShips = false;
    else if(numRelevantSeas == 1)
        wantMoreShips = aii.getNumShips() <= gwb.getNumHarborPoints();
    else
    {
        unsigned wantedShipCt = std::min<unsigned>(7, 3 * shipyards.size());
        wantMoreShips = aii.getNumShips() < wantedShipCt;
    }
    if(!wantMoreShips)
    {
        // Find shipyard from this ship by getting the closest one. Max distance of <12 nodes
        unsigned mindist = 12;
        const nobUsual* creatingShipyard = nullptr;
        for(const nobUsual* shipyard : shipyards)
        {
            unsigned distance = gwb.calcDistance(shipyard->getPos(), pt);
            if(distance < mindist)
            {
                mindist = distance;
                creatingShipyard = shipyard;
            }
        }
        if(creatingShipyard) // might have been destroyed by now
            aii.SetProductionEnabled(creatingShipyard->getPos(), false);
    }
}

void AIPlayerJH::handleBorderChanged(const MapPoint pt)
{
    updateNodesAround(pt, 11); // todo: fix radius

    const auto* mil = gwb.getSpecObj<nobMilitary>(pt);
    if(mil)
    {
        if(mil->getFrontierDistance() != FrontierDistance::Far)
        {
            if(mil->isGoldDisabled())
                aii.SetCoinsAllowed(pt, true);

            // Fill up with soldiers
            for(unsigned rank = 0; rank < NUM_SOLDIER_RANKS; ++rank)
            {
                if(mil->getTroopLimit(rank) != mil->getMaxTroopsCt())
                    aii.SetTroopLimit(mil->getPos(), rank, mil->getMaxTroopsCt());
            }
        }
        if(mil->getBuildingType() != construction->getBiggestAllowedMilBuilding())
        {
            addMilitaryBuildJob(pt);
        }
    }
}

void AIPlayerJH::handleLostLand(const MapPoint pt)
{
    if(aii.getStorehouses()
         .empty()) // check if we have a storehouse left - if we dont have one trying to find a path to one will crash
    {
        return;
    }
    removeAllUnusedRoads(pt);
}

void AIPlayerJH::milUpgradeOptim()
{
    const auto* upgradeBld = updateUpgradeBuilding();
    const std::list<nobMilitary*>& militaryBuildings = aii.getMilitaryBuildings();
    const auto numPlannedConnectedInlandMilitaryBlds = getNumPlannedConnectedInlandMilitaryBlds();
    unsigned count = 0;
    for(const nobMilitary* milBld : militaryBuildings)
    {
        if(!upgradeBld)
        {
            if(milBld->isGoldDisabled() && milBld->getFrontierDistance() != FrontierDistance::Far)
                aii.SetCoinsAllowed(milBld->getPos(), true);
        } else if(milBld != upgradeBld) // not upgrade building
        {
            if(!milBld->isGoldDisabled()) // deactivate gold for all other buildings
                aii.SetCoinsAllowed(milBld->getPos(), false);
            // For dedicated inland buildings send out troops until 1 private is left, then cancel road
            // Connect frontier buildings to road system.
            if(milBld->getFrontierDistance() != FrontierDistance::Far)
                construction->addConnectFlagJob(milBld->getFlag());
            else if(count + numPlannedConnectedInlandMilitaryBlds < militaryBuildings.size())
            {
                if(milBld->getNumTroops() > 1) // more than 1 soldier remaining? -> send out order
                {
                    aii.SetTroopLimit(milBld->getPos(), 0, 1);
                    for(unsigned rank = 1; rank < NUM_SOLDIER_RANKS; ++rank)
                        aii.SetTroopLimit(milBld->getPos(), rank, 0);

                    // TODO: Currently the AI still manages soldiers by disconnecting roads, if in the future it
                    // uses only SetTroopLimit then this can be removed
                    for(unsigned rank = 0; rank < NUM_SOLDIER_RANKS; ++rank)
                        aii.SetTroopLimit(milBld->getPos(), rank, milBld->getMaxTroopsCt());
                } else if(!milBld->isNewBuilt())
                {
                    // 0-1 soldier remains and the building has had at least 1 soldier
                    // at some point and the building is not new on the list
                    // -> cancel road (and fix roadsystem if necessary)
                    removeUnusedRoad(*milBld->getFlag(), Direction::NorthWest, true, true, true);
                }
            }
        } else // upgrade building
        {
            if(!construction->isConnectedToRoadSystem(milBld->getFlag()))
            {
                construction->addConnectFlagJob(milBld->getFlag());
                continue;
            }
            if(milBld->isGoldDisabled()) // activate gold
                aii.SetCoinsAllowed(milBld->getPos(), true);
            // Keep 0 max rank soldiers, 1 of each other rank and fill the rest with privates
            aii.SetTroopLimit(milBld->getPos(), 0, milBld->getMaxTroopsCt());
            for(unsigned rank = 1; rank < ggs.getMaxMilitaryRank(); ++rank)
                aii.SetTroopLimit(milBld->getPos(), rank, 1);
            aii.SetTroopLimit(milBld->getPos(), ggs.getMaxMilitaryRank(), 0);
        }
        count++;
    }
}

bool AIPlayerJH::hasFrontierBuildings()
{
    for(const nobMilitary* milBld : aii.getMilitaryBuildings())
    {
        if(milBld->getFrontierDistance() != FrontierDistance::Far)
            return true;
    }
    return false;
}

void AIPlayerJH::checkExpeditions()
{
    const std::list<nobHarborBuilding*>& harbors = aii.getHarbors();
    for(const nobHarborBuilding* harbor : harbors)
    {
        bool isHarborRelevant = harborPosRelevant(harbor->getHarborPosID(), true);
        if(harbor->isExpeditionActive() != isHarborRelevant) // harbor is collecting for expedition and shouldn't
                                                             // OR not collecting and should -> toggle expedition
        {
            aii.StartStopExpedition(harbor->getPos(), isHarborRelevant);
        }
    }
    // find lost expedition ships
    // AI should get a notice and catch them all but just in case some fell through the system
    const std::vector<noShip*>& ships = aii.getShips();
    for(const noShip* ship : ships)
    {
        if(ship->isWaitingForExpeditionInstructions())
            handleExpedition(*ship);
    }
}

void AIPlayerJH::checkForester()
{
    const std::list<nobUsual*>& foresters = aii.getBuildings(BuildingType::Forester);
    if(!foresters.empty() && foresters.size() < 2 && aii.getMilitaryBuildings().size() < 3
       && aii.getBuildingSites().size() < 3)
    // stop the forester
    {
        if(!(*foresters.begin())->isProductionDisabled())
            aii.SetProductionEnabled(foresters.front()->getPos(), false);
    } else // activate the forester
    {
        if(!foresters.empty() && (*foresters.begin())->isProductionDisabled())
            aii.SetProductionEnabled(foresters.front()->getPos(), true);
    }
}

void AIPlayerJH::checkGranitMine()
{
    // stop production in granite mines when the ai has many stones (100+ and at least 15 for each warehouse)
    bool enableProduction =
      amountInStorage(GoodType::Stones) < 100 || amountInStorage(GoodType::Stones) < 15 * aii.getStorehouses().size();
    for(const nobUsual* mine : aii.getBuildings(BuildingType::GraniteMine))
    {
        // !productionDisabled != enableProduction
        if(mine->isProductionDisabled() == enableProduction)
            aii.SetProductionEnabled(mine->getPos(), enableProduction);
    }
}

void AIPlayerJH::tryToAttack()
{
    unsigned hq_or_harbor_without_soldiers = 0;
    std::vector<const nobBaseMilitary*> potentialTargets;

    // use own military buildings (except inland buildings) to search for enemy military buildings
    const std::list<nobMilitary*>& militaryBuildings = aii.getMilitaryBuildings();
    const unsigned numMilBlds = militaryBuildings.size();
    // when the ai has many buildings the ai will not check the complete list every time
    constexpr unsigned limit = 40;
    for(const nobMilitary* milBld : militaryBuildings)
    {
        // We handle the current building with a probability of limit/numMilBlds
        // -> For twice the number of blds as the limit we will most likely skip every 2nd building
        // This way we check roughly (at most) limit buildings but avoid any preference for one building over an other
        if(!AI::randomChance(numMilBlds, limit))
            continue;

        if(milBld->getFrontierDistance() == FrontierDistance::Far) // inland building? -> skip it
            continue;

        // get nearby enemy buildings and store in set of potential attacking targets
        MapPoint src = milBld->getPos();

        sortedMilitaryBlds buildings = gwb.lookForMilitaryBuildings(src, 2);
        for(const nobBaseMilitary* target : buildings)
        {
            if(helpers::contains(potentialTargets, target))
                continue;
            if(target->getGOT() == GO_Type::NobMilitary && static_cast<const nobMilitary*>(target)->isNewBuilt())
                continue;
            MapPoint dest = target->getPos();
            if(gwb.calcDistance(src, dest) < BASE_ATTACKING_DISTANCE && aii.isPlayerAttackable(target->getPlayer())
               && aii.isVisible(dest))
            {
                if(target->getGOT() != GO_Type::NobMilitary && !target->defendersAvailable())
                {
                    // headquarter or harbor without any troops :)
                    hq_or_harbor_without_soldiers++;
                    potentialTargets.insert(potentialTargets.begin(), target);
                } else
                    potentialTargets.push_back(target);
            }
        }
    }

    // shuffle everything but headquarters and harbors without any troops in them
    std::shuffle(potentialTargets.begin() + hq_or_harbor_without_soldiers, potentialTargets.end(),
                 AI::getRandomGenerator());

    // check for each potential attacking target the number of available attacking soldiers
    for(const nobBaseMilitary* target : potentialTargets)
    {
        const MapPoint dest = target->getPos();

        unsigned attackersCount = 0;
        unsigned attackersStrength = 0;

        // ask each of nearby own military buildings for soldiers to contribute to the potential attack
        sortedMilitaryBlds myBuildings = gwb.lookForMilitaryBuildings(dest, 2);
        for(const nobBaseMilitary* otherMilBld : myBuildings)
        {
            if(otherMilBld->getPlayer() == playerId)
            {
                const auto* myMil = dynamic_cast<const nobMilitary*>(otherMilBld);
                if(!myMil || myMil->isUnderAttack())
                    continue;

                unsigned newAttackers;
                attackersStrength += myMil->getSoldiersStrengthForAttack(dest, newAttackers);
                attackersCount += newAttackers;
            }
        }

        if(attackersCount == 0)
            continue;

        if((level == AI::Level::Hard) && (target->getGOT() == GO_Type::NobMilitary))
        {
            const auto* enemyTarget = static_cast<const nobMilitary*>(target);
            if(attackersStrength <= enemyTarget->getSoldiersStrength() || enemyTarget->getNumTroops() == 0)
                continue;
        }

        aii.Attack(dest, attackersCount, true);
        return;
    }
}

void AIPlayerJH::trySeaAttack()
{
    if(aii.getNumShips() < 1)
        return;
    if(aii.getHarbors().empty())
        return;
    std::vector<SeaId> seaidswithattackers;
    std::vector<SeaId> invalidseas;
    std::deque<const nobBaseMilitary*> potentialTargets;
    std::deque<const nobBaseMilitary*> undefendedTargets;
    std::vector<HarborId> searcharoundharborspots;
    // all seaids with at least 1 ship count available attackers for later checks
    for(const noShip* ship : aii.getShips())
    {
        // sea id not already listed as valid or invalid?
        if(!helpers::contains(seaidswithattackers, ship->getSeaID())
           && !helpers::contains(invalidseas, ship->getSeaID()))
        {
            unsigned attackercount = gwb.getNumSoldiersForSeaAttackAtSea(playerId, ship->getSeaID(), false);
            if(attackercount) // got attackers at this sea id? -> add to valid list
            {
                seaidswithattackers.push_back(ship->getSeaID());
            } else // not listed but no attackers? ->invalid
            {
                invalidseas.push_back(ship->getSeaID());
            }
        }
    }
    if(seaidswithattackers.empty()) // no sea ids with attackers? skip the rest
        return;
    /*else
    {
        for(unsigned i=0;i<seaidswithattackers.size();i++)
            LOG.write(("attackers at sea ids for player %i, sea id %i, count %i \n",playerId, seaidswithattackers[i],
    attackersatseaid[i]);
    }*/
    // first check all harbors there might be some undefended ones
    for(const auto i : helpers::idRange<HarborId>(gwb.getNumHarborPoints()))
    {
        const nobHarborBuilding* hb;
        if((hb = gwb.getSpecObj<nobHarborBuilding>(gwb.getHarborPoint(i))))
        {
            if(aii.isVisible(hb->getPos()))
            {
                if(aii.isPlayerAttackable(hb->getPlayer()))
                {
                    // attackers for this building?
                    const std::vector<SeaId> testseaidswithattackers =
                      gwb.getFilteredSeaIDsForAttack(gwb.getHarborPoint(i), seaidswithattackers, playerId);
                    if(!testseaidswithattackers.empty()) // harbor can be attacked?
                    {
                        if(!hb->defendersAvailable()) // no defenders?
                            undefendedTargets.push_back(hb);
                        else // todo: maybe only attack this when there is a fair win chance for the attackers?
                            potentialTargets.push_back(hb);
                        // LOG.write(("found a defended harbor we can attack at %i,%i \n",hb->GetPos());
                    }
                } else // cant attack player owning the harbor -> add to list
                {
                    searcharoundharborspots.push_back(i);
                }
            }
            // else: not visible for player no need to look any further here
        } else // no harbor -> add to list
        {
            searcharoundharborspots.push_back(i);
            // LOG.write(("found an unused harborspot we have to look around of at %i,%i
            // \n",gwb.GetHarborPoint(i).x,gwb.GetHarborPoint(i).y);
        }
    }
    auto& prng = AI::getRandomGenerator();
    // any undefendedTargets? -> pick one by random
    if(!undefendedTargets.empty())
    {
        std::shuffle(undefendedTargets.begin(), undefendedTargets.end(), prng);
        for(const nobBaseMilitary* targetMilBld : undefendedTargets)
        {
            std::vector<GameWorldBase::PotentialSeaAttacker> attackers =
              gwb.getSoldiersForSeaAttack(playerId, targetMilBld->getPos());
            if(!attackers.empty()) // try to attack it!
            {
                aii.SeaAttack(targetMilBld->getPos(), 1, true);
                return;
            }
        }
    }
    // add all military buildings around still valid harbor spots (unused or used by ally)
    unsigned limit = 15;
    unsigned skip = 0;
    if(searcharoundharborspots.size() > 15)
        skip = AI::randomValue(1u, static_cast<unsigned>(searcharoundharborspots.size() / 15u)) * 15u - 1;
    for(unsigned i = skip; i < searcharoundharborspots.size() && limit > 0; i++)
    {
        limit--;
        // now add all military buildings around the harbor spot to our list of potential targets
        sortedMilitaryBlds buildings = gwb.lookForMilitaryBuildings(gwb.getHarborPoint(searcharoundharborspots[i]), 2);
        for(const nobBaseMilitary* milBld : buildings)
        {
            if(aii.isPlayerAttackable(milBld->getPlayer()) && aii.isVisible(milBld->getPos()))
            {
                const auto* enemyTarget = dynamic_cast<const nobMilitary*>((milBld));

                if(enemyTarget && enemyTarget->isNewBuilt())
                    continue;
                if((milBld->getGOT() != GO_Type::NobMilitary)
                   && (!milBld->defendersAvailable())) // undefended headquarter(or unlikely as it is a harbor...) -
                                                       // priority list!
                {
                    const std::vector<SeaId> testseaidswithattackers =
                      gwb.getFilteredSeaIDsForAttack(milBld->getPos(), seaidswithattackers, playerId);
                    if(!testseaidswithattackers.empty())
                    {
                        undefendedTargets.push_back(milBld);
                    }  // else - no attackers - do nothing
                } else // normal target - check is done after random shuffle so we dont have to check every possible
                       // target and instead only enough to get 1 good one
                {
                    potentialTargets.push_back(milBld);
                }
            } // not attackable or no vision of region - do nothing
        }
    }
    // now we have a deque full of available and maybe undefended targets that are available for attack -> shuffle and
    // attack the first one we can attack("should" be the first we check...)  any undefendedTargets? -> pick one by
    // random
    if(!undefendedTargets.empty())
    {
        std::shuffle(undefendedTargets.begin(), undefendedTargets.end(), prng);
        for(const nobBaseMilitary* targetMilBld : undefendedTargets)
        {
            std::vector<GameWorldBase::PotentialSeaAttacker> attackers =
              gwb.getSoldiersForSeaAttack(playerId, targetMilBld->getPos());
            if(!attackers.empty()) // try to attack it!
            {
                aii.SeaAttack(targetMilBld->getPos(), 1, true);
                return;
            }
        }
    }
    std::shuffle(potentialTargets.begin(), potentialTargets.end(), prng);
    for(const nobBaseMilitary* ship : potentialTargets)
    {
        // TODO: decide if it is worth attacking the target and not just "possible"
        // test only if we should have attackers from one of our valid sea ids
        const std::vector<SeaId> testseaidswithattackers =
          gwb.getFilteredSeaIDsForAttack(ship->getPos(), seaidswithattackers, playerId);
        if(!testseaidswithattackers.empty()) // only do the final check if it will probably be a good result
        {
            std::vector<GameWorldBase::PotentialSeaAttacker> attackers =
              gwb.getSoldiersForSeaAttack(playerId, ship->getPos()); // now get a final list of attackers and attack it
            if(!attackers.empty())
            {
                aii.SeaAttack(ship->getPos(), attackers.size(), true);
                return;
            }
        }
    }
}

void AIPlayerJH::recalcGround(const MapPoint buildingPos, std::vector<Direction>& route_road)
{
    // building itself
    if(aiMap[buildingPos].res == AIResource::Plantspace)
        aiMap[buildingPos].res = AINodeResource::Nothing;

    // flag of building
    const MapPoint flagPos = gwb.getNeighbour(buildingPos, Direction::SouthEast);
    if(aiMap[flagPos].res == AIResource::Plantspace)
        aiMap[flagPos].res = AINodeResource::Nothing;

    // along the road
    MapPoint curPt = flagPos;
    for(auto i : route_road)
    {
        curPt = gwb.getNeighbour(curPt, i);
        if(aiMap[curPt].res == AIResource::Plantspace)
            aiMap[curPt].res = AINodeResource::Nothing;
    }
}

void AIPlayerJH::saveResourceMapsToFile()
{
    for(const auto res : helpers::enumRange<AIResource>())
    {
        bfs::ofstream file("resmap-" + std::to_string(static_cast<unsigned>(res)) + ".log");
        for(unsigned y = 0; y < aiMap.getHeight(); ++y)
        {
            if(y % 2 == 1)
                file << "  ";
            for(unsigned x = 0; x < aiMap.getWidth(); ++x)
                file << resourceMaps[res][MapPoint(x, y)] << "   ";
            file << "\n";
        }
    }
}

int AIPlayerJH::getResMapValue(const MapPoint pt, AIResource res) const
{
    return getResMap(res)[pt];
}

const AIResourceMap& AIPlayerJH::getResMap(AIResource res) const
{
    return resourceMaps[res];
}

void AIPlayerJH::sendAIEvent(std::unique_ptr<AIEvent::Base> ev)
{
    eventManager.addAIEvent(std::move(ev));
}

bool AIPlayerJH::isFlagPartofCircle(const noFlag& startFlag, unsigned maxlen, const noFlag& curFlag,
                                    helpers::OptionalEnum<Direction> excludeDir, std::vector<const noFlag*> oldFlags)
{
    // If oldFlags is empty we just started
    if(!oldFlags.empty() && &startFlag == &curFlag)
        return true;
    if(maxlen < 1)
        return false;
    for(Direction testDir : helpers::EnumRange<Direction>{})
    {
        if(testDir == excludeDir)
            continue;
        if(testDir == Direction::NorthWest
           && (aii.isObjectTypeOnNode(gwb.getNeighbour(curFlag.getPos(), Direction::NorthWest),
                                      NodalObjectType::Building)
               || aii.isObjectTypeOnNode(gwb.getNeighbour(curFlag.getPos(), Direction::NorthWest),
                                         NodalObjectType::Buildingsite)))
        {
            continue;
        }
        const RoadSegment* route = curFlag.getRoute(testDir);
        if(route)
        {
            const noFlag& flag = route->getOtherFlag(curFlag);
            if(!helpers::contains(oldFlags, &flag))
            {
                oldFlags.push_back(&flag);
                Direction revDir = route->getOtherFlagDir(curFlag) + 3u;
                if(isFlagPartofCircle(startFlag, maxlen - 1, flag, revDir, oldFlags))
                    return true;
            }
        }
    }
    return false;
}

void AIPlayerJH::removeAllUnusedRoads(const MapPoint pt)
{
    std::vector<const noFlag*> flags = construction->findFlags(pt, 25);
    // Jede Flagge testen...
    std::vector<const noFlag*> reconnectflags;
    for(const noFlag* flag : flags)
    {
        if(removeUnusedRoad(*flag, std::nullopt, true, false))
            reconnectflags.push_back(flag);
    }
    updateNodesAround(pt, 25);
    for(const noFlag* flag : reconnectflags)
        construction->addConnectFlagJob(flag);
}

void AIPlayerJH::checkForUnconnectedBuildingSites()
{
    if(construction->getConnectJobNum() > 0 || construction->getBuildJobNum() > 0)
        return;
    for(noBuildingSite* bldSite : player.getBuildingRegister().getBuildingSites()) //-V807
    {
        noFlag* flag = bldSite->getFlag();
        bool foundRoute = false;
        for(const auto dir : helpers::EnumRange<Direction>{})
        {
            if(dir == Direction::NorthWest)
                continue;
            if(flag->getRoute(dir))
            {
                foundRoute = true;
                break;
            }
        }
        if(!foundRoute)
            construction->addConnectFlagJob(flag);
    }
}

bool AIPlayerJH::removeUnusedRoad(const noFlag& startFlag, helpers::OptionalEnum<Direction> excludeDir,
                                  bool firstflag /*= true*/, bool allowcircle /*= true*/,
                                  bool keepstartflag /*= false*/)
{
    helpers::OptionalEnum<Direction> foundDir, foundDir2;
    unsigned char finds = 0;
    // Count roads from this flag...
    for(Direction dir : helpers::EnumRange<Direction>{})
    {
        if(dir == excludeDir)
            continue;
        if(dir == Direction::NorthWest
           && (aii.isObjectTypeOnNode(gwb.getNeighbour(startFlag.getPos(), Direction::NorthWest),
                                      NodalObjectType::Building)
               || aii.isObjectTypeOnNode(gwb.getNeighbour(startFlag.getPos(), Direction::NorthWest),
                                         NodalObjectType::Buildingsite)))
        {
            // the flag belongs to a building - update the pathing map around us and try to reconnect it (if we cant
            // reconnect it -> burn it(burning takes place at the pathfinding job))
            return true;
        }
        if(startFlag.getRoute(dir))
        {
            finds++;
            if(finds == 1)
                foundDir = dir;
            else if(finds == 2)
                foundDir2 = dir;
        }
    }
    // if we found more than 1 road -> the flag is still in use.
    if(finds > 2)
        return false;
    else if(finds == 2)
    {
        if(allowcircle)
        {
            if(!isFlagPartofCircle(startFlag, 10, startFlag, std::nullopt, {}))
                return false;
            if(!firstflag)
                return false;
        } else
            return false;
    }

    // kill the flag
    if(keepstartflag)
    {
        if(foundDir)
            aii.DestroyRoad(startFlag.getPos(), *foundDir);
    } else
        aii.DestroyFlag(&startFlag);

    // nothing found?
    if(!foundDir)
        return false;
    // at least 1 road exists
    Direction revDir1 = startFlag.getRoute(*foundDir)->getOtherFlagDir(startFlag) + 3u;
    removeUnusedRoad(startFlag.getRoute(*foundDir)->getOtherFlag(startFlag), revDir1, false);
    // 2 roads exist
    if(foundDir2)
    {
        Direction revDir2 = startFlag.getRoute(*foundDir2)->getOtherFlagDir(startFlag) + 3u;
        removeUnusedRoad(startFlag.getRoute(*foundDir2)->getOtherFlag(startFlag), revDir2, false);
    }
    return false;
}

unsigned AIPlayerJH::soldierAvailable(int rank)
{
    unsigned freeSoldiers = 0;
    for(const nobBaseWarehouse* wh : aii.getStorehouses())
    {
        const Inventory& inventory = wh->getInventory();
        if(rank < 0)
        {
            for(const Job job : SOLDIER_JOBS)
                freeSoldiers += inventory[job];
        } else
            freeSoldiers += inventory[SOLDIER_JOBS[rank]];
    }
    return freeSoldiers;
}

bool AIPlayerJH::huntablesInRange(const MapPoint pt, unsigned min)
{
    // check first if no other hunter (or hunter buildingsite) is nearby
    if(aii.isBuildingNearby(BuildingType::Hunter, pt, 14))
        return false;
    return nofHunter::getHuntableAnimalsInRange(gwb, pt).size() >= min;
}

void AIPlayerJH::initStoreAndMilitarylists()
{
    for(const nobUsual* farm : aii.getBuildings(BuildingType::Farm))
    {
        setFarmedNodes(farm->getPos(), true);
    }
    for(const nobUsual* charburner : aii.getBuildings(BuildingType::Charburner))
    {
        setFarmedNodes(charburner->getPos(), true);
    }
    // find the upgrade building
    updateUpgradeBuilding();
}

const nobMilitary* AIPlayerJH::updateUpgradeBuilding()
{
    std::vector<const nobMilitary*> backup;
    if(!aii.getStorehouses().empty())
    {
        for(const nobMilitary* milBld : aii.getMilitaryBuildings())
        {
            // inland building, tower or fortress
            BuildingType bld = milBld->getBuildingType();
            if((bld == BuildingType::Watchtower || bld == BuildingType::Fortress)
               && milBld->getFrontierDistance() == FrontierDistance::Far)
            {
                if(construction->isConnectedToRoadSystem(milBld->getFlag()))
                {
                    UpgradeBldPos = milBld->getPos();
                    return milBld;
                }
                backup.push_back(milBld);
            }
        }
    }
    // no valid upgrade building yet - try to reconnect correctly flagged buildings
    for(const nobMilitary* milBld : backup)
    {
        construction->addConnectFlagJob(milBld->getFlag());
    }
    UpgradeBldPos = MapPoint::invalid();
    return nullptr;
}
// set default start values for the ai for distribution & military settings
void AIPlayerJH::initDistribution()
{
    // set good distribution settings
    Distributions goodSettings;
    goodSettings[0] = 10; // food granite
    goodSettings[1] = 10; // food coal
    goodSettings[2] = 10; // food iron
    goodSettings[3] = 10; // food gold
    goodSettings[4] = 2;  // food temple

    goodSettings[5] = 10; // grain mill
    goodSettings[6] = 10; // grain pigfarm
    goodSettings[7] = 10; // grain donkeybreeder
    goodSettings[8] = 10; // grain brewery
    goodSettings[9] = 10; // grain charburner

    goodSettings[10] = 10; // iron armory
    goodSettings[11] = 10; // iron metalworks

    goodSettings[12] = 10; // coal armory
    goodSettings[13] = 10; // coal ironsmelter
    goodSettings[14] = 10; // coal mint

    goodSettings[15] = 10; // wood sawmill
    goodSettings[16] = 10; // wood charburner
    goodSettings[17] = 2;  // wood vineyard

    goodSettings[18] = 10; // boards new buildings
    goodSettings[19] = 4;  // boards metalworks
    goodSettings[20] = 2;  // boards shipyard
    goodSettings[21] = 1;  // boards tannery

    goodSettings[22] = 10; // water bakery
    goodSettings[23] = 10; // water brewery
    goodSettings[24] = 10; // water pigfarm
    goodSettings[25] = 10; // water donkeybreeder
    goodSettings[26] = 2;  // water vineyard

    goodSettings[27] = 8; // ham slaughterhouse
    goodSettings[28] = 3; // ham skinner
    aii.ChangeDistribution(goodSettings);
}

bool AIPlayerJH::validTreeinRange(const MapPoint pt)
{
    unsigned max_radius = 6;
    for(MapCoord tx = gwb.getXA(pt, Direction::West), r = 1; r <= max_radius;
        tx = gwb.getXA(MapPoint(tx, pt.y), Direction::West), ++r)
    {
        MapPoint t2(tx, pt.y);
        for(unsigned i = 2; i < 8; ++i)
        {
            for(MapCoord r2 = 0; r2 < r; t2 = gwb.getNeighbour(t2, convertToDirection(i)), ++r2)
            {
                // point has tree & path is available?
                if(gwb.getNO(t2)->getType() == NodalObjectType::Tree)
                {
                    // not already getting cut down or a freaking pineapple thingy?
                    if(!gwb.getNode(t2).reserved && gwb.getSpecObj<noTree>(t2)->producesWood())
                    {
                        if(gwb.findHumanPath(pt, t2, 20))
                            return true;
                    }
                }
            }
        }
    }
    return false;
}

bool AIPlayerJH::validStoneinRange(const MapPoint pt)
{
    unsigned max_radius = 8;
    for(MapCoord tx = gwb.getXA(pt, Direction::West), r = 1; r <= max_radius;
        tx = gwb.getXA(MapPoint(tx, pt.y), Direction::West), ++r)
    {
        MapPoint t2(tx, pt.y);
        for(unsigned i = 2; i < 8; ++i)
        {
            for(MapCoord r2 = 0; r2 < r; t2 = gwb.getNeighbour(t2, convertToDirection(i)), ++r2)
            {
                // point has tree & path is available?
                if(gwb.getNO(t2)->getType() == NodalObjectType::Granite)
                {
                    if(gwb.findHumanPath(pt, t2, 20))
                        return true;
                }
            }
        }
    }
    return false;
}

void AIPlayerJH::executeLuaConstructionOrder(const MapPoint pt, BuildingType bt, bool forced)
{
    if(!aii.canBuildBuildingtype(bt)) // not allowed to build this buildingtype? -> do nothing!
        return;
    if(forced) // fixed location - just a direct gamecommand to build buildingtype at location (no checks if this is a
               // valid & good location from the ai)
    {
        aii.SetBuildingSite(pt, bt);
        auto j = std::make_unique<BuildJob>(*this, bt, pt);
        j->setState(JobState::ExecutingRoad1);
        j->setTarget(pt);
        construction->addBuildJob(std::move(j), true); // connects the buildingsite to roadsystem
    } else
    {
        if(construction->wanted(bt))
        {
            construction->addBuildJob(std::make_unique<BuildJob>(*this, bt, pt),
                                      true); // add build job to the front of the list
        }
    }
}

/// returns the percentage*100 of possible normal+ building places
unsigned AIPlayerJH::bQsurroundcheck(const MapPoint pt, unsigned range, bool includeexisting, unsigned limit)
{
    unsigned maxvalue = 6 * (2 << (range - 1)) - 5; // 1,7,19,43,91,... = 6*2^range -5
    unsigned count = 0;
    RTTR_Assert(aii.getBuildingQuality(pt) == getAINode(pt).bq);
    if((aii.getBuildingQuality(pt) >= BuildingQuality::Hut && aii.getBuildingQuality(pt) <= BuildingQuality::Castle)
       || aii.getBuildingQuality(pt) == BuildingQuality::Harbor)
    {
        count++;
    }
    NodalObjectType nob = gwb.getNO(pt)->getType();
    if(includeexisting)
    {
        if(nob == NodalObjectType::Building || nob == NodalObjectType::Buildingsite || nob == NodalObjectType::Extension
           || nob == NodalObjectType::Fire || nob == NodalObjectType::CharburnerPile)
            count++;
    }
    // first count all the possible building places
    for(MapCoord tx = gwb.getXA(pt, Direction::West), r = 1; r <= range;
        tx = gwb.getXA(MapPoint(tx, pt.y), Direction::West), ++r)
    {
        MapPoint t2(tx, pt.y);
        for(unsigned i = 2; i < 8; ++i)
        {
            for(MapCoord r2 = 0; r2 < r; t2 = gwb.getNeighbour(t2, convertToDirection(i)), ++r2)
            {
                if(limit && ((count * 100) / maxvalue) > limit)
                    return ((count * 100) / maxvalue);
                // point can be used for a building
                if((aii.getBuildingQualityAnyOwner(t2) >= BuildingQuality::Hut
                    && aii.getBuildingQualityAnyOwner(t2) <= BuildingQuality::Castle)
                   || aii.getBuildingQualityAnyOwner(t2) == BuildingQuality::Harbor)
                {
                    count++;
                    continue;
                }
                if(includeexisting)
                {
                    nob = gwb.getNO(t2)->getType();
                    if(nob == NodalObjectType::Building || nob == NodalObjectType::Buildingsite
                       || nob == NodalObjectType::Extension || nob == NodalObjectType::Fire
                       || nob == NodalObjectType::CharburnerPile)
                        count++;
                }
            }
        }
    }
    // LOG.write(("bqcheck at %i,%i r%u result: %u,%u \n",pt,range,count,maxvalue);
    return ((count * 100) / maxvalue);
}

bool AIPlayerJH::harborPosRelevant(HarborId harborId, bool onlyempty) const
{
    if(!harborId || harborId.value() > gwb.getNumHarborPoints()) // not a real harbor - shouldnt happen...
    {
        RTTR_Assert(false);
        return false;
    }
    if(!onlyempty)
        return helpers::contains(aii.getUsableHarbors(), harborId);

    for(const auto dir : helpers::EnumRange<Direction>{})
    {
        const SeaId seaId = gwb.getSeaId(harborId, dir);
        if(!seaId)
            continue;

        for(const auto curHarborId : helpers::idRange<HarborId>(gwb.getNumHarborPoints()))
        {
            if(curHarborId != harborId && gwb.isHarborAtSea(HarborId(curHarborId), seaId))
            {
                // check if the spot is actually free for colonization?
                if(gwb.isHarborPointFree(HarborId(curHarborId), playerId))
                    return true;
            }
        }
    }
    return false;
}

bool AIPlayerJH::noEnemyHarbor()
{
    for(const auto i : helpers::idRange<HarborId>(gwb.getNumHarborPoints()))
    {
        if(aii.isBuildingOnNode(gwb.getHarborPoint(i), BuildingType::HarborBuilding)
           && !aii.isOwnTerritory(gwb.getHarborPoint(i)))
        {
            // LOG.write(("found a harbor at spot %i ",i);
            return false;
        }
    }
    return true;
}

bool AIPlayerJH::isInvalidShipyardPosition(const MapPoint pt)
{
    return aii.isBuildingNearby(BuildingType::Shipyard, pt, 19) || !aii.isHarborPosClose(pt, 7);
}

unsigned AIPlayerJH::amountInStorage(GoodType good) const
{
    unsigned counter = 0;
    for(const nobBaseWarehouse* wh : aii.getStorehouses())
        counter += wh->getInventory().goods[good];
    return counter;
}

unsigned AIPlayerJH::amountInStorage(::Job job) const
{
    unsigned counter = 0;
    for(const nobBaseWarehouse* wh : aii.getStorehouses())
        counter += wh->getInventory().people[job];
    return counter;
}

bool AIPlayerJH::validFishInRange(const MapPoint pt)
{
    unsigned max_radius = 5;
    return gwb.checkPointsInRadius(
      pt, max_radius,
      [this, pt](const MapPoint curPt, unsigned) {
          if(gwb.getNode(curPt).resources.has(ResourceType::Fish)) // fish on current spot?
          {
              // try to find a path to a neighboring node on the coast
              for(const MapPoint nb : gwb.getNeighbours(curPt))
              {
                  if(gwb.findHumanPath(pt, nb, 10))
                      return true;
              }
          }
          return false;
      },
      false);
}

unsigned AIPlayerJH::getNumAIRelevantSeaIds() const
{
    std::vector<SeaId> validseaids;
    std::list<SeaId> onetimeuseseaids;
    for(const auto i : helpers::idRange<HarborId>(gwb.getNumHarborPoints()))
    {
        for(const auto dir : helpers::EnumRange<Direction>{})
        {
            const SeaId seaId = gwb.getSeaId(i, dir);
            if(!seaId)
                continue;
            // there is a sea id? -> check if it is already a validid or a once found id
            if(!helpers::contains(validseaids, seaId)) // not yet in validseas?
            {
                if(!helpers::contains(onetimeuseseaids, seaId)) // not yet in onetimeuseseaids?
                    onetimeuseseaids.push_back(seaId);
                else
                {
                    // LOG.write(("found a second harbor at sea id %i \n",seaIds[r]);
                    onetimeuseseaids.remove(seaId);
                    validseaids.push_back(seaId);
                }
            }
        }
    }
    return validseaids.size();
}

void AIPlayerJH::adjustSettings()
{
    const Inventory& inventory = aii.getInventory();
    // update tool creation settings
    if(bldPlanner->getNumBuildings(BuildingType::Metalworks) > 0u)
    {
        ToolSettings toolsettings{};
        const auto calcToolPriority = [&](const Tool tool) {
            const GoodType good = TOOL_TO_GOOD[tool];
            unsigned numToolsAvailable = inventory[good];
            // Find missing jobs for buildings
            for(const auto job : helpers::enumRange<Job>())
            {
                if(JOB_CONSTS[job].tool != good)
                    continue;
                unsigned numBuildingsRequiringWorker = 0;
                for(const auto bld : helpers::enumRange<BuildingType>())
                {
                    if(BLD_WORK_DESC[bld].job == job)
                        numBuildingsRequiringWorker += bldPlanner->getNumBuildings(bld);
                }
                if(numBuildingsRequiringWorker > inventory[job])
                {
                    const unsigned requiredTools = numBuildingsRequiringWorker - inventory[job];
                    // When we are missing tools produce some.
                    // Slightly higher priority if we don't have any tool at all.
                    if(requiredTools > numToolsAvailable)
                        return (inventory[good] == 0) ? 4 : 2;
                    numToolsAvailable -= requiredTools;
                }
            }
            return 0;
        };
        // Basic tools to produce stone, boards and iron are very important to have, do those first
        for(const Tool tool : {Tool::Axe, Tool::Saw, Tool::PickAxe, Tool::Crucible})
            toolsettings[tool] = calcToolPriority(tool);
        // Set some minima
        if(inventory[GoodType::Saw] + inventory[Job::Carpenter] < 2)
            toolsettings[Tool::Saw] = 10;
        if(inventory[GoodType::Axe] + inventory[Job::Woodcutter] < 2)
            toolsettings[Tool::Axe] = 10;
        if(inventory[GoodType::PickAxe] + inventory[Job::Stonemason] < 2)
            toolsettings[Tool::PickAxe] = 7;
        // Only if we haven't ordered any basic tool, we may order other tools
        if(toolsettings[Tool::Axe] == 0 && toolsettings[Tool::PickAxe] == 0 && toolsettings[Tool::Saw] == 0
           && toolsettings[Tool::Crucible] == 0)
        {
            // Order those as required for existing and planned buildings
            for(const Tool tool : {Tool::Hammer, Tool::Scythe, Tool::Rollingpin, Tool::Shovel, Tool::Tongs,
                                   Tool::Cleaver, Tool::RodAndLine, Tool::Bow})
            {
                toolsettings[tool] = calcToolPriority(tool);
            }
            // Always have at least one of those in stock for other stuff
            for(const Tool tool : {Tool::Hammer, Tool::Shovel, Tool::Tongs})
            {
                if(inventory[TOOL_TO_GOOD[tool]] == 0)
                    toolsettings[tool] = std::max<unsigned>(toolsettings[tool], 1u);
            }
            // We want about 12 woodcutters, so if we don't have axes produce some
            if(inventory[GoodType::Axe] == 0 && inventory[Job::Woodcutter] < 12)
            {
                // Higher priority if we can't meet the building requirements as calculated above
                toolsettings[Tool::Axe] = (toolsettings[Tool::Axe] == 0) ? 4 : 7;
            }
        }

        for(const auto tool : helpers::enumRange<Tool>())
        {
            if(toolsettings[tool] != player.getToolPriority(tool))
            {
                aii.ChangeTools(toolsettings);
                break;
            }
        }
    }

    // Set military settings to some currently required values
    MilitarySettings milSettings;
    milSettings[0] = 10;
    milSettings[1] = hasFrontierBuildings() ?
                       5 :
                       0; // if we have a front send strong soldiers first else weak first to make upgrading easier
    milSettings[2] = 4;
    milSettings[3] = 5;
    // interior 0bar full if we have an upgrade building and gold(or produce gold) else 1 soldier each
    milSettings[4] = updateUpgradeBuilding()
                         && (inventory[GoodType::Coins] > 0
                             || (inventory[GoodType::Gold] > 0 && inventory[GoodType::Coal] > 0
                                 && !aii.getBuildings(BuildingType::Mint).empty())) ?
                       8 :
                       0;
    milSettings[6] =
      ggs.isEnabled(AddonId::SEA_ATTACK) ? 8 : 0; // harbor flag: no sea attacks?->no soldiers else 50% to 100%
    milSettings[5] = calcMilSettings(); // inland 1bar min 50% max 100% depending on how many soldiers are available
    milSettings[7] = 8;                 // front: 100%
    if(player.getMilitarySetting(5) != milSettings[5] || player.getMilitarySetting(6) != milSettings[6]
       || player.getMilitarySetting(4) != milSettings[4]
       || player.getMilitarySetting(1) != milSettings[1]) // only send the command if we want to change something
        aii.ChangeMilitary(milSettings);
}

unsigned AIPlayerJH::calcMilSettings()
{
    std::array<unsigned, 5> InlandTroops = {
      0, 0, 0, 0, 0}; // how many troops are required to fill inland buildings at settings 4,5,6,7,8
    /// first sum up all soldiers we have
    unsigned numSoldiers = 0;
    for(auto i : SOLDIER_JOBS)
        numSoldiers += aii.getInventory().people[i];

    // now add up all counts of soldiers that are fixed in use and those that depend on whatever we have as a result
    const unsigned numShouldStayConnected = getNumPlannedConnectedInlandMilitaryBlds();
    unsigned count = 0;
    unsigned soldierInUseFixed = 0;
    const auto* upgradeBld = updateUpgradeBuilding();
    const std::list<nobMilitary*>& militaryBuildings = aii.getMilitaryBuildings();
    for(const nobMilitary* milBld : militaryBuildings)
    {
        if(milBld->getFrontierDistance() == FrontierDistance::Near
           || milBld->getFrontierDistance() == FrontierDistance::Harbor
           || (milBld->getFrontierDistance() == FrontierDistance::Far
               && (count + numShouldStayConnected > militaryBuildings.size()
                   || milBld == upgradeBld))) // front or connected interior
        {
            soldierInUseFixed += milBld->calcRequiredNumTroops(FrontierDistance::Mid, 8);
        } else if(milBld->getFrontierDistance() == FrontierDistance::Mid) // 1 bar (inland)
        {
            for(int i = 0; i < 5; i++)
                InlandTroops[i] += milBld->calcRequiredNumTroops(FrontierDistance::Mid, 4 + i);
        } else // setting should be 0 so add 1 soldier
            soldierInUseFixed++;

        count++;
    }

    // now the current need total and for inland and harbor is ready for use
    unsigned returnValue = 8;
    while(returnValue > 4)
    {
        // have more than enough soldiers for this setting or just enough and this is the current setting? -> return it
        // else try the next lower setting down to 4 (50%)
        if(soldierInUseFixed + InlandTroops[returnValue - 4] < numSoldiers * 10 / 11
           || (player.getMilitarySetting(5) >= returnValue
               && soldierInUseFixed + InlandTroops[returnValue - 4] < numSoldiers))
            break;
        returnValue--;
    }
    // LOG.write(("player %i inland milsetting %i \n",playerId,returnvalue);
    return returnValue;
}

} // namespace AIJH
