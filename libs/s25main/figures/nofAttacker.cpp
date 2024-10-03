// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofAttacker.h"
#include "EventManager.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "SerializedGameData.h"
#include "addons/const_addons.h"
#include "buildings/nobHarborBuilding.h"
#include "buildings/nobMilitary.h"
#include "helpers/containerUtils.h"
#include "nofAggressiveDefender.h"
#include "nofDefender.h"
#include "nofPassiveSoldier.h"
#include "postSystem/PostMsgWithBuilding.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include "nodeObjs/noFighting.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noShip.h"
#include "gameData/BuildingProperties.h"

/// The time the attacker stands at the buildings flag before it starts blocking the road
/// Only used for AttackingWaitingfordefender
constexpr unsigned BLOCK_OFFSET = 10;

nofAttacker::nofAttacker(const nofPassiveSoldier& other, nobBaseMilitary& attacked_goal,
                         const nobHarborBuilding* const harbor)
    : nofActiveSoldier(other, harbor ? SoldierState::SeaattackingGoToHarbor : SoldierState::AttackingWalkingToGoal),
      attacked_goal(&attacked_goal), mayBeHunted(true),
      canPlayerSendAggDefender(world->GetNumPlayers(), SendDefender::Undecided), huntingDefender(nullptr), radius(0),
      blocking_event(nullptr), harborPos(harbor ? harbor->GetPos() : MapPoint::Invalid()), shipPos(MapPoint::Invalid()),
      ship_obj_id(0)
{
    attacked_goal.LinkAggressor(*this);
}

nofAttacker::~nofAttacker() = default;

void nofAttacker::Destroy()
{
    RTTR_Assert(!attacked_goal);
    RTTR_Assert(!ship_obj_id);
    RTTR_Assert(!huntingDefender);
    nofActiveSoldier::Destroy();

    /*unsigned char oplayer = (player == 0) ? 1 : 0;
    RTTR_Assert(!world->GetPlayer(oplayer).GetFirstWH()->Test(this));*/
}

void nofAttacker::Serialize(SerializedGameData& sgd) const
{
    nofActiveSoldier::Serialize(sgd);

    if(state != SoldierState::WalkingHome && state != SoldierState::FigureWork)
    {
        sgd.PushObject(attacked_goal);
        sgd.PushBool(mayBeHunted);
        helpers::pushContainer(sgd, canPlayerSendAggDefender);
        sgd.PushObject(huntingDefender, true);
        sgd.PushUnsignedShort(radius);

        if(state == SoldierState::AttackingWaitingForDefender)
            sgd.PushEvent(blocking_event);

        helpers::pushPoint(sgd, harborPos);
        helpers::pushPoint(sgd, shipPos);
        sgd.PushUnsignedInt(ship_obj_id);
    } else
    {
        RTTR_Assert(!attacked_goal);
        RTTR_Assert(!huntingDefender);
        RTTR_Assert(!blocking_event);
    }
}

nofAttacker::nofAttacker(SerializedGameData& sgd, const unsigned obj_id) : nofActiveSoldier(sgd, obj_id)
{
    if(state != SoldierState::WalkingHome && state != SoldierState::FigureWork)
    {
        attacked_goal = sgd.PopObject<nobBaseMilitary>();
        mayBeHunted = sgd.PopBool();
        sgd.PopContainer(canPlayerSendAggDefender);
        RTTR_Assert(canPlayerSendAggDefender.size() == world->GetNumPlayers());
        huntingDefender = sgd.PopObject<nofAggressiveDefender>(GO_Type::NofAggressivedefender);

        radius = sgd.PopUnsignedShort();

        if(state == SoldierState::AttackingWaitingForDefender)
            blocking_event = sgd.PopEvent();
        else
            blocking_event = nullptr;

        harborPos = sgd.PopMapPoint();
        shipPos = sgd.PopMapPoint();
        ship_obj_id = sgd.PopUnsignedInt();
    } else
    {
        attacked_goal = nullptr;
        mayBeHunted = false;
        canPlayerSendAggDefender.resize(world->GetNumPlayers(), SendDefender::Undecided);
        huntingDefender = nullptr;
        radius = 0;
        blocking_event = nullptr;
        harborPos = MapPoint::Invalid();
        shipPos = MapPoint::Invalid(); //-V656
        ship_obj_id = 0;
    }
}

void nofAttacker::Walked()
{
    ExpelEnemies();

    switch(state)
    {
        default: nofActiveSoldier::Walked(); break;
        case SoldierState::AttackingWalkingToGoal: MissAttackingWalk(); break;
        case SoldierState::AttackingAttackingFlag:
        {
            // If target is destroyed go home
            if(!attacked_goal)
            {
                ReturnHomeMissionAttacking();
                return;
            }

            const MapPoint goalFlagPos = attacked_goal->GetFlagPos();

            nofDefender* defender = nullptr;
            // Look for defenders at this position
            for(auto& figure : world->GetFigures(goalFlagPos))
            {
                if(figure.GetGOT() == GO_Type::NofDefender)
                {
                    // Is the defender waiting at the flag?
                    // (could be wandering around or something)
                    if(static_cast<nofDefender&>(figure).IsWaitingAtFlag())
                    {
                        defender = &static_cast<nofDefender&>(figure);
                        break;
                    }
                }
            }

            // Are we at the flag?
            if(pos == goalFlagPos)
            {
                if(defender)
                {
                    // Start fight with the defender
                    world->AddFigure(pos, std::make_unique<noFighting>(*this, *defender));
                    state = SoldierState::AttackingFightingVsDefender;
                    defender->FightStarted();
                } else
                {
                    // No defender at the flag?
                    // -> Order new defenders or capture the building
                    ContinueAtFlag();
                }
            } else
            {
                const auto dir = world->FindHumanPath(pos, goalFlagPos, 5, true);
                if(dir)
                    StartWalking(*dir);
                else
                {
                    // If there is no path anymore walk in the direction of the goal (or go back home)
                    state = SoldierState::AttackingWalkingToGoal;
                    MissAttackingWalk();
                    // Notify defender if any
                    if(defender)
                        defender->AttackerArrested();
                }
            }
        }
        break;
        case SoldierState::AttackingCapturingFirst:
        {
            if(!attacked_goal)
            {
                // Goal destroyed -> Go gome
                ReturnHomeMissionAttacking();
                return;
            }

            // If there is a defender available then an enemy soldier went back in
            if(attacked_goal->DefendersAvailable())
            {
                // Go back out
                if(attacked_goal->GetGOT() == GO_Type::NobMilitary)
                    static_cast<nobMilitary*>(attacked_goal)->StopCapturing();

                state = SoldierState::AttackingWalkingToGoal;
                StartWalking(Direction::SouthEast);
            } else
            {
                // Did we just conquer a "regular" military building?
                if(BuildingProperties::IsMilitary(attacked_goal->GetBuildingType()))
                {
                    RTTR_Assert(dynamic_cast<nobMilitary*>(attacked_goal));
                    // We will now have this building as the new home, so inform old home, hunting soldier and ship
                    if(building)
                        building->SoldierLost(this);
                    CancelAtHuntingDefender();
                    if(ship_obj_id)
                        CancelAtShip();
                    // Store the goal in a temporary as attacked_goal will be reset
                    auto* goal = static_cast<nobMilitary*>(attacked_goal);
                    goal->Capture(player);
                    building = attacked_goal;
                    attacked_goal->AddActiveSoldier(world->RemoveFigure(pos, *this));
                    RemoveFromAttackedGoal();
                    // This might call other capturers
                    goal->CapturingSoldierArrived();
                } else // Something like HQ or harbor
                {
                    // Inform the owner of the building and destroy it
                    const std::string msg = (attacked_goal->GetGOT() == GO_Type::NobHq) ?
                                              _("Our headquarters was destroyed!") :
                                              _("This harbor building was destroyed");
                    SendPostMessage(attacked_goal->GetPlayer(),
                                    std::make_unique<PostMsgWithBuilding>(GetEvMgr().GetCurrentGF(), msg,
                                                                          PostCategory::Military, *attacked_goal));

                    nobBaseMilitary* tmp_goal = attacked_goal; // Store in a temporary in case it gets reset by Destroy
                    tmp_goal->Destroy();
                    delete tmp_goal;
                    attacked_goal = nullptr;
                    ReturnHomeMissionAttacking();
                }
            }
        }
        break;
        case SoldierState::AttackingCapturingNext: CapturingWalking(); break;

        case SoldierState::SeaattackingGoToHarbor:
        {
            // Abort if the harbor doesn't exist, belongs to another player or the attacked building is gone
            const noBase* hb = world->GetNO(harborPos);
            const bool valid_harbor = hb->GetGOT() == GO_Type::NobHarborbuilding
                                      && static_cast<const nobHarborBuilding*>(hb)->GetPlayer() == player;

            if(!valid_harbor || !attacked_goal)
            {
                ReturnHomeMissionAttacking();
                return;
            }

            if(pos == harborPos) // When we arrived add to harbor
            {
                state = SoldierState::SeaattackingWaitInHarbor;
                world->GetSpecObj<nobHarborBuilding>(pos)->AddSeaAttacker(world->RemoveFigure(pos, *this));
            } else // Go to flag then walk inside the building
            {
                const MapPoint harborFlagPos = world->GetNeighbour(harborPos, Direction::SouthEast);

                if(pos == harborFlagPos)
                    StartWalking(Direction::NorthWest);
                else
                {
                    const auto dir =
                      world->FindHumanPath(pos, harborFlagPos, MAX_ATTACKING_RUN_DISTANCE, false, nullptr);
                    if(dir)
                        StartWalking(*dir);
                    else
                        ReturnHomeMissionAttacking(); // No possible path -> Abort
                }
            }
        }
        break;
        case SoldierState::SeaattackingWaitInHarbor: break;
        case SoldierState::SeaattackingOnShip:
            // Can't walk while on the ship
            RTTR_Assert(false);
            break;
        case SoldierState::SeaattackingReturnToShip: HandleState_SeaAttack_ReturnToShip(); break;
    }
}

void nofAttacker::HomeDestroyed()
{
    // If we are waiting, no events will trigger -> Start wandering right now,
    // else continue current event (e.g. movement) and handle at the end of that
    if(state == SoldierState::AttackingWaitingAroundBuilding)
    {
        // We are lost now so no valid source or target of attacks
        nobBaseMilitary* curGoal = attacked_goal; // attacked_goal gets reset
        InformTargetsAboutCancelling();
        if(ship_obj_id)
            CancelAtShip();

        building = nullptr;
        state = SoldierState::FigureWork;
        StartWandering();
        Wander();

        // Someone else might take this place
        curGoal->SendSuccessor(pos, radius);
    } else
    {
        // If we were going back home, reset that goal
        if(goal_ == building)
            goal_ = nullptr;
        building = nullptr;
    }
}

void nofAttacker::HomeDestroyedAtBegin()
{
    building = nullptr;

    // We are lost now and hence not targetable
    InformTargetsAboutCancelling();

    state = SoldierState::FigureWork;

    StartWandering();
    StartWalking(RANDOM_ENUM(Direction));
}

void nofAttacker::WonFighting()
{
    if(world->GetGGS().isEnabled(AddonId::BATTLEFIELD_PROMOTION))
        IncreaseRank();
    // If our home was destroyed then we are lost
    // unless we are currently fighting at the flag so that building can become our new home
    if(!building && state != SoldierState::AttackingFightingVsDefender)
    {
        // Lost -> Tell all dependents
        InformTargetsAboutCancelling();
        if(ship_obj_id)
            CancelAtShip();

        state = SoldierState::FigureWork;
        StartWandering();
        Wander();
    } else if(!attacked_goal)
    {
        // Target is gone -> Go home
        ReturnHomeMissionAttacking();
    } else
        ContinueAtFlag();
}

void nofAttacker::ContinueAtFlag()
{
    RTTR_Assert(attacked_goal);
    // If we were fighting at the flag of the goal then call a new defender
    // or capture the building if there isn't one anymore
    if(state == SoldierState::AttackingFightingVsDefender
       || (state == SoldierState::Fighting && attacked_goal->GetFlagPos() == pos))
    {
        if(attacked_goal->CallDefender(*this)) //-V522
            SwitchStateAttackingWaitingForDefender();
        else if(!TryFightingNearbyEnemy(attacked_goal->GetPlayer()))
        {
            // No defender or soldiers of other non-friendly non-owner players to fight
            // -> Enter building
            state = SoldierState::AttackingCapturingFirst;
            StartWalking(Direction::NorthWest);

            // Regular military buildings will be captured
            if(attacked_goal->GetGOT() == GO_Type::NobMilitary)
                static_cast<nobMilitary*>(attacked_goal)->PrepareCapturing();
        }
    } else
    {
        // Go (back) to goal
        state = SoldierState::AttackingWalkingToGoal;
        MissAttackingWalk();
    }
}

void nofAttacker::LostFighting()
{
    // Inform home building (so e.g. new troops can be ordered) and all dependents
    AbrogateWorkplace();
    InformTargetsAboutCancelling();
    if(ship_obj_id)
        this->CancelAtShip();
}

void nofAttacker::ReturnHomeMissionAttacking()
{
    InformTargetsAboutCancelling(); // We don't attack anymore
    if(ship_obj_id)
    {
        state = SoldierState::SeaattackingReturnToShip;
        HandleState_SeaAttack_ReturnToShip();
    } else
        ReturnHome();
}

void nofAttacker::MissAttackingWalk()
{
    // If our home is destroyed we are lost
    if(!building)
    {
        InformTargetsAboutCancelling();
        if(ship_obj_id)
            CancelAtShip();

        state = SoldierState::FigureWork;
        StartWandering();
        Wander();

        return;
    }

    // When target is gone go home
    if(!attacked_goal)
    {
        ReturnHomeMissionAttacking();
        return;
    }

    // Find a position next to the target
    const MapPoint goal = attacked_goal->FindAnAttackerPlace(radius, *this);

    if(!goal.isValid()) // Go home if there is no free spot
        ReturnHomeMissionAttacking();
    else if(pos == goal)
        ReachedDestination();
    else if(!TryFightingNearbyEnemy())
    {
        // If no Enemy (attackers, aggressive defenders..) was found (would be handled by nofActiveSoldier)
        // check if some building might send a defender and walk to goal

        TryToOrderAggressiveDefender();

        const auto dir = world->FindHumanPath(pos, goal, MAX_ATTACKING_RUN_DISTANCE, true);
        if(dir)
            StartWalking(*dir);
        else
            ReturnHomeMissionAttacking();
    }
}

void nofAttacker::ReachedDestination()
{
    // Are we at the flag of the target?
    const MapPoint attFlagPos = attacked_goal->GetFlagPos();
    if(pos == attFlagPos)
    {
        // If building is already captured continue capturing
        // (This can only be a far away attacker)
        if(attacked_goal->GetPlayer() == player)
        {
            state = SoldierState::AttackingCapturingNext;
            RTTR_Assert(dynamic_cast<nobMilitary*>(attacked_goal));
            auto* goal = static_cast<nobMilitary*>(attacked_goal);
            RTTR_Assert(goal->IsFarAwayCapturer(*this));
            // Start walking first so the flag is free
            StartWalking(Direction::NorthWest);
            // Then tell the building
            goal->FarAwayCapturerReachedGoal(*this, true);
        } else
        {
            // Notify player
            // TODO: Possibly improve that not every attacker does that
            SendPostMessage(attacked_goal->GetPlayer(),
                            std::make_unique<PostMsgWithBuilding>(GetEvMgr().GetCurrentGF(), _("We are under attack!"),
                                                                  PostCategory::Military, *attacked_goal));

            // Get a defender or walk into the building
            if(attacked_goal->CallDefender(*this))
                SwitchStateAttackingWaitingForDefender();
            else
            {
                state = SoldierState::AttackingCapturingFirst;
                StartWalking(Direction::NorthWest);
                // Regular military buildings will be captured
                if(attacked_goal->GetGOT() == GO_Type::NobMilitary)
                    static_cast<nobMilitary*>(attacked_goal)->PrepareCapturing();
            }
        }
    } else
    {
        // Destination wasn't the flag, so just wait until we can fight and face the flag
        state = SoldierState::AttackingWaitingAroundBuilding;
        Direction dir;
        if(pos.y == attFlagPos.y)
            dir = (pos.x < attFlagPos.x) ? Direction::East : Direction::West;
        else if(pos.x == attFlagPos.x)
        {
            if(pos.y < attFlagPos.y)
            {
                if(absDiff(pos.y, attFlagPos.y) & 1)
                    dir = (pos.y & 1) ? Direction::SouthWest : Direction::SouthEast;
                else
                    dir = Direction::SouthEast;
            } else
            {
                if(absDiff(pos.y, attFlagPos.y) & 1)
                    dir = (pos.y & 1) ? Direction::NorthWest : Direction::NorthEast;
                else
                    dir = Direction::NorthEast;
            }
        } else if(pos.y < attFlagPos.y)
            dir = (pos.x < attFlagPos.x) ? Direction::SouthEast : Direction::SouthWest;
        else // pos.y > attFlagPos.y
            dir = (pos.x < attFlagPos.x) ? Direction::NorthEast : Direction::NorthWest;
        FaceDir(dir);
        if(attacked_goal->GetPlayer() == player)
        {
            // Building already captured? -> Then we might be a far-away-capturer
            // -> Tell the building, that we are here
            RTTR_Assert(dynamic_cast<nobMilitary*>(attacked_goal));
            auto* goal = static_cast<nobMilitary*>(attacked_goal);
            if(goal->IsFarAwayCapturer(*this))
                goal->FarAwayCapturerReachedGoal(*this, false);
        }
    }
}

void nofAttacker::TryToOrderAggressiveDefender()
{
    RTTR_Assert(state == SoldierState::AttackingWalkingToGoal);
    if(mayBeHunted)
    {
        // 20% chance to get one
        if(RANDOM_RAND(10) < 2)
            OrderAggressiveDefender();
    }
}

void nofAttacker::OrderAggressiveDefender()
{
    // Any military building in at most this distance could send a defender
    constexpr auto maxDistance = 14;
    sortedMilitaryBlds buildings = world->LookForMilitaryBuildings(pos, 2);
    for(nobBaseMilitary* bld : buildings)
    {
        // Exclude HQs unless the one being attacked
        if(bld->GetBuildingType() == BuildingType::Headquarters && bld != attacked_goal)
            continue;
        if(world->CalcDistance(pos, bld->GetPos()) > maxDistance)
            continue;
        const unsigned bldOwnerId = bld->GetPlayer();
        if(canPlayerSendAggDefender[bldOwnerId] == SendDefender::No)
            continue;
        // Only allies of the attacked player send defenders and only if we can attack them
        GamePlayer& bldOwner = world->GetPlayer(bldOwnerId);
        if(!bldOwner.IsAlly(attacked_goal->GetPlayer()))
            continue;
        if(!bldOwner.IsAttackable(player))
            continue;
        // If player did not decide on sending do it now.
        // Doing this as late as here reduces chances,
        // that the player changed the setting when the defender is asked for
        if(canPlayerSendAggDefender[bldOwnerId] == SendDefender::Undecided)
        {
            if(bldOwner.ShouldSendDefender())
                canPlayerSendAggDefender[bldOwnerId] = SendDefender::Yes;
            else
            {
                canPlayerSendAggDefender[bldOwnerId] = SendDefender::No;
                continue;
            }
        }
        huntingDefender = bld->SendAggressiveDefender(*this);
        if(huntingDefender)
        {
            // Cannot be hunted again
            mayBeHunted = false;
            break;
        }
    }
}

void nofAttacker::AttackedGoalDestroyed()
{
    attacked_goal = nullptr;

    if(state == SoldierState::SeaattackingWaitInHarbor)
    {
        // We don't need to wait anymore, target was destroyed
        auto* harbor = world->GetSpecObj<nobHarborBuilding>(harborPos);
        RTTR_Assert(harbor);
        // go home
        goal_ = building;
        state = SoldierState::FigureWork;
        fs = FigureState::GotToGoal;
        harbor->CancelSeaAttacker(this);
    } else
    {
        const bool was_waiting_for_defender = (state == SoldierState::AttackingWaitingForDefender);

        // If waiting start moving
        if(state == SoldierState::AttackingWaitingForDefender || state == SoldierState::AttackingWaitingAroundBuilding
           || state == SoldierState::WaitingForFight)
            ReturnHomeMissionAttacking();
        if(was_waiting_for_defender)
        {
            GetEvMgr().RemoveEvent(blocking_event);
            world->RoadNodeAvailable(pos);
        }
    }
}

bool nofAttacker::AttackDefenderAtFlag()
{
    // Walk to flag if possible
    const auto dir = world->FindHumanPath(pos, attacked_goal->GetFlagPos(), 3, true);
    if(!dir)
        return false;

    const bool waiting_around_building = (state == SoldierState::AttackingWaitingAroundBuilding);
    state = SoldierState::AttackingAttackingFlag;

    if(waiting_around_building)
    {
        StartWalking(*dir);
        attacked_goal->SendSuccessor(pos, radius);
    }
    return true;
}

void nofAttacker::AttackFlag()
{
    state = SoldierState::AttackingWalkingToGoal;
    MissAttackingWalk();
}

void nofAttacker::CaptureBuilding()
{
    state = SoldierState::AttackingCapturingNext;
    CapturingWalking();
}

void nofAttacker::CapturingWalking()
{
    // If building is gone go home
    if(!attacked_goal)
    {
        ReturnHomeMissionAttacking();
        return;
    }
    RTTR_Assert(dynamic_cast<nobMilitary*>(attacked_goal));

    const MapPoint attFlagPos = attacked_goal->GetFlagPos();

    // 3 cases:
    // 1. We arrived in the building -> Capture
    // 2. We arrived at flag -> Go into the building
    // 3. Otherwise walk to the flag if our home building still exists, else wander around
    if(pos == attacked_goal->GetPos())
    {
        // We switch buildings
        if(building)
            building->SoldierLost(this);
        CancelAtHuntingDefender();
        if(ship_obj_id)
            CancelAtShip();
        building = attacked_goal;
        attacked_goal->AddActiveSoldier(world->RemoveFigure(pos, *this));

        // No longer attacking
        if(BuildingProperties::IsMilitary(attacked_goal->GetBuildingType()))
        {
            RTTR_Assert(dynamic_cast<nobMilitary*>(attacked_goal));
            auto* goal = static_cast<nobMilitary*>(attacked_goal);
            // If we are still a far-away-capturer at this point,
            // then the building belongs to us and capturing was already finished
            if(goal->IsFarAwayCapturer(*this))
            {
                RTTR_Assert(goal->GetPlayer() == player);
                RemoveFromAttackedGoal();
            } else
            {
                RemoveFromAttackedGoal();
                goal->CapturingSoldierArrived();
            }
        } else
            RemoveFromAttackedGoal();
    } else if(pos == attFlagPos)
    {
        // Go into the building and call next one for reinforcement
        StartWalking(Direction::NorthWest);
        RTTR_Assert(attacked_goal->GetPlayer() == player); // Assumed by the call below
        static_cast<nobMilitary*>(attacked_goal)->NeedOccupyingTroops();
    } else if(!building)
    {
        // If our home is destroyed we are lost and don't walk to the target (our new home if we were at least at the
        // flag already) Notify it, if it still exists (could be destroyed in the meantime too)
        if(attacked_goal)
        {
            auto* attackedBld = static_cast<nobMilitary*>(attacked_goal);
            RemoveFromAttackedGoal();
            // Reinforce building (should be already ours)
            RTTR_Assert(attackedBld->GetPlayer() == player);
            attackedBld->NeedOccupyingTroops();
        }

        if(ship_obj_id)
            CancelAtShip();

        state = SoldierState::FigureWork;
        StartWandering();
        Wander();
    } else
    {
        // Our home still exists so walk to the flag of the building if possible
        const auto dir = world->FindHumanPath(pos, attFlagPos, 10, true);
        if(dir)
            StartWalking(*dir);
        else
        {
            // Call other troops to occupy the building instead and go home
            RTTR_Assert(attacked_goal->GetPlayer() == player); // Assumed by the call below
            static_cast<nobMilitary*>(attacked_goal)->NeedOccupyingTroops();
            ReturnHomeMissionAttacking();
        }
    }
}

void nofAttacker::CapturedBuildingFull()
{
    // Reset goal, no longer our business
    attacked_goal = nullptr;

    // We only need to do something when we are currently waiting,
    // otherwise the next steps are handled when our current action (walk, fight) is finished
    if(state == SoldierState::AttackingWaitingAroundBuilding)
        ReturnHomeMissionAttacking();
}

void nofAttacker::StartSucceeding(const MapPoint /*pt*/, unsigned short /*new_radius*/)
{
    state = SoldierState::AttackingWalkingToGoal;

    const auto oldPos = pos;
    const auto oldRadius = radius;

    MissAttackingWalk();

    if(IsMoving())
        attacked_goal->SendSuccessor(oldPos, oldRadius);
}

void nofAttacker::LetsFight(nofAggressiveDefender& other)
{
    RTTR_Assert(!huntingDefender);
    // We only get hunted once
    mayBeHunted = false;
    huntingDefender = &other;
}

void nofAttacker::AggressiveDefenderLost()
{
    RTTR_Assert(huntingDefender);
    huntingDefender = nullptr;
}

void nofAttacker::SwitchStateAttackingWaitingForDefender()
{
    state = SoldierState::AttackingWaitingForDefender;
    blocking_event = GetEvMgr().AddEvent(this, BLOCK_OFFSET, 5);
}

void nofAttacker::HandleDerivedEvent(const unsigned /*id*/)
{
    RTTR_Assert(blocking_event);
    // If we are still waiting (we could be walking again if building was destroyed)
    // stop figures walking to this position
    if(state == SoldierState::AttackingWaitingForDefender)
    {
        world->StopOnRoads(pos);
        blocking_event = nullptr;
    }
}

bool nofAttacker::IsBlockingRoads() const
{
    if(state != SoldierState::AttackingWaitingForDefender)
        return false;

    // If the event isn't expired yet people can still move through
    return blocking_event == nullptr;
}

void nofAttacker::InformTargetsAboutCancelling()
{
    nofActiveSoldier::InformTargetsAboutCancelling();
    CancelAtHuntingDefender();

    if(attacked_goal)
        RemoveFromAttackedGoal();
    RTTR_Assert(attacked_goal == nullptr);
}

void nofAttacker::RemoveFromAttackedGoal()
{
    // If state == AttackingFightingVsDefender then we probably just lost the fight against the defender,
    // otherwise there must either be no defender or he is not waiting for us
    RTTR_Assert(
      state == SoldierState::AttackingFightingVsDefender || !attacked_goal->GetDefender()
      || (attacked_goal->GetDefender()->GetAttacker() != this && attacked_goal->GetDefender()->GetEnemy() != this));
    // No defender should be chasing us at this point
    for(auto* it : attacked_goal->GetAggresiveDefenders())
        RTTR_Assert(it->GetAttacker() != this);
    attacked_goal->UnlinkAggressor(*this);
    attacked_goal = nullptr;
}

void nofAttacker::StartAttackOnOtherIsland(const MapPoint shipPos, const unsigned ship_id)
{
    pos = this->shipPos = shipPos;
    this->ship_obj_id = ship_id;

    state = SoldierState::AttackingWalkingToGoal;
    on_ship = false;
    MissAttackingWalk();
}

void nofAttacker::SeaAttackFailedBeforeLaunch()
{
    InformTargetsAboutCancelling();
    RTTR_Assert(!huntingDefender);
    AbrogateWorkplace();
    goal_ = nullptr;
    state = SoldierState::FigureWork;
}

void nofAttacker::StartReturnViaShip(noShip& ship)
{
    if(pos.isValid())
    {
        ship.AddReturnedAttacker(world->RemoveFigure(pos, *this));
        pos = MapPoint::Invalid(); // Similar to start ship journey
    } else
    {
        // If pos is not valid, then we are still on the ship!
        // This can happen, if the ship cannot reach its target
        RTTR_Assert(state == SoldierState::SeaattackingOnShip);
        RTTR_Assert(ship.IsOnBoard(*this));
        InformTargetsAboutCancelling();
    }

    goal_ = building;
    state = SoldierState::FigureWork;
    fs = FigureState::GotToGoal;
    on_ship = true;
    ship_obj_id = 0;
}

void nofAttacker::HomeHarborLost()
{
    // this in combination with telling the home building that the soldier is lost should work just fine
    goal_ = nullptr;
}

void nofAttacker::CancelAtShip()
{
    for(noBase& figure : world->GetFigures(shipPos))
    {
        if(figure.GetObjId() == ship_obj_id)
        {
            static_cast<noShip&>(figure).SeaAttackerWishesNoReturn();
            break;
        }
    }
    ship_obj_id = 0;
}

void nofAttacker::CancelAtHuntingDefender()
{
    if(huntingDefender)
    {
        RTTR_Assert(huntingDefender->GetAttacker() == this);
        huntingDefender->AttackerLost();
        huntingDefender = nullptr;
    }
}

void nofAttacker::HandleState_SeaAttack_ReturnToShip()
{
    if(!building)
    {
        // Home destroyed -> start wandering
        state = SoldierState::FigureWork;
        StartWandering();
        Wander();

        CancelAtShip();
    } else if(pos == shipPos) // Arrived at ship
    {
        for(noBase& figure : world->GetFigures(pos))
        {
            if(figure.GetObjId() == ship_obj_id)
            {
                StartReturnViaShip(static_cast<noShip&>(figure));
                return;
            }
        }

        // Ship is gone, shouldn't happen!
        RTTR_Assert(false);
        ship_obj_id = 0;
        StartWandering();
        state = SoldierState::FigureWork;
        Wander();
    } else
    {
        const auto dir = world->FindHumanPath(pos, shipPos, MAX_ATTACKING_RUN_DISTANCE);
        if(dir)
            StartWalking(*dir);
        else
        {
            // No path -> wander around
            StartWandering();
            state = SoldierState::FigureWork;
            Wander();

            // Notify home and ship
            building->SoldierLost(this);
            CancelAtShip();
        }
    }
}

void nofAttacker::CancelSeaAttack()
{
    InformTargetsAboutCancelling();
    RTTR_Assert(!huntingDefender);
    Abrogate();
}

void nofAttacker::FreeFightEnded()
{
    nofActiveSoldier::FreeFightEnded();
    // Continue with normal walking towards our goal
    state = SoldierState::AttackingWalkingToGoal;
}

bool nofAttacker::CanStartFarAwayCapturing(const nobMilitary& dest) const
{
    // Are we already walking to the destination?
    if(state == SoldierState::AttackingWalkingToGoal || state == SoldierState::MeetEnemy
       || state == SoldierState::WaitingForFight || state == SoldierState::Fighting)
    {
        // Not too far away?
        if(world->CalcDistance(pos, dest.GetPos()) < MAX_FAR_AWAY_CAPTURING_DISTANCE)
            return true;
    }

    return false;
}
