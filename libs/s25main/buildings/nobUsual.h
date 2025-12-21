// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "noBuilding.h"
#include "gameTypes/GoodTypes.h"
#include <array>
#include <list>
#include <vector>

class Ware;
class nofBuildingWorker;
class SerializedGameData;
class noFigure;
class noRoadNode;
class GameEvent;

// Generic production building with one worker and input wares
class nobUsual : public noBuilding
{
protected:
    /// Worker assigned to this building
    nofBuildingWorker* worker;
    /// Current productivity value
    unsigned short productivity;
    /// Real and visual production suspension flags (visual flag hides latency)
    bool disableProduction, disableProductionVirtual;
    /// Index of the last ware type ordered (for buildings with multiple inputs)
    unsigned char lastOrderedWare;
    /// Resource counts required for production
    std::array<uint8_t, 3> numWares;
    /// Ordered wares per ware type
    std::vector<std::list<Ware*>> orderedWares;
    /// Event for ordering wares
    const GameEvent* orderware_ev;
    /// Event for recalculating productivity
    const GameEvent* productivity_ev;
    /// History of recent productivity values (used to compute averages, newest first)
    std::array<uint16_t, 6> lastProductivities;
    /// How many GFs he did not work since the last productivity calculation
    unsigned short numGfNotWorking;
    /// Total number of goods produced by this building
    unsigned totalProducedGoods;
    /// Since which GF he did not work (0xFFFFFFFF = currently working)
    unsigned sinceNotWorking;
    /// Did we notify the player that we are out of resources?
    bool outOfRessourcesMsgSent;

    friend class SerializedGameData;
    friend class BuildingFactory;
    nobUsual(BuildingType type, MapPoint pos, unsigned char player, Nation nation);
    nobUsual(SerializedGameData& sgd, unsigned obj_id);

    void DestroyBuilding() override;

public:
    /// Is the building currently working?
    bool is_working;

    ~nobUsual() override;

    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const override { return GO_Type::NobUsual; }
    unsigned GetMilitaryRadius() const override { return 0; }

    void Draw(DrawPoint drawPt) override;

    bool HasWorker() const;

    /// Event-Handler
    void HandleEvent(unsigned id) override;
    /// Place a ware at this node (any road node can accept deliveries)
    void AddWare(std::unique_ptr<Ware> ware) override;
    /// Triggered when a ware is picked up from the flag in front of the building
    bool FreePlaceAtFlag() override;
    /// Called when an ordered ware fails to arrive
    void WareLost(Ware& ware) override;
    /// Called when a worker has been assigned to this building
    void GotWorker(Job job, noFigure& worker) override;
    /// Called by the worker upon arrival
    void WorkerArrived();
    /// Called by the worker when they can no longer reach the workplace
    void WorkerLost();

    /// Return the stored quantity for the given ware slot (incoming minus consumed)
    unsigned char GetNumWares(unsigned id) const { return numWares[id]; }
    /// Check whether enough wares are available for the next work cycle
    bool WaresAvailable();
    /// Consume wares for production
    void ConsumeWares();

    /// Compute the priority score for requesting a ware
    unsigned CalcDistributionPoints(GoodType type);

    /// Called when a new ware arrives at the building (not just when it was ordered)
    void TakeWare(Ware* ware) override;

    /// Return whether any wares are currently on order
    bool AreThereAnyOrderedWares() const;

    /// Accessors for productivity and worker state
    const unsigned short* GetProductivityPointer() const { return &productivity; }
    unsigned short GetProductivity() const { return productivity; }
    const nofBuildingWorker* GetWorker() const { return worker; }
    unsigned GetTotalProducedGoods() const { return totalProducedGoods; }
    void RegisterProducedGood(GoodType good);

    /// Toggle production visually
    void ToggleProductionVirtual() { disableProductionVirtual = !disableProductionVirtual; }
    /// Enable or disable production logic
    void SetProductionEnabled(bool enabled);
    /// Query whether production is visually disabled
    bool IsProductionDisabledVirtual() const { return disableProductionVirtual; }
    /// Query whether production is actually disabled
    bool IsProductionDisabled() const { return disableProduction; }
    /// Called when there are no more resources
    void OnOutOfResources();
    /// Mark the building as idle for productivity tracking
    void StartNotWorking();
    /// Mark the building as working again for productivity tracking
    void StopNotWorking();

private:
    /// Calculates the productivity and resets the counter
    unsigned short CalcCurrentProductivity();
};
