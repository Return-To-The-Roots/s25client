// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nofActiveSoldier.h"
#include <vector>

class nofDefender;
class nofAggressiveDefender;
class nofPassiveSoldier;
class nobHarborBuilding;
class nobMilitary;
class noShip;
class SerializedGameData;
class nobBaseMilitary;
class GameEvent;

/// Attacking soldier
class nofAttacker : public nofActiveSoldier
{
    /// State whether a defender might be sent
    enum class SendDefender : uint8_t
    {
        No,
        Yes,
        Undecided /// To be determined
    };
    friend constexpr auto maxEnumValue(SendDefender) { return SendDefender::Undecided; }

    /// Building which is attacked by the soldier
    nobBaseMilitary* attacked_goal;
    /// Can we get chased by an aggressive defender?
    bool mayBeHunted;
    /// Whether we may send a defender to this player
    std::vector<SendDefender> canPlayerSendAggDefender;
    /// Defender who is currently chasing after this soldier
    nofAggressiveDefender* huntingDefender;
    /// Distance to flag when waiting around it
    unsigned short radius;
    /// Event when the road from/to the attacked buildings flag will be blocked.
    /// Only used for AttackingWaitingForDefender
    const GameEvent* blocking_event;

    /// For sea attacks: Position of the start harbor
    MapPoint harborPos;
    /// For sea attacks: Anchor point of the ship which can be used to go home
    MapPoint shipPos;
    unsigned ship_obj_id;

    void walked() override;
    /// Go home from an attack
    void returnHomeMissionAttacking();
    /// Continue walking during an attack
    void missAttackingWalk();
    /// Reached target military building
    void reachedDestination();
    /// Handle next step of walking into an already captured building
    void capturingWalking();
    /// Find a defender (coming out of a building, i.e. not the main defender at the flag) with a certain chance
    void tryToOrderAggressiveDefender();
    /// Actually do order an aggressive defender
    void orderAggressiveDefender();

    /// Doesn't find a defender at the flag -> Send defenders or capture it
    void continueAtFlag();

    /// Start waiting for a defender (at the flag) and register the block event
    void switchStateAttackingWaitingForDefender();

    /// Sea attackers: Notify ship that this soldier is not coming any more
    void cancelAtShip();
    void cancelAtHuntingDefender();
    /// Handle walking back to ship
    void handleStateSeaAttackReturnToShip();

protected:
    SoldierState freeFightAborted() override;

public:
    /// Notify all targets (or those that target us) that we won't be coming anymore
    void informTargetsAboutCancelling() override;

    void removeFromAttackedGoal();

    /// Create an attacker from a passive soldier, if harbor is set, the soldier will first walk there for a sea attack
    nofAttacker(const nofPassiveSoldier& other, nobBaseMilitary& attacked_goal,
                const nobHarborBuilding* harbor = nullptr);
    nofAttacker(SerializedGameData& sgd, unsigned obj_id);
    ~nofAttacker() override;

    void destroy() override;
    void serialize(SerializedGameData& sgd) const override;

    void abrogate() override;

    GO_Type getGOT() const final { return GO_Type::NofAttacker; }
    const nofAggressiveDefender* getHuntingDefender() const { return huntingDefender; }

    void handleDerivedEvent(unsigned id) override;
    /// Return whether the roads to the position of this soldier cannot be used
    bool isBlockingRoads() const;

    /// Notify the soldier about the destruction of the home building
    void homeDestroyed() override;
    /// Notify the soldier that the home building was destroyed while he was waiting to leave
    void homeDestroyedAtBegin() override;

    /// Called when a fight was won
    void wonFighting() override;
    /// Called when a fight was lost (i.e. the soldier will die)
    void lostFighting() override;

    /// Notify that the target building gone
    void attackedGoalDestroyed();
    /// The aggressive defender won't chase this soldier anymore
    void aggressiveDefenderLost();
    /// Go to the defender waiting at the flag
    bool attackDefenderAtFlag();
    /// Go to the flag without an available defender (e.g. just freed after a fight)
    void attackFlag();
    /// Go into a building that was just captured.
    void captureBuilding();
    /// Notification that the captured building is full
    void capturedBuildingFull();
    /// Return the distance to the flag at which the soldier is waiting
    unsigned getRadius() const { return radius; }
    /// A waiting soldier should move closer to the flag to fill gaps
    void startSucceeding(MapPoint pt, unsigned short new_radius);
    /// Return if this soldier is available to occupy the captured building
    bool canStartFarAwayCapturing(const nobMilitary& dest) const;

    /// The aggressive defender is going to fight with this soldier
    void letsFight(nofAggressiveDefender& other);

    /// Return if this attacker waits around the building for a fight
    bool isAttackerReady() const { return (state == SoldierState::AttackingWaitingAroundBuilding); }

    /// Return target building being attacked
    nobBaseMilitary* getAttackedGoal() const { return attacked_goal; }

    /// Start attack from anchor point of the ship
    void startAttackOnOtherIsland(MapPoint shipPos, unsigned ship_id);
    /// Call attacker back to the ship
    void startReturnViaShip(noShip& ship);
    /// Notification in a harbor before boarding a ship to the fight that e.g. the ship has no longer a valid route or
    /// target
    void seaAttackFailedBeforeLaunch();
    /// Tell soldier the sea attack failed and they can't go home either
    void cancelSeaAttack();
    /// notify sea attackers that they wont return home
    void homeHarborLost();
    /// Notification that the attacker is now on a ship
    void seaAttackStarted() { state = SoldierState::SeaattackingOnShip; }
    /// Return whether the soldier is on a ship waiting to attack, i.e. not already back from a fight
    bool isSeaAttackCompleted() const { return (state != SoldierState::SeaattackingOnShip); }
};
