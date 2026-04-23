// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ai/AIEventManager.h"
#include "ai/AIPlayer.h"
#include "ai/aijh/combat/AICombatContext.h"
#include "ai/aijh/debug/AIDebugView.h"
#include "ai/aijh/debug/AIStatsSource.h"
#include "ai/aijh/runtime/AIPlanningContext.h"
#include "ai/aijh/runtime/AIMap.h"
#include "ai/aijh/runtime/AIResourceMap.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/SettingsTypes.h"
#include "helpers/EnumArray.h"
#include "helpers/OptionalEnum.h"

#include <list>
#include <memory>
#include <queue>
#include <vector>

class noFlag;
class noShip;
class nobBaseMilitary;
class nobMilitary;
class nobBaseWarehouse;
struct AIConfig;

namespace AIEvent {
class Base;
}

namespace AIJH {

class GlobalPositionFinder;
class BuildingPlanner;
class AIConstruction;
class AIJob;
class BuildJob;
class ConnectJob;
class EventJob;
class SearchJob;
class AIMapState;
class AIEconomyController;
class AIEventHandler;
class AIMilitaryLogistics;
class AIWorldQueries;
class AIStatsReporter;
class AIPerfReporter;
class AICombatController;
class AIRoadController;

/// Create a subscription which records all nodes for which the BQ (may) have changed
/// Requires arguments to have the same lifetime as the subscription
Subscription recordBQsToUpdate(const GameWorldBase& gw, std::vector<MapPoint>& bqsToUpdate);

/// Create a subscription which invalidates cached Borderland resource values on territory ownership changes.
/// Requires arguments to have the same lifetime as the subscription.
Subscription subscribeOwnerChangesToInvalidateBorderlandCache(const GameWorldBase& gw, AIQueryService& queries);

/// Concrete AI player implementation for the JH AI.
/// The public API lives here; internal helpers and state are declared in
/// `AIPlayerJHInternal.h`.
class AIPlayerJH final
    : public AIPlayer, public AIPlanningContext, public AICombatContext, public AIStatsSource, public AIDebugView
{
public:
    using TargetSelectionMode = AICombatTargetSelectionMode;

    AIPlayerJH(unsigned char playerId, const GameWorldBase& gwb, AI::Level level);
    ~AIPlayerJH() override;

    // Core orchestration and shared context
    const std::string& GetPlayerName() const { return AIPlayer::GetPlayerName(); }
    AIInterface& GetInterface() { return aii; }
    const AIInterface& GetInterface() const { return aii; }
    const AIConfig& GetConfig() const { return config_; }
    const GameWorldBase& GetWorld() const { return gwb; }
    const GamePlayer& GetPlayer() const { return player; }
    const GlobalGameSettings& GetGameSettings() const { return ggs; }
    unsigned char GetPlayerId() const { return AIPlayer::GetPlayerId(); }
    AI::Level GetLevel() const { return AIPlayer::GetLevel(); }
    AIConstruction& GetConstruction() { return *construction; }
    const BuildingPlanner& GetBldPlanner() const { return *bldPlanner; }
    const AIJob* GetCurrentJob() const { return currentJob.get(); }
    unsigned GetNumJobs() const;

    void RunGF(unsigned gf, bool gfisnwf) override;
    void OnChatMessage(unsigned sendPlayerId, ChatDestination, const std::string& msg) override;

    // Reporting and top-level state access
    void saveStats(unsigned gf) const override;
    unsigned long long GetResourceValueCacheHits() const override;
    unsigned long long GetResourceValueCacheMisses() const override;

    // Shared map and resource accessors
    bool IsInvalidShipyardPosition(MapPoint pt);

    int GetResMapValue(MapPoint pt, AIResource res) const;
    AIResourceMap& GetResMap(AIResource res);
    const AIResourceMap& GetResMap(AIResource res) const;

    Node& GetAINode(MapPoint pt);
    const Node& GetAINode(MapPoint pt) const;

    unsigned GetNumPlannedConnectedInlandMilitaryBlds()
    {
        return std::max<unsigned>(6u, aii.GetMilitaryBuildings().size() / 5u);
    }

    unsigned BQsurroundcheck(MapPoint pt, unsigned range, bool includeexisting, unsigned limit = 0);
    unsigned AmountInStorage(GoodType good) const;
    unsigned AmountInStorage(Job job) const;
    unsigned GetNumAIRelevantSeaIds() const;

    // Planner- and debug-facing queries
    MapPoint FindBestPosition(const MapPoint& pt, AIResource res, BuildingQuality size, unsigned radius,
                              int minimum = 1);
    MapPoint SimpleFindPosition(const MapPoint& pt, BuildingType type, unsigned radius) const;
    MapPoint FindPositionForBuildingAround(BuildingType type, const MapPoint& around);

    unsigned GetAvailableResources(AISurfaceResource resource) const;
    unsigned GetDensity(MapPoint pt, AIResource res, int radius);

    const helpers::EnumArray<unsigned, GoodType>& GetProducedGoods() const;
    unsigned GetProductivity(BuildingType type) const;
    // Legacy collaboration API for jobs/controllers until narrower context
    // interfaces replace direct `AIPlayerJH` access.
    MapPoint FindBestPosition(BuildingType bt);
    void RecordGlobalPositionSearchInvocation() override;
    void RecordGlobalPositionSearchCooldownSkip() override;
    void AddBuildJob(BuildingType type, MapPoint pt, bool front = false, bool searchPosition = true);
    void AddGlobalBuildJob(BuildingType type);
    void AddMilitaryBuildJob(MapPoint pt);
    void UpdateNodesAround(MapPoint pt, unsigned radius);
    void RecalcGround(MapPoint buildingPos, std::vector<Direction>& route_road);
    void SetFarmedNodes(MapPoint pt, bool set);
    void ExecuteLuaConstructionOrder(MapPoint pt, BuildingType bt, bool forced = false);
    void HandleNewMilitaryBuildingOccupied(MapPoint pt);
    void HandleMilitaryBuildingLost(MapPoint pt);
    void HandleBuildingDestroyed(MapPoint pt, BuildingType bld);
    void HandleNoMoreResourcesReachable(MapPoint pt, BuildingType bld);
    void HandleShipBuilt(MapPoint pt);
    void HandleRoadConstructionComplete(MapPoint pt, Direction dir);
    void HandleRoadConstructionFailed(MapPoint pt, Direction dir);
    void HandleBorderChanged(MapPoint pt);
    void HandleBuildingFinished(MapPoint pt, BuildingType bld);
    void HandleExpedition(MapPoint pt);
    void HandleExpedition(const noShip* ship);
    void HandleTreeChopped(MapPoint pt);
    void HandleNewColonyFounded(MapPoint pt);
    void HandleLostLand(MapPoint pt);
    bool IsRecentlyLostMilitaryBuilding(MapPoint pt) const;
    void TrackCombatStart(const nobBaseMilitary& target);

private:
#define RTTR_AIJH_INCLUDE_AI_PLAYER_JH_INTERNAL
#include "ai/aijh/runtime/AIPlayerJHInternal.h"
#undef RTTR_AIJH_INCLUDE_AI_PLAYER_JH_INTERNAL
};

} // namespace AIJH
