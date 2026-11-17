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
      canPlayerSendAggDefender(world->getNumPlayers(), SendDefender::Undecided), huntingDefender(nullptr), radius(0),
      blocking_event(nullptr), harborPos(harbor ? harbor->getPos() : MapPoint::invalid()), shipPos(MapPoint::invalid()),
      ship_obj_id(0)
{
    attacked_goal.linkAggressor(*this);
}

nofAttacker::~nofAttacker() = default;

void nofAttacker::destroy()
{
    RTTR_Assert(!attacked_goal);
    RTTR_Assert(!ship_obj_id);
    RTTR_Assert(!huntingDefender);
    nofActiveSoldier::destroy();

    /*unsigned char oplayer = (player == 0) ? 1 : 0;
    RTTR_Assert(!world->GetPlayer(oplayer).GetFirstWH()->Test(this));*/
}

void nofAttacker::serialize(SerializedGameData& sgd) const
{
    nofActiveSoldier::serialize(sgd);

    if(state != SoldierState::WalkingHome && state != SoldierState::FigureWork)
    {
        sgd.pushObject(attacked_goal);
        sgd.PushBool(mayBeHunted);
        helpers::pushContainer(sgd, canPlayerSendAggDefender);
        sgd.pushObject(huntingDefender, true);
        sgd.PushUnsignedShort(radius);

        if(state == SoldierState::AttackingWaitingForDefender)
            sgd.pushEvent(blocking_event);

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
        attacked_goal = sgd.popObject<nobBaseMilitary>();
        mayBeHunted = sgd.PopBool();
        sgd.popContainer(canPlayerSendAggDefender);
        RTTR_Assert(canPlayerSendAggDefender.size() == world->getNumPlayers());
        huntingDefender = sgd.popObject<nofAggressiveDefender>(GO_Type::NofAggressivedefender);

        radius = sgd.PopUnsignedShort();

        if(state == SoldierState::AttackingWaitingForDefender)
            blocking_event = sgd.popEvent();
        else
            blocking_event = nullptr;

        harborPos = sgd.popMapPoint();
        shipPos = sgd.popMapPoint();
        ship_obj_id = sgd.PopUnsignedInt();
    } else
    {
        attacked_goal = nullptr;
        mayBeHunted = false;
        canPlayerSendAggDefender.resize(world->getNumPlayers(), SendDefender::Undecided);
        huntingDefender = nullptr;
        radius = 0;
        blocking_event = nullptr;
        harborPos = MapPoint::invalid();
        shipPos = MapPoint::invalid(); //-V656
        ship_obj_id = 0;
    }
}

void nofAttacker::walked()
{
    expelEnemies();

    switch(state)
    {
        default: nofActiveSoldier::walked(); break;
        case SoldierState::AttackingWalkingToGoal: missAttackingWalk(); break;
        case SoldierState::AttackingAttackingFlag:
        {
            // If target is destroyed go home
            if(!attacked_goal)
            {
                returnHomeMissionAttacking();
                return;
            }

            const MapPoint goalFlagPos = attacked_goal->getFlagPos();

            nofDefender* defender = nullptr;
            // Look for defenders at this position
            for(auto& figure : world->getFigures(goalFlagPos))
            {
                if(figure.getGOT() == GO_Type::NofDefender)
                {
                    // Is the defender waiting at the flag?
                    // (could be wandering around or something)
                    if(static_cast<nofDefender&>(figure).isWaitingAtFlag())
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
                    world->addFigure(pos, std::make_unique<noFighting>(*this, *defender));
                    state = SoldierState::AttackingFightingVsDefender;
                    defender->fightStarted();
                } else
                {
                    // No defender at the flag?
                    // -> Order new defenders or capture the building
                    continueAtFlag();
                }
            } else
            {
                const auto dir = world->findHumanPath(pos, goalFlagPos, 5, true);
                if(dir)
                    startWalking(*dir);
                else
                {
                    // If there is no path anymore walk in the direction of the goal (or go back home)
                    state = SoldierState::AttackingWalkingToGoal;
                    missAttackingWalk();
                    // Notify defender if any
                    if(defender)
                        defender->attackerArrested();
                }
            }
        }
        break;
        case SoldierState::AttackingCapturingFirst:
        {
            if(!attacked_goal)
            {
                // Goal destroyed -> Go gome
                returnHomeMissionAttacking();
                return;
            }

            // If there is a defender available then an enemy soldier went back in
            if(attacked_goal->defendersAvailable())
            {
                // Go back out
                if(attacked_goal->getGOT() == GO_Type::NobMilitary)
                    static_cast<nobMilitary*>(attacked_goal)->stopCapturing();

                state = SoldierState::AttackingWalkingToGoal;
                startWalking(Direction::SouthEast);
            } else
            {
                // Did we just conquer a "regular" military building?
                if(BuildingProperties::isMilitary(attacked_goal->getBuildingType()))
                {
                    RTTR_Assert(dynamic_cast<nobMilitary*>(attacked_goal));
                    // We will now have this building as the new home, so inform old home, hunting soldier and ship
                    if(homeBld)
                        homeBld->soldierLost(this);
                    cancelAtHuntingDefender();
                    if(ship_obj_id)
                        cancelAtShip();
                    // Store the goal in a temporary as attacked_goal will be reset
                    auto* goal = static_cast<nobMilitary*>(attacked_goal);
                    goal->capture(player);
                    homeBld = attacked_goal;
                    attacked_goal->addActiveSoldier(world->removeFigure(pos, *this));
                    removeFromAttackedGoal();
                    // This might call other capturers
                    goal->capturingSoldierArrived();
                } else // Something like HQ or harbor
                {
                    // Inform the owner of the building and destroy it
                    const std::string msg = (attacked_goal->getGOT() == GO_Type::NobHq) ?
                                              _("Our headquarters was destroyed!") :
                                              _("This harbor building was destroyed");
                    sendPostMessage(attacked_goal->getPlayer(),
                                    std::make_unique<PostMsgWithBuilding>(getEvMgr().getCurrentGF(), msg,
                                                                          PostCategory::Military, *attacked_goal));

                    nobBaseMilitary* tmp_goal = attacked_goal; // Store in a temporary in case it gets reset by Destroy
                    tmp_goal->destroy();
                    delete tmp_goal;
                    attacked_goal = nullptr;
                    returnHomeMissionAttacking();
                }
            }
        }
        break;
        case SoldierState::AttackingCapturingNext: capturingWalking(); break;

        case SoldierState::SeaattackingGoToHarbor:
        {
            // Abort if the harbor doesn't exist, belongs to another player or the attacked building is gone
            const noBase* hb = world->getNO(harborPos);
            const bool valid_harbor = hb->getGOT() == GO_Type::NobHarborbuilding
                                      && static_cast<const nobHarborBuilding*>(hb)->getPlayer() == player;

            if(!valid_harbor || !attacked_goal)
            {
                returnHomeMissionAttacking();
                return;
            }

            if(pos == harborPos) // When we arrived add to harbor
            {
                state = SoldierState::SeaattackingWaitInHarbor;
                world->getSpecObj<nobHarborBuilding>(pos)->addSeaAttacker(world->removeFigure(pos, *this));
            } else // Go to flag then walk inside the building
            {
                const MapPoint harborFlagPos = world->getNeighbour(harborPos, Direction::SouthEast);

                if(pos == harborFlagPos)
                    startWalking(Direction::NorthWest);
                else
                {
                    const auto dir =
                      world->findHumanPath(pos, harborFlagPos, MAX_ATTACKING_RUN_DISTANCE, false, nullptr);
                    if(dir)
                        startWalking(*dir);
                    else
                        returnHomeMissionAttacking(); // No possible path -> Abort
                }
            }
        }
        break;
        case SoldierState::SeaattackingWaitInHarbor: break;
        case SoldierState::SeaattackingOnShip:
            // Can't walk while on the ship
            RTTR_Assert(false);
            break;
        case SoldierState::SeaattackingReturnToShip: handleStateSeaAttackReturnToShip(); break;
    }
}

void nofAttacker::homeDestroyed()
{
    // If we are waiting, no events will trigger -> Start wandering right now,
    // else continue current event (e.g. movement) and handle at the end of that
    if(state == SoldierState::AttackingWaitingAroundBuilding)
    {
        // We are lost now so no valid source or target of attacks
        nobBaseMilitary* curGoal = attacked_goal; // attacked_goal gets reset
        informTargetsAboutCancelling();
        if(ship_obj_id)
            cancelAtShip();

        homeBld = nullptr;
        state = SoldierState::FigureWork;
        startWandering();
        wander();

        // Someone else might take this place
        curGoal->sendSuccessor(pos, radius);
    } else
    {
        // If we were going back home, reset that goal
        if(goal_ == homeBld)
            goal_ = nullptr;
        homeBld = nullptr;
    }
}

void nofAttacker::homeDestroyedAtBegin()
{
    homeBld = nullptr;

    // We are lost now and hence not targetable
    informTargetsAboutCancelling();

    state = SoldierState::FigureWork;

    startWandering();
    startWalking(RANDOM_ENUM(Direction));
}

void nofAttacker::wonFighting()
{
    if(world->getGGS().isEnabled(AddonId::BATTLEFIELD_PROMOTION))
        increaseRank();
    // If our home was destroyed then we are lost
    // unless we are currently fighting at the flag so that building can become our new home
    if(!homeBld && state != SoldierState::AttackingFightingVsDefender)
    {
        // Lost -> Tell all dependents
        informTargetsAboutCancelling();
        if(ship_obj_id)
            cancelAtShip();

        state = SoldierState::FigureWork;
        startWandering();
        wander();
    } else if(!attacked_goal)
    {
        // Target is gone -> Go home
        returnHomeMissionAttacking();
    } else
        continueAtFlag();
}

void nofAttacker::continueAtFlag()
{
    RTTR_Assert(attacked_goal);
    // If we were fighting at the flag of the goal then call a new defender
    // or capture the building if there isn't one anymore
    if(state == SoldierState::AttackingFightingVsDefender
       || (state == SoldierState::Fighting && attacked_goal->getFlagPos() == pos))
    {
        if(attacked_goal->callDefender(*this)) //-V522
            switchStateAttackingWaitingForDefender();
        else if(!tryFightingNearbyEnemy(attacked_goal->getPlayer()))
        {
            // No defender or soldiers of other non-friendly non-owner players to fight
            // -> Enter building
            state = SoldierState::AttackingCapturingFirst;
            startWalking(Direction::NorthWest);

            // Regular military buildings will be captured
            if(attacked_goal->getGOT() == GO_Type::NobMilitary)
                static_cast<nobMilitary*>(attacked_goal)->prepareCapturing();
        }
    } else
    {
        // Go (back) to goal
        state = SoldierState::AttackingWalkingToGoal;
        missAttackingWalk();
    }
}

void nofAttacker::lostFighting()
{
    // Inform home building (so e.g. new troops can be ordered) and all dependents
    abrogateWorkplace();
    informTargetsAboutCancelling();
    if(ship_obj_id)
        this->cancelAtShip();
}

void nofAttacker::returnHomeMissionAttacking()
{
    informTargetsAboutCancelling(); // We don't attack anymore
    if(ship_obj_id)
    {
        state = SoldierState::SeaattackingReturnToShip;
        handleStateSeaAttackReturnToShip();
    } else
        returnHome();
}

void nofAttacker::missAttackingWalk()
{
    // If our home is destroyed we are lost
    if(!homeBld)
    {
        informTargetsAboutCancelling();
        if(ship_obj_id)
            cancelAtShip();

        state = SoldierState::FigureWork;
        startWandering();
        wander();

        return;
    }

    // When target is gone go home
    if(!attacked_goal)
    {
        returnHomeMissionAttacking();
        return;
    }

    // Find a position next to the target
    const MapPoint goal = attacked_goal->findAnAttackerPlace(radius, *this);

    if(!goal.isValid()) // Go home if there is no free spot
        returnHomeMissionAttacking();
    else if(pos == goal)
        reachedDestination();
    else if(!tryFightingNearbyEnemy())
    {
        // If no Enemy (attackers, aggressive defenders..) was found (would be handled by nofActiveSoldier)
        // check if some building might send a defender and walk to goal

        tryToOrderAggressiveDefender();

        const auto dir = world->findHumanPath(pos, goal, MAX_ATTACKING_RUN_DISTANCE, true);
        if(dir)
            startWalking(*dir);
        else
            returnHomeMissionAttacking();
    }
}

void nofAttacker::reachedDestination()
{
    // Are we at the flag of the target?
    const MapPoint attFlagPos = attacked_goal->getFlagPos();
    if(pos == attFlagPos)
    {
        // If building is already captured continue capturing
        // (This can only be a far away attacker)
        if(attacked_goal->getPlayer() == player)
        {
            state = SoldierState::AttackingCapturingNext;
            RTTR_Assert(dynamic_cast<nobMilitary*>(attacked_goal));
            auto* goal = static_cast<nobMilitary*>(attacked_goal);
            RTTR_Assert(goal->isFarAwayCapturer(*this));
            // Start walking first so the flag is free
            startWalking(Direction::NorthWest);
            // Then tell the building
            goal->farAwayCapturerReachedGoal(*this, true);
        } else
        {
            // Notify player
            // TODO: Possibly improve that not every attacker does that
            sendPostMessage(attacked_goal->getPlayer(),
                            std::make_unique<PostMsgWithBuilding>(getEvMgr().getCurrentGF(), _("We are under attack!"),
                                                                  PostCategory::Military, *attacked_goal));

            // Get a defender or walk into the building
            if(attacked_goal->callDefender(*this))
                switchStateAttackingWaitingForDefender();
            else
            {
                state = SoldierState::AttackingCapturingFirst;
                startWalking(Direction::NorthWest);
                // Regular military buildings will be captured
                if(attacked_goal->getGOT() == GO_Type::NobMilitary)
                    static_cast<nobMilitary*>(attacked_goal)->prepareCapturing();
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
        faceDir(dir);
        if(attacked_goal->getPlayer() == player)
        {
            // Building already captured? -> Then we might be a far-away-capturer
            // -> Tell the building, that we are here
            RTTR_Assert(dynamic_cast<nobMilitary*>(attacked_goal));
            auto* goal = static_cast<nobMilitary*>(attacked_goal);
            if(goal->isFarAwayCapturer(*this))
                goal->farAwayCapturerReachedGoal(*this, false);
        }
    }
}

void nofAttacker::tryToOrderAggressiveDefender()
{
    RTTR_Assert(state == SoldierState::AttackingWalkingToGoal);
    if(mayBeHunted)
    {
        // 20% chance to get one
        if(RANDOM_RAND(10) < 2)
            orderAggressiveDefender();
    }
}

void nofAttacker::orderAggressiveDefender()
{
    // Any military building in at most this distance could send a defender
    constexpr auto maxDistance = 14;
    sortedMilitaryBlds buildings = world->lookForMilitaryBuildings(pos, 2);
    for(nobBaseMilitary* bld : buildings)
    {
        // Exclude HQs unless the one being attacked
        if(bld->getBuildingType() == BuildingType::Headquarters && bld != attacked_goal)
            continue;
        if(world->calcDistance(pos, bld->getPos()) > maxDistance)
            continue;
        const unsigned bldOwnerId = bld->getPlayer();
        if(canPlayerSendAggDefender[bldOwnerId] == SendDefender::No)
            continue;
        // Only allies of the attacked player send defenders and only if we can attack them
        GamePlayer& bldOwner = world->getPlayer(bldOwnerId);
        if(!bldOwner.isAlly(attacked_goal->getPlayer()))
            continue;
        if(!bldOwner.isAttackable(player))
            continue;
        // If player did not decide on sending do it now.
        // Doing this as late as here reduces chances,
        // that the player changed the setting when the defender is asked for
        if(canPlayerSendAggDefender[bldOwnerId] == SendDefender::Undecided)
        {
            if(bldOwner.shouldSendDefender())
                canPlayerSendAggDefender[bldOwnerId] = SendDefender::Yes;
            else
            {
                canPlayerSendAggDefender[bldOwnerId] = SendDefender::No;
                continue;
            }
        }
        huntingDefender = bld->sendAggressiveDefender(*this);
        if(huntingDefender)
        {
            // Cannot be hunted again
            mayBeHunted = false;
            break;
        }
    }
}

void nofAttacker::attackedGoalDestroyed()
{
    attacked_goal = nullptr;

    if(state == SoldierState::SeaattackingWaitInHarbor)
    {
        // We don't need to wait anymore, target was destroyed
        auto* harbor = world->getSpecObj<nobHarborBuilding>(harborPos);
        RTTR_Assert(harbor);
        // go home
        goal_ = homeBld;
        state = SoldierState::FigureWork;
        fs = FigureState::GotToGoal;
        harbor->cancelSeaAttacker(this);
    } else
    {
        const bool was_waiting_for_defender = (state == SoldierState::AttackingWaitingForDefender);

        // If waiting start moving
        if(state == SoldierState::AttackingWaitingForDefender || state == SoldierState::AttackingWaitingAroundBuilding
           || state == SoldierState::WaitingForFight)
            returnHomeMissionAttacking();
        if(was_waiting_for_defender)
        {
            getEvMgr().removeEvent(blocking_event);
            world->roadNodeAvailable(pos);
        }
    }
}

bool nofAttacker::attackDefenderAtFlag()
{
    // Walk to flag if possible
    const auto dir = world->findHumanPath(pos, attacked_goal->getFlagPos(), 3, true);
    if(dir)
    {
        const bool waiting_around_building = (state == SoldierState::AttackingWaitingAroundBuilding);
        state = SoldierState::AttackingAttackingFlag;

        if(waiting_around_building)
        {
            startWalking(*dir);
            attacked_goal->sendSuccessor(pos, radius);
        }
        return true;
    } else
    {
        state = nofActiveSoldier::SoldierState::AttackingWalkingToGoal;
        missAttackingWalk();
        return false;
    }
}

void nofAttacker::attackFlag()
{
    state = SoldierState::AttackingWalkingToGoal;
    missAttackingWalk();
}

void nofAttacker::captureBuilding()
{
    state = SoldierState::AttackingCapturingNext;
    capturingWalking();
}

void nofAttacker::capturingWalking()
{
    // If building is gone go home
    if(!attacked_goal)
    {
        returnHomeMissionAttacking();
        return;
    }
    RTTR_Assert(dynamic_cast<nobMilitary*>(attacked_goal));

    const MapPoint attFlagPos = attacked_goal->getFlagPos();

    // 3 cases:
    // 1. We arrived in the building -> Capture
    // 2. We arrived at flag -> Go into the building
    // 3. Otherwise walk to the flag if our home building still exists, else wander around
    if(pos == attacked_goal->getPos())
    {
        // We switch buildings
        if(homeBld)
            homeBld->soldierLost(this);
        cancelAtHuntingDefender();
        if(ship_obj_id)
            cancelAtShip();
        homeBld = attacked_goal;
        attacked_goal->addActiveSoldier(world->removeFigure(pos, *this));

        // No longer attacking
        if(BuildingProperties::isMilitary(attacked_goal->getBuildingType()))
        {
            RTTR_Assert(dynamic_cast<nobMilitary*>(attacked_goal));
            auto* goal = static_cast<nobMilitary*>(attacked_goal);
            // If we are still a far-away-capturer at this point,
            // then the building belongs to us and capturing was already finished
            if(goal->isFarAwayCapturer(*this))
            {
                RTTR_Assert(goal->getPlayer() == player);
                removeFromAttackedGoal();
            } else
            {
                removeFromAttackedGoal();
                goal->capturingSoldierArrived();
            }
        } else
            removeFromAttackedGoal();
    } else if(pos == attFlagPos)
    {
        // Go into the building and call next one for reinforcement
        startWalking(Direction::NorthWest);
        RTTR_Assert(attacked_goal->getPlayer() == player); // Assumed by the call below
        static_cast<nobMilitary*>(attacked_goal)->needOccupyingTroops();
    } else if(!homeBld)
    {
        // If our home is destroyed we are lost and don't walk to the target (our new home if we were at least at the
        // flag already) Notify it, if it still exists (could be destroyed in the meantime too)
        if(attacked_goal)
        {
            auto* attackedBld = static_cast<nobMilitary*>(attacked_goal);
            removeFromAttackedGoal();
            // Reinforce building (should be already ours)
            RTTR_Assert(attackedBld->getPlayer() == player);
            attackedBld->needOccupyingTroops();
        }

        if(ship_obj_id)
            cancelAtShip();

        state = SoldierState::FigureWork;
        startWandering();
        wander();
    } else
    {
        // Our home still exists so walk to the flag of the building if possible
        const auto dir = world->findHumanPath(pos, attFlagPos, 10, true);
        if(dir)
            startWalking(*dir);
        else
        {
            // Call other troops to occupy the building instead and go home
            RTTR_Assert(attacked_goal->getPlayer() == player); // Assumed by the call below
            static_cast<nobMilitary*>(attacked_goal)->needOccupyingTroops();
            returnHomeMissionAttacking();
        }
    }
}

void nofAttacker::capturedBuildingFull()
{
    // Reset goal, no longer our business
    attacked_goal = nullptr;

    // We only need to do something when we are currently waiting,
    // otherwise the next steps are handled when our current action (walk, fight) is finished
    if(state == SoldierState::AttackingWaitingAroundBuilding)
        returnHomeMissionAttacking();
}

void nofAttacker::startSucceeding(const MapPoint /*pt*/, unsigned short /*new_radius*/)
{
    state = SoldierState::AttackingWalkingToGoal;

    const auto oldPos = pos;
    const auto oldRadius = radius;

    missAttackingWalk();

    if(isMoving())
        attacked_goal->sendSuccessor(oldPos, oldRadius);
}

void nofAttacker::letsFight(nofAggressiveDefender& other)
{
    RTTR_Assert(!huntingDefender);
    // We only get hunted once
    mayBeHunted = false;
    huntingDefender = &other;
}

void nofAttacker::aggressiveDefenderLost()
{
    RTTR_Assert(huntingDefender);
    huntingDefender = nullptr;
}

void nofAttacker::switchStateAttackingWaitingForDefender()
{
    state = SoldierState::AttackingWaitingForDefender;
    blocking_event = getEvMgr().addEvent(this, BLOCK_OFFSET, 5);
}

void nofAttacker::handleDerivedEvent(const unsigned /*id*/)
{
    RTTR_Assert(blocking_event);
    // If we are still waiting (we could be walking again if building was destroyed)
    // stop figures walking to this position
    if(state == SoldierState::AttackingWaitingForDefender)
    {
        world->stopOnRoads(pos);
        blocking_event = nullptr;
    }
}

bool nofAttacker::isBlockingRoads() const
{
    if(state != SoldierState::AttackingWaitingForDefender)
        return false;

    // If the event isn't expired yet people can still move through
    return blocking_event == nullptr;
}

void nofAttacker::informTargetsAboutCancelling()
{
    nofActiveSoldier::informTargetsAboutCancelling();
    cancelAtHuntingDefender();

    if(attacked_goal)
        removeFromAttackedGoal();
    RTTR_Assert(attacked_goal == nullptr);
}

void nofAttacker::removeFromAttackedGoal()
{
    // If state == AttackingFightingVsDefender then we probably just lost the fight against the defender,
    // otherwise there must either be no defender or he is not waiting for us
    RTTR_Assert(
      state == SoldierState::AttackingFightingVsDefender || !attacked_goal->getDefender()
      || (attacked_goal->getDefender()->getAttacker() != this && attacked_goal->getDefender()->getEnemy() != this));
    // No defender should be chasing us at this point
    for(auto* it : attacked_goal->getAggresiveDefenders())
        RTTR_Assert(it->getAttacker() != this);
    attacked_goal->unlinkAggressor(*this);
    attacked_goal = nullptr;
}

void nofAttacker::startAttackOnOtherIsland(const MapPoint shipPos, const unsigned ship_id)
{
    pos = this->shipPos = shipPos;
    this->ship_obj_id = ship_id;

    state = SoldierState::AttackingWalkingToGoal;
    on_ship = false;
    missAttackingWalk();
}

void nofAttacker::seaAttackFailedBeforeLaunch()
{
    informTargetsAboutCancelling();
    RTTR_Assert(!huntingDefender);
    abrogateWorkplace();
    goal_ = nullptr;
    state = SoldierState::FigureWork;
}

void nofAttacker::startReturnViaShip(noShip& ship)
{
    if(pos.isValid())
    {
        ship.addReturnedAttacker(world->removeFigure(pos, *this));
        pos = MapPoint::invalid(); // Similar to start ship journey
    } else
    {
        // If pos is not valid, then we are still on the ship!
        // This can happen, if the ship cannot reach its target
        RTTR_Assert(state == SoldierState::SeaattackingOnShip);
        RTTR_Assert(ship.isOnBoard(*this));
        informTargetsAboutCancelling();
    }

    goal_ = homeBld;
    state = SoldierState::FigureWork;
    fs = FigureState::GotToGoal;
    on_ship = true;
    ship_obj_id = 0;
}

void nofAttacker::homeHarborLost()
{
    // this in combination with telling the home building that the soldier is lost should work just fine
    goal_ = nullptr;
}

void nofAttacker::cancelAtShip()
{
    for(noBase& figure : world->getFigures(shipPos))
    {
        if(figure.getObjId() == ship_obj_id)
        {
            static_cast<noShip&>(figure).seaAttackerWishesNoReturn();
            break;
        }
    }
    ship_obj_id = 0;
}

void nofAttacker::cancelAtHuntingDefender()
{
    if(huntingDefender)
    {
        RTTR_Assert(huntingDefender->getAttacker() == this);
        huntingDefender->attackerLost();
        huntingDefender = nullptr;
    }
}

void nofAttacker::handleStateSeaAttackReturnToShip()
{
    if(!homeBld)
    {
        // Home destroyed -> start wandering
        state = SoldierState::FigureWork;
        startWandering();
        wander();

        cancelAtShip();
    } else if(pos == shipPos) // Arrived at ship
    {
        for(noBase& figure : world->getFigures(pos))
        {
            if(figure.getObjId() == ship_obj_id)
            {
                startReturnViaShip(static_cast<noShip&>(figure));
                return;
            }
        }

        // Ship is gone, shouldn't happen!
        RTTR_Assert(false);
        ship_obj_id = 0;
        startWandering();
        state = SoldierState::FigureWork;
        wander();
    } else
    {
        const auto dir = world->findHumanPath(pos, shipPos, MAX_ATTACKING_RUN_DISTANCE);
        if(dir)
            startWalking(*dir);
        else
        {
            // No path -> wander around
            startWandering();
            state = SoldierState::FigureWork;
            wander();

            // Notify home and ship
            homeBld->soldierLost(this);
            cancelAtShip();
        }
    }
}

void nofAttacker::cancelSeaAttack()
{
    informTargetsAboutCancelling();
    RTTR_Assert(!huntingDefender);
    abrogate();
}

nofActiveSoldier::SoldierState nofAttacker::freeFightAborted()
{
    // Continue with normal walking towards our goal
    return SoldierState::AttackingWalkingToGoal;
}

bool nofAttacker::canStartFarAwayCapturing(const nobMilitary& dest) const
{
    // Are we already walking to the destination?
    if(state == SoldierState::AttackingWalkingToGoal || state == SoldierState::MeetEnemy
       || state == SoldierState::WaitingForFight || state == SoldierState::Fighting)
    {
        // Not too far away?
        if(world->calcDistance(pos, dest.getPos()) < MAX_FAR_AWAY_CAPTURING_DISTANCE)
            return true;
    }

    return false;
}
