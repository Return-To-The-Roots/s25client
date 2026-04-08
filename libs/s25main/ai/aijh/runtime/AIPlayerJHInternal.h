// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef RTTR_AIJH_INCLUDE_AI_PLAYER_JH_INTERNAL
#error "Include AIPlayerJHInternal.h only from AIPlayerJH.h"
#endif

    // Lifecycle orchestration
    friend class AIMapState;
    friend class AIEconomyController;
    friend class AIEventHandler;
    friend class AIMilitaryLogistics;
    friend class AIWorldQueries;

    bool TestDefeat();
    void saveDebugStats(unsigned gf) const;
    void PlanNewBuildings(unsigned gf);
    void SendAIEvent(std::unique_ptr<AIEvent::Base> ev);
    void ExecuteAIJob();
    void InitNodes();
    void InitResourceMaps();
    void InitStoreAndMilitarylists();
    void InitDistribution();
    void AdjustSettings();
    unsigned CalcMilSettings();
    void UpdateTroopsLimit();

    // Planner-facing helpers
    void AddBuildJobAroundEveryWarehouse(BuildingType bt);
    void AddBuildJobAroundEveryMilBld(BuildingType bt);
    void DistributeGoodsByBlocking(GoodType good, unsigned limit);
    void DistributeMaxRankSoldiersByBlocking(unsigned limit, nobBaseWarehouse* upwh);
    int UpdateUpgradeBuilding();
    bool HasFrontierBuildings();
    nobBaseWarehouse* GetUpgradeBuildingWarehouse();
    void SetSendingForUpgradeWarehouse(nobBaseWarehouse* upgradewarehouse);
    void SetGatheringForUpgradeWarehouse(nobBaseWarehouse* upgradewarehouse);
    AINodeResource CalcResource(MapPoint pt);
    bool HarborPosRelevant(unsigned harborid, bool onlyempty = false) const;
    unsigned SoldierAvailable(int rank = -1);
    bool HuntablesinRange(MapPoint pt, unsigned min);
    bool ValidTreeinRange(MapPoint pt);
    bool ValidStoneinRange(MapPoint pt);
    bool ValidFishInRange(MapPoint pt);
    bool NoEnemyHarbor();

    // Event handling and runtime maintenance
    void CheckExpeditions();
    void CheckForester();
    void CheckGraniteMine();
    void TryToAttack();
    const nobBaseMilitary* SelectAttackTarget(TargetSelectionMode mode) const;
    const nobBaseMilitary* SelectAttackTargetRandom() const;
    const nobBaseMilitary* SelectAttackTargetPrudent() const;
    const nobBaseMilitary* SelectAttackTargetBiting() const;
    const nobBaseMilitary* SelectAttackTargetAttrition() const;
    void TrySeaAttack();
    void SaveResourceMapsToFile();
    void InitReachableNodes();
    void IterativeReachableNodeChecker(std::queue<MapPoint> toCheck);
    void UpdateReachableNodes(const std::vector<MapPoint>& pts);
    void MilUpgradeOptim();
    double GetCombatFulfillmentLevel() const;
    double GetCombatAttackWeight() const;
    bool IsInDefenseMode() const;
    double GetCaptureRiskEstimate(const nobBaseMilitary& building) const;

    // Core state and combat bookkeeping
    const AIConfig& config_;

    void UpdateCombatMode();
    bool CanAttackInDefenseMode(const nobBaseMilitary& target, unsigned attackersCount) const;
    bool IsLonelyEnemyStronghold(const nobBaseMilitary& target) const;
    double ComputeFulfillmentLevel(double* outTotalWeight = nullptr) const;
    double ComputeEnemyFrontlineWeight() const;
    void LogFinishedCombats(unsigned gf) const;
    void InitializeCombatsLogFile() const;
    void RememberLostMilitaryBuilding(MapPoint pt);
    void ForgetLostMilitaryBuilding(MapPoint pt);
    void PruneRecentlyLostBuildings();
    bool IsFlagPartOfCircle(const noFlag& startFlag, unsigned maxlen, const noFlag& curFlag,
                            helpers::OptionalEnum<Direction> excludeDir, std::vector<const noFlag*> oldFlags);
    bool RemoveUnusedRoad(const noFlag& startFlag, helpers::OptionalEnum<Direction> excludeDir, bool firstflag = true,
                          bool allowcircle = true, bool keepstartflag = false);
    void RemoveAllUnusedRoads(MapPoint pt);
    void CheckForUnconnectedBuildingSites();
    std::vector<const nobBaseMilitary*> GetPotentialTargets(unsigned& hq_or_harbor_without_soldiers) const;
    unsigned CalcPotentialAttackers(const nobBaseMilitary& target) const;
    void EvaluateCaptureRisks();
    double ComputeCaptureRisk(const nobMilitary& building) const;
    void AdjustDistribution();

    std::unique_ptr<AIJob> currentJob;
    std::list<MapPoint> milBuildings;
    std::list<MapPoint> milBuildingSites;
    std::unique_ptr<AIMapState> mapState_;
    std::unique_ptr<AIEconomyController> economyController_;
    std::unique_ptr<AIEventHandler> eventHandler_;
    std::unique_ptr<AIMilitaryLogistics> militaryLogistics_;
    std::unique_ptr<AIWorldQueries> worldQueries_;

    unsigned attack_interval;
    unsigned build_interval;
    int isInitGfCompleted;
    bool defeated;
    AIEventManager eventManager;
    std::unique_ptr<BuildingPlanner> bldPlanner;
    std::unique_ptr<AIConstruction> construction;
    std::unique_ptr<GlobalPositionFinder> globalPositionFinder;
    std::unique_ptr<AIStatsReporter> statsReporter_;
    std::unique_ptr<AICombatController> combatController_;
    std::unique_ptr<AIRoadController> roadController_;

    Subscription subBuilding, subExpedition, subResource, subRoad, subShip, subProduction, subBQ;
    unsigned currentGF_ = 0;
