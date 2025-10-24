// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "DataChangedObservable.h"
#include "nobBaseMilitary.h"
#include "variant.h"
#include "gameTypes/GoodsAndPeopleArray.h"
#include "gameTypes/InventorySetting.h"
#include "gameTypes/VirtualInventory.h"
#include <array>
#include <list>
#include <memory>

class nofCarrier;
class noFigure;
class Ware;
class nobMilitary;
class TradeRoute;

class RoadSegment;
class SerializedGameData;
class noBaseBuilding;
class noRoadNode;
class nofActiveSoldier;
class nofAggressiveDefender;
class nofAttacker;
class nofDefender;
class nofSoldier;
class GameEvent;

namespace gc {
class SetInventorySetting;
class SetAllInventorySettings;
} // namespace gc

/// Structure describing storage import/export settings
struct InventorySettings : GoodsAndPeopleArray<InventorySetting>
{};

/// Base class for warehouses (HQs, storehouses, harbors) bundling their shared functionality.
/// Change events: 1 = InventorySettings
class nobBaseWarehouse : public nobBaseMilitary, public DataChangedObservable
{
protected:
    // Wares waiting to be dispatched but currently blocked because the flag is full
    std::list<std::unique_ptr<Ware>> waiting_wares;
    // Prevents collecting two wares at once
    bool fetch_double_protection;
    /// Figures currently heading to or leaving from this warehouse (includes soldiers)
    std::list<noFigure*> dependent_figures;
    /// Wares currently en route to the warehouse
    std::list<Ware*> dependent_wares;
    /// Event that produces additional carriers
    const GameEvent* producinghelpers_event;
    /// Soldier recruitment event
    const GameEvent* recruiting_event;
    /// Event that handles sending out wares and figures
    const GameEvent* empty_event;
    /// Event that handles storing wares and figures
    const GameEvent* store_event;

    /// Soldier reserve configuration
    std::array<unsigned, 5> reserve_soldiers_available;      /// soldiers currently stored in reserve
    std::array<unsigned, 5> reserve_soldiers_claimed_visual; /// requested reserve soldiers (visual state)
    std::array<unsigned, 5> reserve_soldiers_claimed_real;   /// requested reserve soldiers (authoritative state)

    /// Inventory of the building, real is the usable amount, visual is the total amount currently in the wh
    VirtualInventory inventory;
    InventorySettings inventorySettingsVisual; /// inventory settings as shown to the player
    InventorySettings inventorySettings;       /// authoritative inventory settings

private:
    /// Check whether all recruitment prerequisites are met
    bool AreRecruitingConditionsComply();
    /// Allow derived classes to consume a freshly created ware immediately (return true if consumed)
    virtual bool UseWareAtOnce(std::unique_ptr<Ware>& ware, noBaseBuilding& goal);
    /// Same for figures
    virtual bool UseFigureAtOnce(std::unique_ptr<noFigure>& fig, noRoadNode& goal);
    /// Evaluate potential uses for a newly arrived ware
    void CheckUsesForNewWare(GoodType gt);
    /// Handle a new figure entering the warehouse (assignment, recruitment, etc.)
    void CheckJobsForNewFigure(Job job);

    friend class gc::SetInventorySetting;
    friend class gc::SetAllInventorySettings;
    /// Change a single import/export setting
    void SetInventorySetting(const boost_variant2<GoodType, Job>& what, InventorySetting state);

    /// Change all import/export settings for either wares or figures (authoritative)
    void SetAllInventorySettings(bool isJob, const std::vector<InventorySetting>& states);

    /// Trigger outhousing for a specific ware or job type when allowed
    void CheckOuthousing(const boost_variant2<GoodType, Job>& what);
    void HandleCollectEvent();
    void HandleSendoutEvent();
    void HandleRecrutingEvent();
    void HandleProduceHelperEvent();
    void HandleLeaveEvent();

protected:
    /// Provide a defender for this building
    std::unique_ptr<nofDefender> ProvideDefender(nofAttacker& attacker) override;

    void HandleBaseEvent(unsigned id);

    /// Try to schedule a recruitment event when enough weapons, beer, and helpers are available (per military settings)
    void TryRecruiting();
    /// Cancel the recruitment event once the requirements are no longer met (e.g., goods were removed)
    void TryStopRecruiting();
    /// Merge the current stock into the inventory totals
    void AddToInventory();
    /// Recruit a worker of the given job if possible
    bool TryRecruitJob(Job job);

    nobBaseWarehouse(BuildingType type, MapPoint pos, unsigned char player, Nation nation);
    nobBaseWarehouse(SerializedGameData& sgd, unsigned obj_id);

public:
    void Clear();

    ~nobBaseWarehouse() override;

protected:
    void DestroyBuilding() override;

public:
    void Serialize(SerializedGameData& sgd) const override;

    const Inventory& GetInventory() const;

    /// Adds specified goods. If addToPlayer is true,
    /// then they are also added to the owners inventory (for newly created/arrived goods)
    /// Use false for goods, that are only moved between players units
    void AddGoods(const Inventory& goods, bool addToPlayer);

    /// Return the number of wares or figures stored
    unsigned GetNumRealWares(GoodType type) const { return inventory.real[type]; }
    unsigned GetNumRealFigures(Job job) const { return inventory.real[job]; }
    unsigned GetNumVisualWares(GoodType type) const { return inventory.visual[type]; }
    unsigned GetNumVisualFigures(Job job) const { return inventory.visual[job]; }

    /// Get the import/export settings
    InventorySetting GetInventorySettingVisual(Job job) const;
    InventorySetting GetInventorySettingVisual(GoodType ware) const;
    InventorySetting GetInventorySetting(Job job) const;
    InventorySetting GetInventorySetting(GoodType ware) const;
    // Convenience functions
    bool IsInventorySettingVisual(const Job job, const EInventorySetting setting) const
    {
        return GetInventorySettingVisual(job).IsSet(setting);
    }
    bool IsInventorySettingVisual(const GoodType ware, const EInventorySetting setting) const
    {
        return GetInventorySettingVisual(ware).IsSet(setting);
    }
    bool IsInventorySetting(const Job job, const EInventorySetting setting) const
    {
        return GetInventorySetting(job).IsSet(setting);
    }
    bool IsInventorySetting(const GoodType ware, const EInventorySetting setting) const
    {
        return GetInventorySetting(ware).IsSet(setting);
    }

    void SetInventorySettingVisual(const boost_variant2<GoodType, Job>& what, InventorySetting state);

    /// Order a carrier
    void OrderCarrier(noRoadNode& goal, RoadSegment& workplace);
    /// Order a profession (may craft the corresponding tool if needed)
    bool OrderJob(Job job, noRoadNode* goal, bool allow_recruiting);
    /// Order a donkey
    nofCarrier* OrderDonkey(RoadSegment* road, noRoadNode* goal_flag);
    /// Order a ware and return the pointer to it
    Ware* OrderWare(GoodType good, noBaseBuilding* goal);
    /// Returns true, if the given job can be recruited. Excludes soldiers and carriers!
    bool CanRecruit(Job job) const;

    /// Called when warehouse workers return a ware they could not drop at the flag
    void AddWaitingWare(std::unique_ptr<Ware> ware);
    /// Triggered when a resource is picked up from the flag
    bool FreePlaceAtFlag() override;
    // A ware is waiting at the flag and needs to be carried inside
    void FetchWare();
    // Skip collecting the next ware
    void DontFetchNextWare() { fetch_double_protection = true; }

    /// Store a ware inside the warehouse
    void AddWare(std::unique_ptr<Ware> ware) override;
    /// Add a figure to the warehouse
    virtual void AddFigure(std::unique_ptr<noFigure> figure, bool increase_visual_counts = true);

    /// Called when an ordered ware fails to arrive
    void WareLost(Ware& ware) override;
    /// Cancel delivery for an ordered ware that is still waiting here
    void CancelWare(Ware*& ware);
    /// Cancel a queued figure that can no longer arrive
    virtual void CancelFigure(noFigure* figure);

    /// Called when a newly delivered ware arrives (not when it is ordered)
    void TakeWare(Ware* ware) override;

    /// Register a figure that is on the way to the warehouse
    void AddDependentFigure(noFigure& figure)
    {
        RTTR_Assert(!IsDependentFigure(figure));
        dependent_figures.push_back(&figure);
    }
    //// Remove a dependent figure from the tracking list
    virtual void RemoveDependentFigure(noFigure& figure)
    {
        RTTR_Assert(IsDependentFigure(figure));
        dependent_figures.remove(&figure);
    }
    /// Wird aufgerufen, wenn ein Arbeiter hierher kommt
    void GotWorker(Job /*job*/, noFigure& worker) override
    {
        RTTR_Assert(!IsDependentFigure(worker));
        dependent_figures.push_back(&worker);
    }

    //// Remove a dependent ware from the list (added via TakeWare)
    void RemoveDependentWare(Ware& ware)
    {
        RTTR_Assert(IsWareDependent(ware));
        dependent_wares.remove(&ware);
    }
    /// Check whether a ware is tracked as dependent
    bool IsWareDependent(const Ware& ware);
    /// Determine whether there are wares pending export
    bool AreWaresToEmpty() const;

    /// Add an active soldier returning from a mission to the garrison
    void AddActiveSoldier(std::unique_ptr<nofActiveSoldier> soldier) override;
    /// Return the total number of soldiers stored in the warehouse
    unsigned GetNumSoldiers() const
    {
        return GetNumRealFigures(Job::Private) + GetNumRealFigures(Job::PrivateFirstClass)
               + GetNumRealFigures(Job::Sergeant) + GetNumRealFigures(Job::Officer) + GetNumRealFigures(Job::General);
    }
    /// Order troops of each rank according to `counts` without exceeding `max` in total. The number of soldiers
    /// of each rank that is sent out is subtracted from the corresponding count in `counts` and from `max`.
    void OrderTroops(nobMilitary* goal, std::array<unsigned, NUM_SOLDIER_RANKS>& counts, unsigned& max);

    /// Dispatch an aggressive defender that intercepts an attacker
    nofAggressiveDefender* SendAggressiveDefender(nofAttacker& attacker) override;
    /// Called when an ordered soldier fails to arrive
    void SoldierLost(nofSoldier* soldier) override;

    /// Check whether the warehouse still houses defenders
    bool DefendersAvailable() const override;

    /// Adjust reserve settings
    void SetReserveVisual(unsigned rank, unsigned count);
    void SetRealReserve(unsigned rank, unsigned count);

    /// Try to provide the requested reserve soldiers
    void RefreshReserve(unsigned rank);

    /// Return pointers to reserve values for UI consumption
    const unsigned* GetReserveAvailablePointer(unsigned rank) const { return &reserve_soldiers_available[rank]; }
    const unsigned* GetReserveClaimedVisualPointer(unsigned rank) const
    {
        return &reserve_soldiers_claimed_visual[rank];
    }
    unsigned GetReserveClaimed(unsigned rank) const { return reserve_soldiers_claimed_real[rank]; }

    /// Available goods of a specific type that can be used for trading
    unsigned GetAvailableWaresForTrading(GoodType gt) const;
    /// Available figures of a specific type that can be used for trading
    unsigned GetAvailableFiguresForTrading(Job job) const;
    /// Starts a trade caravane from this warehouse
    void StartTradeCaravane(const boost_variant2<GoodType, Job>& what, unsigned count, const TradeRoute& tr,
                            nobBaseWarehouse* goal);

    /// For debug only
    bool IsDependentFigure(const noFigure& fig) const;
};
