// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AIPlayerJH.h"

#include "ai/aijh/combat/AICombatController.h"
#include "ai/aijh/config/AIConfig.h"
#include "ai/aijh/debug/AIPerfReporter.h"
#include "ai/aijh/debug/AIRuntimeProfiler.h"
#include "ai/aijh/debug/AIStatsReporter.h"
#include "ai/aijh/debug/StatsConfig.h"
#include "ai/aijh/planning/AIConstruction.h"
#include "ai/aijh/planning/BuildingPlanner.h"
#include "ai/aijh/planning/GlobalPositionFinder.h"
#include "ai/aijh/runtime/AIEconomyController.h"
#include "ai/aijh/runtime/AIEventHandler.h"
#include "ai/aijh/runtime/AIMapState.h"
#include "ai/aijh/runtime/AIMilitaryLogistics.h"
#include "ai/aijh/runtime/AIRoadController.h"
#include "ai/aijh/runtime/AIWorldQueries.h"
#include "addons/const_addons.h"
#include "ai/AIEvents.h"
#include "helpers/containerUtils.h"
#include "network/GameMessages.h"
#include "notifications/BuildingNote.h"
#include "notifications/ExpeditionNote.h"
#include "notifications/NodeNote.h"
#include "notifications/ProductionNote.h"
#include "notifications/ResourceNote.h"
#include "notifications/RoadNote.h"
#include "notifications/ShipNote.h"
#include "GlobalGameSettings.h"

#include <memory>
#include <stdexcept>

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
    eventMgr.AddAIEvent(std::move(ev));
}

void HandleExpeditionNote(AIEventManager& eventMgr, const ExpeditionNote& note)
{
    switch(note.type)
    {
        case ExpeditionNote::Waiting:
            eventMgr.AddAIEvent(std::make_unique<AIEvent::Location>(AIEvent::EventType::ExpeditionWaiting, note.pos));
            break;
        case ExpeditionNote::ColonyFounded:
            eventMgr.AddAIEvent(std::make_unique<AIEvent::Location>(AIEvent::EventType::NewColonyFounded, note.pos));
            break;
    }
}

void HandleResourceNote(AIEventManager& eventMgr, const ResourceNote& note)
{
    eventMgr.AddAIEvent(std::make_unique<AIEvent::Resource>(AIEvent::EventType::ResourceFound, note.pos, note.res));
}

void HandleRoadNote(AIEventManager& eventMgr, const RoadNote& note)
{
    switch(note.type)
    {
        case RoadNote::Constructed:
            eventMgr.AddAIEvent(std::make_unique<AIEvent::Direction>(AIEvent::EventType::RoadConstructionComplete,
                                                                     note.pos, note.route.front()));
            break;
        case RoadNote::ConstructionFailed:
            eventMgr.AddAIEvent(std::make_unique<AIEvent::Direction>(AIEvent::EventType::RoadConstructionFailed,
                                                                     note.pos, note.route.front()));
            break;
    }
}

void HandleShipNote(AIEventManager& eventMgr, const ShipNote& note)
{
    if(note.type == ShipNote::Constructed)
        eventMgr.AddAIEvent(std::make_unique<AIEvent::Location>(AIEvent::EventType::ShipBuilt, note.pos));
}

} // namespace

namespace AIJH {

Subscription recordBQsToUpdate(const GameWorldBase& gw, std::vector<MapPoint>& bqsToUpdate)
{
    auto addToBqsToUpdate = [&bqsToUpdate](const MapPoint pt, unsigned) {
        bqsToUpdate.push_back(pt);
        return false;
    };
    return gw.GetNotifications().subscribe<NodeNote>([&gw, addToBqsToUpdate](const NodeNote& note) {
        if(note.type == NodeNote::BQ)
        {
            // Need to check surrounding nodes for possible/impossible flags (e.g. near border)
            gw.CheckPointsInRadius(note.pos, 1, addToBqsToUpdate, true);
        } else if(note.type == NodeNote::Owner)
        {
            // Owner changes border, which changes where buildings can be placed next to it
            // And as flags are need for buildings we need range 2 (e.g. range 1 is flag, range 2 building)
            gw.CheckPointsInRadius(note.pos, 2, addToBqsToUpdate, true);
        }
    });
}

AIPlayerJH::AIPlayerJH(const unsigned char playerId, const GameWorldBase& gwb, const AI::Level level)
    : AIPlayer(playerId, gwb, level), config_(GetAIConfigForPlayer(playerId)),
      mapState_(std::make_unique<AIMapState>(*this)),
      economyController_(std::make_unique<AIEconomyController>(*this)),
      eventHandler_(std::make_unique<AIEventHandler>(*this)),
      militaryLogistics_(std::make_unique<AIMilitaryLogistics>(*this)),
      worldQueries_(std::make_unique<AIWorldQueries>(*this)),
      isInitGfCompleted(false), defeated(player.IsDefeated()),
      bldPlanner(std::make_unique<BuildingPlanner>(*this)),
      construction(std::make_unique<AIConstruction>(*this)),
      globalPositionFinder(std::make_unique<GlobalPositionFinder>(*this)),
      statsReporter_(std::make_unique<AIStatsReporter>(*this)),
      perfReporter_(std::make_unique<AIPerfReporter>(*this)),
      combatController_(std::make_unique<AICombatController>(*this)),
      roadController_(std::make_unique<AIRoadController>(*this))
{
    aii.Queries().SetReserveMilitaryBorderSlots(config_.reserveMilitaryBorderSlots);
    aii.Queries().SetReserveMilitaryBorderlandThreshold(config_.reserveMilitaryBorderlandThreshold);

    InitNodes();
    InitResourceMaps();
#ifdef DEBUG_AI
    SaveResourceMapsToFile();
#endif

    attack_interval = config_.combat.attackIntervals[level];
    switch(config_.combat.targetSelection)
    {
        case TargetSelectionAlgorithm::Prudent:
            combatController_->SetTargetSelectionMode(AICombatController::TargetSelectionMode::Prudent);
            break;
        case TargetSelectionAlgorithm::Biting:
            combatController_->SetTargetSelectionMode(AICombatController::TargetSelectionMode::Biting);
            break;
        case TargetSelectionAlgorithm::Attrition:
            combatController_->SetTargetSelectionMode(AICombatController::TargetSelectionMode::Attrition);
            break;
        default:
            combatController_->SetTargetSelectionMode(AICombatController::TargetSelectionMode::Prudent);
            break;
    }

    switch(level)
    {
        case AI::Level::Easy:
            build_interval = 1000;
            break;
        case AI::Level::Medium:
            build_interval = 400;
            break;
        case AI::Level::Hard:
            build_interval = 200;
            break;
        default: throw std::invalid_argument("Invalid AI level!");
    }

    NotificationManager& notifications = gwb.GetNotifications();
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
    subProduction = notifications.subscribe<ProductionNote>([this, playerId](const ProductionNote& note) noexcept {
        if(note.player == playerId)
            economyController_->RecordProducedGood(note.good);
    });
    subBQ = recordBQsToUpdate(this->gwb, mapState_->GetNodesWithOutdatedBQ());
}

AIPlayerJH::~AIPlayerJH() = default;

void AIPlayerJH::RunGF(const unsigned gf, bool gfisnwf)
{
    currentGF_ = gf;
    militaryLogistics_->PruneRecentlyLostBuildings();

    if(gf == 0)
        InitializeCombatsLogFile();
    LogFinishedCombats(gf);
    perfReporter_->MaybeLog(gf);

    const ScopedAIRuntimeProfile runGfProfile(AIRuntimeProfileSection::RunGF);

    if(IsStatsPeriodHit(gf, STATS_CONFIG.stats_period))
        saveStats(gf);
    if(IsStatsPeriodHit(gf, STATS_CONFIG.debug_stats_period))
        saveDebugStats(gf);
    if(defeated)
        return;

    if(TestDefeat())
        return;

    if(isInitGfCompleted == 0)
    {
        InitStoreAndMilitarylists();
        InitDistribution();
        isInitGfCompleted = 1;
    }
    if(isInitGfCompleted < 10)
    {
        isInitGfCompleted++;
        return; //  1 init -> 2 test defeat -> 3 do other ai stuff -> goto 2
    }
    if(gf == 100)
    {
        if(aii.GetMilitaryBuildings().empty() && aii.GetStorehouses().size() < 2)
            aii.Chat(_("Hi, I'm an artifical player and I'm not very good yet!"));
    }

    {
        const ScopedAIRuntimeProfile refreshBuildingQualitiesProfile(AIRuntimeProfileSection::RefreshBuildingQualities);
        mapState_->RefreshBuildingQualities();
    }

    {
        const ScopedAIRuntimeProfile plannerUpdateProfile(AIRuntimeProfileSection::BuildingPlannerUpdate);
        bldPlanner->Update(gf, *this);
    }

    if(gfisnwf)
        construction->ConstructionsExecuted();

    if(gf % 100 == 0) {
        bldPlanner->UpdateBuildingsWanted(*this);
        const ScopedAIRuntimeProfile executeAiJobProfile(AIRuntimeProfileSection::ExecuteAIJob);
        ExecuteAIJob();
    }

    // if((gf + playerId * 29) % 500 == 0)
    // {
    //     const ScopedAIRuntimeProfile captureRisksProfile(AIRuntimeProfileSection::EvaluateCaptureRisks);
    //     EvaluateCaptureRisks();
    // }

    if((gf + playerId * 17) % attack_interval == 0)
    {
        const ScopedAIRuntimeProfile tryToAttackProfile(AIRuntimeProfileSection::TryToAttack);
        TryToAttack();
    }
    if(((gf + playerId * 17) % 73 == 0) && (level != AI::Level::Easy))
    {
        // MilUpgradeOptim();
    }

    if((gf + 41 + playerId * 17) % attack_interval == 0)
    {
        if(ggs.isEnabled(AddonId::SEA_ATTACK))
        {
            const ScopedAIRuntimeProfile seaAttackProfile(AIRuntimeProfileSection::TrySeaAttack);
            TrySeaAttack();
        }
    }

    if((gf + playerId * 13) % 1500 == 0)
    {
        const ScopedAIRuntimeProfile economicHotspotsProfile(AIRuntimeProfileSection::CheckEconomicHotspots);
        CheckExpeditions();
        CheckForester();
        CheckGraniteMine();
    }

    if((gf + playerId * 19) % 1000 == 0)
    {
        const ScopedAIRuntimeProfile troopsLimitProfile(AIRuntimeProfileSection::UpdateTroopsLimit);
        UpdateTroopsLimit();
    }

    if((gf + playerId * 11) % 150 == 0)
    {
        const ScopedAIRuntimeProfile adjustSettingsProfile(AIRuntimeProfileSection::AdjustSettings);
        AdjustSettings();
    }

    if((gf + playerId * 7) % build_interval == 0)
    {
        CheckForUnconnectedBuildingSites();
        const ScopedAIRuntimeProfile newBuildingsProfile(AIRuntimeProfileSection::PlanNewBuildings);
        PlanNewBuildings(gf);
    }
}

void AIPlayerJH::OnChatMessage(unsigned /*sendPlayerId*/, ChatDestination, const std::string& /*msg*/) {}

bool AIPlayerJH::TestDefeat()
{
    if(isInitGfCompleted >= 10 && aii.GetStorehouses().empty())
    {
        defeated = true;
        aii.Surrender();
        aii.Chat(_("You win"));
        return true;
    }
    return false;
}

unsigned AIPlayerJH::GetNumJobs() const
{
    return eventManager.GetEventNum() + construction->GetBuildJobNum() + construction->GetConnectJobNum();
}

} // namespace AIJH
