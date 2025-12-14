// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "figures/nofSoldier.h"
#include "nobBaseMilitary.h"
#include <boost/container/flat_set.hpp>
#include <algorithm>
#include <list>
#include <vector>

class GameEvent;
class nobHarborBuilding;
class nofActiveSoldier;
class nofAggressiveDefender;
class nofAttacker;
class nofDefender;
class noFigure;
class nofPassiveSoldier;
class SerializedGameData;
class Ware;

/// Distance to the next enemy border
enum class FrontierDistance : uint8_t
{
    Far,    /// next military building is far away
    Mid,    /// Next military building is in reachable range
    Harbor, /// Military building is near a harbor
    Near    /// Military building is next to a border
};
constexpr auto maxEnumValue(FrontierDistance)
{
    return FrontierDistance::Near;
}

/// Represents a military building of any size (from guardhouse up to fortress)
class nobMilitary : public nobBaseMilitary
{
    using OwnedSortedTroops = boost::container::flat_set<std::unique_ptr<nofPassiveSoldier>, ComparatorSoldiersByRank>;
    using SortedTroops = boost::container::flat_set<nofPassiveSoldier*, ComparatorSoldiersByRank>;

private:
    /// Tracks whether the building is newly constructed (land borders must be recalculated when troops arrive)
    bool new_built;
    /// Number of gold coins stored inside the building
    unsigned char numCoins;
    /// Indicates whether gold coin delivery is disabled (visual flag masks network latency)
    bool coinsDisabled, coinsDisabledVirtual;
    /// Cached chance that the building will be captured soon
    double captureRisk_ = 0.0;
    /// Distance to the enemy border (drives garrison size); values: 0 far, 3 near, 2 harbor
    FrontierDistance frontier_distance;
    /// Size/type of the military building (0 = barracks, 3 = fortress)
    unsigned char size;
    /// Soldiers that have been ordered
    SortedTroops ordered_troops;
    /// Ordered gold coins
    std::list<Ware*> ordered_coins;
    /// Signals that conquerors are entering the building (prevent further attacks)
    bool capturing;
    /// Number of soldiers currently capturing the building
    unsigned capturing_soldiers;
    /// List of soldiers who are on the way to capture the military building
    /// but who are still quite far away (didn't stand around the building)
    std::list<nofAttacker*> far_away_capturers;
    /// Event for ordering gold coins
    const GameEvent* goldorder_event;
    /// Event for scheduling promotions
    const GameEvent* upgrade_event;
    /// Is the military building regulating its troops at the moment? (then block furthere RegulateTroop calls)
    bool is_regulating_troops;
    /// Stationed soldiers
    OwnedSortedTroops troops;
    std::array<uint8_t, NUM_SOLDIER_RANKS> troop_limits;
    unsigned total_troop_limit;

    /// Cancel all pending orders (soldiers and coins)
    void CancelOrders();
    /// Select an appropriate soldier based on the military defense setting
    nofPassiveSoldier* ChooseSoldier();
    /// Provide a defender for the building
    std::unique_ptr<nofDefender> ProvideDefender(nofAttacker& attacker) override;
    /// Determine whether the building can or should receive more coins
    bool WantCoins() const;
    /// Checks whether any stationed soldier can still be promoted
    bool HasUpgradeableSoldier() const;
    /// Check for coins and promotable soldiers and schedule a promotion event if possible
    void PrepareUpgrading();
    /// Gets the total amount of soldiers (ordered, stationed, on mission)
    size_t GetTotalSoldiers() const;
    std::array<unsigned, NUM_SOLDIER_RANKS> GetTotalSoldiersByRank() const;
    /// Looks for the next far-away-capturer waiting around and calls it to the flag
    void CallNextFarAwayCapturer(nofAttacker& attacker);
    unsigned CalcDefenderBonusHp() const;

    friend class SerializedGameData;
    friend class BuildingFactory;
    nobMilitary(BuildingType type, MapPoint pos, unsigned char player, Nation nation);
    nobMilitary(SerializedGameData& sgd, unsigned obj_id);

public:
    ~nobMilitary() override;

protected:
    void DestroyBuilding() override;

public:
    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const final { return GO_Type::NobMilitary; }

    void Draw(DrawPoint drawPt) override;
    void HandleEvent(unsigned id) override;

    /// Was the military building recently constructed and still unoccupied (can it be dismantled after losing land)?
    bool IsNewBuilt() const { return new_built; }

    /// Return the military control radius of the building
    unsigned GetMilitaryRadius() const override;
    unsigned GetMaxCoinCt() const;
    unsigned GetMaxTroopsCt() const;

    /// Scan for enemy military buildings to update the frontier distance (both for this and nearby buildings) and
    /// request reinforcements if necessary; an optional exception building can be ignored
    void LookForEnemyBuildings(const nobBaseMilitary* exception = nullptr);

    /// Called by enemy buildings when a new structure appears nearby to recalculate distances and possibly request
    /// reinforcements
    void NewEnemyMilitaryBuilding(FrontierDistance distance);
    bool IsUseless() const;
    bool IsAttackable(unsigned playerIdx) const override;
    /// Return the current frontier distance
    FrontierDistance GetFrontierDistance() const { return frontier_distance; }

    /// Calculate the desired garrison size based on border proximity
    unsigned CalcRequiredNumTroops() const;
    /// Calculate the required troop count for the given setting
    unsigned CalcRequiredNumTroops(FrontierDistance assumedFrontierDistance, unsigned settingValue) const;
    /// Adjust the garrison according to border distance, ordering or dismissing soldiers as needed
    void RegulateTroops();
    /// Return the current number of stationed troops
    unsigned GetNumTroops() const { return troops.size(); }
    auto GetTroops() const { return helpers::nonNullPtrSpan(troops); }
    bool IsInTroops(const nofPassiveSoldier& soldier) const;

    /// Get/Set the maximum number of soldiers of rank `rank` allowed in this building
    unsigned GetTroopLimit(const unsigned rank) const { return troop_limits[rank]; }
    void SetTroopLimit(unsigned rank, unsigned limit);
    unsigned GetTotalTroopLimit() const { return total_troop_limit; }
    void SetTotalTroopLimit(unsigned limit);

    /// Called when a new ware (gold coin) is delivered to the building
    void TakeWare(Ware* ware) override;
    /// Place a ware at this node (any road node can accept wares)
    void AddWare(std::unique_ptr<Ware> ware) override;
    /// Called when an ordered ware fails to arrive
    void WareLost(Ware& ware) override;
    /// Triggered when a resource is picked up from the flag in front of the building
    bool FreePlaceAtFlag() override;

    /// Calculate how urgently the building needs a gold coin (higher value means higher priority)
    unsigned CalcCoinsPoints() const;

    /// Called when a soldier arrives
    void GotWorker(Job job, noFigure& worker) override;
    /// Add an active soldier returning from a mission to the garrison
    void AddActiveSoldier(std::unique_ptr<nofActiveSoldier> soldier) override;
    /// Add a passive soldier arriving from a warehouse to the garrison
    void AddPassiveSoldier(std::unique_ptr<nofPassiveSoldier> soldier);
    /// Called when a soldier fails to arrive
    void SoldierLost(nofSoldier* soldier) override;
    /// Send the given passive soldier from this building to attack the goal, optionally via the given harbor
    void SendAttacker(nofPassiveSoldier*& passive_soldier, nobBaseMilitary& goal,
                      const nobHarborBuilding* harbor = nullptr);

    /// Schickt einen Verteidiger raus, der einem Angreifer in den Weg rennt
    nofAggressiveDefender* SendAggressiveDefender(nofAttacker& attacker) override;

    /// Return the number of soldiers available to attack the specified destination
    unsigned GetNumSoldiersForAttack(MapPoint dest) const;
    /// Return the list of soldiers available to attack the specified destination
    std::vector<nofPassiveSoldier*> GetSoldiersForAttack(MapPoint dest) const;
    /// Return the combined strength of soldiers available to attack the specified destination
    unsigned GetSoldiersStrengthForAttack(MapPoint dest, unsigned& soldiers_count) const;
    /// Return this building's overall military strength
    unsigned GetSoldiersStrength() const;
    /// Return the total strength of stationed soldiers including potential defender bonuses
    unsigned GetGarrisonStrengthWithBonus() const;
    /// Return the last-estimated capture risk
    double GetCaptureRiskEstimate() const { return captureRisk_; }
    /// Update the stored capture risk probability
    void SetCaptureRiskEstimate(double value) { captureRisk_ = std::clamp(value, 0.0, 1.0); }

    /// Handle the building being captured by an enemy (player becomes the new owner)
    void Capture(unsigned char new_owner);
    /// After capture, determine whether additional occupying soldiers are needed; call or dismiss troops accordingly
    void NeedOccupyingTroops();
    /// Notify the building that it is being captured when the first conqueror enters, preventing further entries
    void PrepareCapturing()
    {
        RTTR_Assert(!IsBeingCaptured());
        capturing = true;
        ++capturing_soldiers;
    }

    /// Is the building currently being captured?
    bool IsBeingCaptured() const { return capturing; }
    /// Reset capturing state (e.g., if another soldier occupied it first)
    void StopCapturing()
    {
        capturing_soldiers = 0;
        capturing = false;
    }
    /// Notify that a capturing soldier reached the building
    void CapturingSoldierArrived();
    /// A far-away capturer arrived at the flag and starts the capturing or is waiting around it
    void FarAwayCapturerReachedGoal(nofAttacker& attacker, bool walkingIntoBld);

    bool IsFarAwayCapturer(const nofAttacker& attacker);

    /// Toggle coin delivery visually
    void ToggleCoinsVirtual() { coinsDisabledVirtual = !coinsDisabledVirtual; }
    /// Enable or disable actual coin delivery
    void SetCoinsAllowed(bool enabled);
    /// Query whether coin delivery is visually disabled
    bool IsGoldDisabledVirtual() const { return coinsDisabledVirtual; }
    /// Query whether coin delivery is actually disabled
    bool IsGoldDisabled() const { return coinsDisabled; }
    unsigned char GetNumCoins() const { return numCoins; }
    /// is there a max rank soldier in the building?
    bool HasMaxRankSoldier() const;

    /// Scan all warehouses for gold coins and order one if required
    void SearchCoins();

    /// Handle the building being hit by a catapult stone
    void HitOfCatapultStone();

    /// Determine whether defenders remain inside the building
    bool DefendersAvailable() const override { return GetNumTroops() > 0; }

    /// Check whether the building may be demolished (respecting demolition restrictions)
    bool IsDemolitionAllowed() const;

    void UnlinkAggressor(nofAttacker& soldier) override;
};
