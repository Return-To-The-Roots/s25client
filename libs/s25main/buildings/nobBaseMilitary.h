// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "buildings/noBuilding.h"
#include "helpers/PtrSpan.h"
#include <boost/container/flat_set.hpp>
#include <list>

class nofSoldier;
class nofActiveSoldier;
class nofAttacker;
class nofAggressiveDefender;
class nofDefender;
class SerializedGameData;
class noFigure;
class GameEvent;

/// Common base class for all military-style buildings (HQs, regular military buildings, harbors, and even
/// warehouses, which share many of the same characteristics).
class nobBaseMilitary : public noBuilding
{
protected:
    /// Figures queued to leave the building (prevents everyone from exiting at once)
    std::list<std::unique_ptr<noFigure>> leave_house;
    /// Event used to stagger departures
    const GameEvent* leaving_event;
    /// Flag indicating that someone is currently leaving (also prevents mass departures)
    bool go_out;
    /// Troops that belong to this building but are currently away on missions and might return
    std::list<nofActiveSoldier*> troops_on_mission;
    /// Soldiers currently attacking this building
    std::list<nofAttacker*> aggressors;
    /// Aggressive defenders assisting this building
    std::list<nofAggressiveDefender*> aggressive_defenders;
    /// The defender currently protecting the building
    nofDefender* defender_;
    /// Game frame of the latest capture by the current owner; 0 if never captured
    unsigned captured_gf_;
    /// Player who originally owned/built the building
    unsigned char origin_owner_;

public:
    nobBaseMilitary(BuildingType type, MapPoint pos, unsigned char player, Nation nation);
    nobBaseMilitary(SerializedGameData& sgd, unsigned obj_id);
    ~nobBaseMilitary() override;

protected:
    void DestroyBuilding() override;

public:
    void Serialize(SerializedGameData& sgd) const override;

    auto GetLeavingFigures() const { return helpers::nonNullPtrSpan(leave_house); }

    /// Retrieve the current defender
    const nofDefender* GetDefender() const { return defender_; }

    /// Compares according to build time (Age): Bigger objIds are "younger"
    bool operator<(const nobBaseMilitary& other) const { return GetObjId() > other.GetObjId(); }

    /// Schedule a new "leaving" event if no figure is currently exiting (prevents mass departures)
    void AddLeavingEvent();

    /// Add an active soldier returning from a mission to the garrison
    virtual void AddActiveSoldier(std::unique_ptr<nofActiveSoldier> soldier) = 0;

    /// Send out an aggressive defender to intercept an attacker
    virtual nofAggressiveDefender* SendAggressiveDefender(nofAttacker& attacker) = 0;

    /// Register and unregister soldiers currently attacking this building
    void LinkAggressor(nofAttacker& soldier) { aggressors.push_back(&soldier); }
    virtual void UnlinkAggressor(nofAttacker& soldier);

    /// Register and unregister aggressive defenders supporting this building
    void LinkAggressiveDefender(nofAggressiveDefender& soldier) { aggressive_defenders.push_back(&soldier); }
    void UnlinkAggressiveDefender(nofAggressiveDefender& soldier)
    {
        RTTR_Assert(IsAggressiveDefender(soldier));
        aggressive_defenders.remove(&soldier);
    }

    /// Called when a soldier can no longer reach this building
    virtual void SoldierLost(nofSoldier* soldier) = 0;

    /// Find an idle attacker who is eager to fight and near the building (used by aggressive defenders);
    /// returns nullptr if none is available
    nofAttacker* FindAggressor(nofAggressiveDefender& defender);
    /// Find the best spot near the flag for an attacker to occupy and wait
    MapPoint FindAnAttackerPlace(unsigned short& ret_radius, const nofAttacker& soldier);
    /// Find a successor further back in line and send them to the given position
    bool SendSuccessor(MapPoint pt, unsigned short radius);

    /// Return whether a defender is available and, if so, dispatch them against the attacker at the flag
    bool CallDefender(nofAttacker& attacker);
    /// Locate an attacker for this building and send them to the flag if appropriate
    nofAttacker* FindAttackerNearBuilding();
    /// After a flag battle, search for attackers that might have been blocked and send one to the flag if needed
    void CheckArrestedAttackers();
    /// Clear the defender reference when the defender returns or dies
    void NoDefender() { defender_ = nullptr; }
    /// Cancel an ongoing attack/aggressive defense initiated by this building, returning queued soldiers inside
    void CancelJobs();

    /// Determine whether defenders remain inside the building
    virtual bool DefendersAvailable() const = 0;

    /// Return the list of all current aggressors, that is enemy soldiers currently attacking this military building.
    const std::list<nofAttacker*>& GetAggressors() const { return aggressors; }
    unsigned GetCapturedGF() const { return captured_gf_; }
    unsigned char GetOriginOwner() const { return origin_owner_; }
    void SetCapturedGF(unsigned gf) { captured_gf_ = gf; }

    /// Return true if the military building is under attack.
    bool IsUnderAttack() const { return !aggressors.empty(); };

    /// Return whether this building can be attacked by the given player.
    virtual bool IsAttackable(unsigned playerIdx) const;

    /// Debugging
    bool IsAggressor(const nofAttacker& attacker) const;
    bool IsAggressiveDefender(const nofAggressiveDefender& soldier) const;
    bool IsOnMission(const nofActiveSoldier& soldier) const;
    const std::list<nofAggressiveDefender*>& GetAggresiveDefenders() const { return aggressive_defenders; }

    // Compare buildings by construction time to obtain an ordered sequence
    struct Comparer
    {
        bool operator()(const nobBaseMilitary* const one, const nobBaseMilitary* const two) const
        {
            return (*one) < (*two);
        }
    };

protected:
    /// The building shall provide a soldier for defense. Return nullptr if none available
    virtual std::unique_ptr<nofDefender> ProvideDefender(nofAttacker& attacker) = 0;
    /// Add a figure that will leave the house
    void AddLeavingFigure(std::unique_ptr<noFigure> fig);
};

class sortedMilitaryBlds : public boost::container::flat_set<nobBaseMilitary*, nobBaseMilitary::Comparer>
{};
