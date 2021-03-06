// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

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

/// Nach einer bestimmten Zeit, in der der Angreifer an der Flagge des Gebäudes steht, blockt er den Weg
/// nur benutzt bei AttackingWaitingfordefender
/// Dieses Konstante gibt an, wie lange, nachdem er anfängt da zu stehen, er blockt
const unsigned BLOCK_OFFSET = 10;

nofAttacker::nofAttacker(const nofPassiveSoldier& other, nobBaseMilitary& attacked_goal,
                         const nobHarborBuilding* const harbor)
    : nofActiveSoldier(other, harbor ? SoldierState::SeaattackingGoToHarbor : SoldierState::AttackingWalkingToGoal),
      attacked_goal(&attacked_goal), mayBeHunted(true), canPlayerSendAggDefender(world->GetNumPlayers(), 2),
      huntingDefender(nullptr), blocking_event(nullptr), harborPos(harbor ? harbor->GetPos() : MapPoint::Invalid()),
      shipPos(MapPoint::Invalid()), ship_obj_id(0)
{
    // Dem Ziel Bescheid sagen
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
        canPlayerSendAggDefender.resize(world->GetNumPlayers(), 2);
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

    // Was bestimmtes machen, je nachdem welchen Status wir gerade haben
    switch(state)
    {
        default: nofActiveSoldier::Walked(); break;
        case SoldierState::AttackingWalkingToGoal:
        {
            MissAttackingWalk();
        }
        break;
        case SoldierState::AttackingAttackingFlag:
        {
            // Ist evtl. das Zielgebäude zerstört?
            if(!attacked_goal)
            {
                // Nach Hause gehen
                ReturnHomeMissionAttacking();
                return;
            }

            MapPoint goalFlagPos = attacked_goal->GetFlagPos();
            // RTTR_Assert(enemy->GetGOT() == GOT_NOF_DEFENDER);
            // Are we at the flag?

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

            if(pos == goalFlagPos)
            {
                if(defender)
                {
                    // Start fight with the defender
                    world->AddFigure(pos, std::make_unique<noFighting>(*this, *defender));

                    // Set the appropriate states
                    state = SoldierState::AttackingFightingVsDefender;
                    defender->FightStarted();
                } else
                    // No defender at the flag?
                    // -> Order new defenders or capture the building
                    ContinueAtFlag();
            } else
            {
                const auto dir = world->FindHumanPath(pos, goalFlagPos, 5, true);
                if(dir)
                    StartWalking(*dir);
                else
                {
                    // es wurde kein Weg mehr gefunden --> neues Plätzchen suchen und warten
                    state = SoldierState::AttackingWalkingToGoal;
                    MissAttackingWalk();
                    // der Verteidiger muss darüber informiert werden
                    if(defender)
                        defender->AttackerArrested();
                }
            }
        }
        break;
        case SoldierState::AttackingCapturingFirst:
        {
            // Ist evtl. das Zielgebäude zerstört?
            if(!attacked_goal)
            {
                // Nach Hause gehen
                ReturnHomeMissionAttacking();
                return;
            }

            // Wenn schon welche drin sind, ist wieder ein feindlicher reingegangen
            if(attacked_goal->DefendersAvailable())
            {
                // Wieder rausgehen, Platz reservieren
                if(attacked_goal->GetGOT() == GO_Type::NobMilitary)
                    static_cast<nobMilitary*>(attacked_goal)->StopCapturing();

                state = SoldierState::AttackingWalkingToGoal;
                StartWalking(Direction::SouthEast);
                return;
            } else
            {
                // Ist das Gebäude ein "normales Militärgebäude", das wir da erobert haben?
                if(BuildingProperties::IsMilitary(attacked_goal->GetBuildingType()))
                {
                    RTTR_Assert(dynamic_cast<nobMilitary*>(attacked_goal));
                    // Meinem Heimatgebäude Bescheid sagen, dass ich nicht mehr komme (falls es noch eins gibt)
                    if(building)
                        building->SoldierLost(this);
                    CancelAtHuntingDefender();
                    // Ggf. Schiff Bescheid sagen (Schiffs-Angreifer)
                    if(ship_obj_id)
                        CancelAtShip();
                    // Gebäude einnehmen
                    auto* goal = static_cast<nobMilitary*>(attacked_goal);
                    goal->Capture(player);
                    // This is the new home
                    building = attacked_goal;
                    // mich zum Gebäude hinzufügen und von der Karte entfernen
                    attacked_goal->AddActiveSoldier(world->RemoveFigure(pos, *this));
                    RemoveFromAttackedGoal();
                    // Tell that we arrived and probably call other capturers
                    goal->CapturingSoldierArrived();
                }
                // oder ein Hauptquartier oder Hafen?
                else
                {
                    // Inform the owner of the building
                    const std::string msg = (attacked_goal->GetGOT() == GO_Type::NobHq) ?
                                              _("Our headquarters was destroyed!") :
                                              _("This harbor building was destroyed");
                    SendPostMessage(attacked_goal->GetPlayer(),
                                    std::make_unique<PostMsgWithBuilding>(GetEvMgr().GetCurrentGF(), msg,
                                                                          PostCategory::Military, *attacked_goal));

                    // abreißen
                    nobBaseMilitary* tmp_goal = attacked_goal; // attacked_goal wird evtl auf 0 gesetzt!
                    tmp_goal->Destroy();
                    delete tmp_goal;
                    attacked_goal = nullptr;
                    ReturnHomeMissionAttacking();
                }
            }
        }
        break;
        case SoldierState::AttackingCapturingNext:
        {
            CapturingWalking();
        }
        break;

        case SoldierState::SeaattackingGoToHarbor: // geht von seinem Heimatmilitärgebäude zum Starthafen
        {
            // Gucken, ob der Abflughafen auch noch steht und sich in unserer Hand befindet
            noBase* hb = world->GetNO(harborPos);
            const bool valid_harbor =
              hb->GetGOT() == GO_Type::NobHarborbuilding && static_cast<nobHarborBuilding*>(hb)->GetPlayer() == player;

            // Nicht mehr oder das angegriffene Gebäude kaputt? Dann müssen wir die ganze Aktion abbrechen
            if(!valid_harbor || !attacked_goal)
            {
                // Dann gehen wir halt wieder nach Hause
                ReturnHomeMissionAttacking();
                return;
            }

            // Sind wir schon da?
            if(pos == harborPos)
            {
                // Uns zum Hafen hinzufügen
                state = SoldierState::SeaattackingWaitInHarbor;
                world->GetSpecObj<nobHarborBuilding>(pos)->AddSeaAttacker(world->RemoveFigure(pos, *this));
                return;
            }

            // Erstmal Flagge ansteuern
            MapPoint harborFlagPos = world->GetNeighbour(harborPos, Direction::SouthEast);

            // Wenn wir an der Flagge bereits sind, in den Hafen eintreten
            if(pos == harborFlagPos)
                StartWalking(Direction::NorthWest);
            else
            {
                // Weg zum Hafen suchen
                const auto dir = world->FindHumanPath(pos, harborFlagPos, MAX_ATTACKING_RUN_DISTANCE, false, nullptr);
                if(!dir)
                {
                    // Kein Weg gefunden? Dann auch abbrechen!
                    ReturnHomeMissionAttacking();
                    return;
                }

                // Und schön weiterlaufen
                StartWalking(*dir);
            }
        }
        break;
        case SoldierState::SeaattackingWaitInHarbor: // wartet im Hafen auf das ankommende Schiff
        {
        }
        break;
        case SoldierState::SeaattackingOnShip: // befindet sich auf dem Schiff auf dem Weg zum Zielpunkt
        {
            // Auweia, das darf nicht passieren
            RTTR_Assert(false);
        }
        break;
        case SoldierState::SeaattackingReturnToShip: // befindet sich an der Zielposition auf dem Weg zurück zum
                                                     // Schiff
        {
            HandleState_SeaAttack_ReturnToShip();
        }
        break;
    }
}

/// Wenn ein Heimat-Militärgebäude bei Missionseinsätzen zerstört wurde
void nofAttacker::HomeDestroyed()
{
    switch(state)
    {
        case SoldierState::AttackingWaitingAroundBuilding:
        {
            // Hier muss sofort reagiert werden, da man steht

            // Angreifer muss zusätzlich seinem Ziel Bescheid sagen
            nobBaseMilitary* curGoal = attacked_goal; // attacked_goal gets reset
            InformTargetsAboutCancelling();

            // Ggf. Schiff Bescheid sagen (Schiffs-Angreifer)
            if(ship_obj_id)
                CancelAtShip();

            // Rumirren
            building = nullptr;
            state = SoldierState::FigureWork;
            StartWandering();
            Wander();

            // und evtl einen Nachrücker für diesen Platz suchen
            curGoal->SendSuccessor(pos, radius);
        }
        break;
        default:
        {
            // Wenn wir gerade auf dem Weg dorthin sind (z.B. an Bord eines Schiffs), Ziel löschen
            if(goal_ == building)
                goal_ = nullptr;

            //  Die normale Tätigkeit wird erstmal fortgesetzt (Laufen, Kämpfen, wenn er schon an der Fahne ist
            // wird er auch nicht mehr zurückgehen)
            building = nullptr;
        }
        break;
    }
}

void nofAttacker::HomeDestroyedAtBegin()
{
    building = nullptr;

    // angegriffenem Gebäude Bescheid sagen, dass wir doch nicht mehr kommen
    InformTargetsAboutCancelling();

    state = SoldierState::FigureWork;

    // Rumirren
    StartWandering();
    StartWalking(RANDOM_ENUM(Direction));
}

/// Wenn ein Kampf gewonnen wurde
void nofAttacker::WonFighting()
{
    // addon BattlefieldPromotion active? -> increase rank!
    if(world->GetGGS().isEnabled(AddonId::BATTLEFIELD_PROMOTION))
        IncreaseRank();
    // Ist evtl. unser Heimatgebäude zerstört?
    if(!building && state != SoldierState::AttackingFightingVsDefender)
    {
        // Dann dem Ziel Bescheid sagen, falls es existiert (evtl. wurdes zufällig zur selben Zeit zerstört)
        InformTargetsAboutCancelling();

        // Ggf. Schiff Bescheid sagen (Schiffs-Angreifer)
        if(ship_obj_id)
            CancelAtShip();

        // Rumirren
        state = SoldierState::FigureWork;
        StartWandering();
        Wander();
        return;
    }

    // Ist evtl. unser Ziel-Gebäude zerstört?
    if(!attacked_goal)
    {
        // Nach Hause gehen
        ReturnHomeMissionAttacking();
        return;
    }
    ContinueAtFlag();
}

/// Doesn't find a defender at the flag -> Send defenders or capture it
void nofAttacker::ContinueAtFlag()
{
    RTTR_Assert(attacked_goal);
    // Greifen wir grad ein Gebäude an?
    if(state == SoldierState::AttackingFightingVsDefender
       || (state == SoldierState::Fighting && attacked_goal->GetFlagPos() == pos))
    {
        // Dann neuen Verteidiger rufen
        if(attacked_goal->CallDefender(*this)) //-V522
        {
            // Verteidiger gefunden --> hinstellen und auf ihn warten
            SwitchStateAttackingWaitingForDefender();
        } else
        {
            // check for soldiers of other non-friendly non-owner players to fight
            if(FindEnemiesNearby(attacked_goal->GetPlayer()))
                return;
            // kein Verteidiger gefunden --> ins Gebäude laufen und es erobern
            state = SoldierState::AttackingCapturingFirst;
            StartWalking(Direction::NorthWest);

            // Normalen Militärgebäuden schonmal Bescheid sagen
            if(attacked_goal->GetGOT() == GO_Type::NobMilitary)
                static_cast<nobMilitary*>(attacked_goal)->PrepareCapturing();
        }
    } else
    {
        // weiterlaufen
        state = SoldierState::AttackingWalkingToGoal;
        MissAttackingWalk();
    }
}

/// Wenn ein Kampf verloren wurde (Tod)
void nofAttacker::LostFighting()
{
    // Meinem zu Hause Bescheid sagen, dass ich nicht mehr lebe (damit neue Truppen reinkönnen)
    // falls das Gebäude noch existiert
    AbrogateWorkplace();

    // Angreifer müssen zusätzlich ihrem Ziel Bescheid sagen
    InformTargetsAboutCancelling();

    // Ggf. Schiff Bescheid sagen
    if(ship_obj_id)
        this->CancelAtShip();
}

void nofAttacker::ReturnHomeMissionAttacking()
{
    // Zielen Bescheid sagen
    InformTargetsAboutCancelling();
    // Schiffsangreifer?
    if(ship_obj_id)
    {
        state = SoldierState::SeaattackingReturnToShip;
        HandleState_SeaAttack_ReturnToShip();
    } else
        // Und nach Hause gehen
        ReturnHome();
}

void nofAttacker::MissAttackingWalk()
{
    // Ist evtl. unser Heimatgebäude zerstört?
    if(!building)
    {
        // Dann dem Ziel Bescheid sagen, falls es existiert (evtl. wurdes zufällig zur selben Zeit zerstört)
        InformTargetsAboutCancelling();

        // Ggf. Schiff Bescheid sagen (Schiffs-Angreifer)
        if(ship_obj_id)
            CancelAtShip();

        // Rumirren
        state = SoldierState::FigureWork;
        StartWandering();
        Wander();

        return;
    }

    // Gibts das Ziel überhaupt noch?
    if(!attacked_goal)
    {
        ReturnHomeMissionAttacking();
        return;
    }

    /*// Is it still a hostile destination?
    // (Could be captured in the meantime)
    if(!players->getElement(player)->IsPlayerAttackable(attacked_goal->GetPlayer()))
    {
        ReturnHomeMissionAttacking();
        return;
    }*/

    // Eine Position rund um das Militärgebäude suchen
    MapPoint goal = attacked_goal->FindAnAttackerPlace(radius, this);

    // Keinen Platz mehr gefunden?
    if(!goal.isValid())
    {
        // Dann nach Haus gehen
        ReturnHomeMissionAttacking();
        return;
    }

    // Sind wir evtl schon da?
    if(pos == goal)
    {
        ReachedDestination();
        return;
    }

    // Find all sorts of enemies (attackers, aggressive defenders..) nearby
    if(FindEnemiesNearby())
        // Enemy found -> abort, because nofActiveSoldier handles all things now
        return;

    // Haben wir noch keinen Feind?
    // Könnte mir noch ein neuer Verteidiger entgegenlaufen?
    TryToOrderAggressiveDefender();

    // Ansonsten Weg zum Ziel suchen
    const auto dir = world->FindHumanPath(pos, goal, MAX_ATTACKING_RUN_DISTANCE, true);
    // Keiner gefunden? Nach Hause gehen
    if(!dir)
    {
        ReturnHomeMissionAttacking();
        return;
    }

    // Start walking
    StartWalking(*dir);
}

/// Ist am Militärgebäude angekommen
void nofAttacker::ReachedDestination()
{
    // Sind wir direkt an der Flagge?
    if(pos == attacked_goal->GetFlagPos())
    {
        // Building already captured? Continue capturing
        // This can only be a far away attacker
        if(attacked_goal->GetPlayer() == player)
        {
            state = SoldierState::AttackingCapturingNext;
            RTTR_Assert(dynamic_cast<nobMilitary*>(attacked_goal));
            auto* goal = static_cast<nobMilitary*>(attacked_goal);
            RTTR_Assert(goal->IsFarAwayCapturer(*this));
            // Start walking first so the flag is free
            StartWalking(Direction::NorthWest);
            // Then tell the building
            goal->FarAwayCapturerReachedGoal(*this);
            return;
        }

        // Post schicken "Wir werden angegriffen" TODO evtl. unschön, da jeder Attacker das dann aufruft
        SendPostMessage(attacked_goal->GetPlayer(),
                        std::make_unique<PostMsgWithBuilding>(GetEvMgr().GetCurrentGF(), _("We are under attack!"),
                                                              PostCategory::Military, *attacked_goal));

        // Dann Verteidiger rufen
        if(attacked_goal->CallDefender(*this))
        {
            // Verteidiger gefunden --> hinstellen und auf ihn warten
            SwitchStateAttackingWaitingForDefender();
        } else
        {
            // kein Verteidiger gefunden --> ins Gebäude laufen und es erobern
            state = SoldierState::AttackingCapturingFirst;
            StartWalking(Direction::NorthWest);
            // Normalen Militärgebäuden schonmal Bescheid sagen
            if(attacked_goal->GetGOT() == GO_Type::NobMilitary)
                static_cast<nobMilitary*>(attacked_goal)->PrepareCapturing();
        }
    } else
    {
        // dann hinstellen und warten, bis wir an die Reihe kommmen mit Kämpfen und außerdem diesen Platz
        // reservieren, damit sich kein anderer noch hier hinstellt
        state = SoldierState::AttackingWaitingAroundBuilding;
        // zur Flagge hin ausrichten
        Direction dir(Direction::West);
        MapPoint attFlagPos = attacked_goal->GetFlagPos();
        if(pos.y == attFlagPos.y && pos.x <= attFlagPos.x)
            dir = Direction::East;
        else if(pos.y == attFlagPos.y && pos.x > attFlagPos.x)
            dir = Direction::West;
        else if(pos.y < attFlagPos.y && pos.x < attFlagPos.x)
            dir = Direction::SouthEast;
        else if(pos.y < attFlagPos.y && pos.x > attFlagPos.x)
            dir = Direction::SouthWest;
        else if(pos.y > attFlagPos.y && pos.x < attFlagPos.x)
            dir = Direction::NorthEast;
        else if(pos.y > attFlagPos.y && pos.x > attFlagPos.x)
            dir = Direction::NorthWest;
        else /* (pos.x ==  attFlagPos.x)*/
        {
            if(pos.y < attFlagPos.y && !(safeDiff(pos.y, attFlagPos.y) & 1))
                dir = Direction::SouthEast;
            else if(pos.y < attFlagPos.y && (safeDiff(pos.y, attFlagPos.y) & 1))
            {
                if(pos.y & 1)
                    dir = Direction::SouthWest;
                else
                    dir = Direction::SouthEast;
            } else if(pos.y > attFlagPos.y && !(safeDiff(pos.y, attFlagPos.y) & 1))
                dir = Direction::NorthEast;
            else /* (pos.y > attFlagPos.y && (safeDiff(pos.y, attFlagPos.y) & 1))*/
            {
                if(pos.y & 1)
                    dir = Direction::NorthWest;
                else
                    dir = Direction::NorthEast;
            }
        }
        FaceDir(dir);
        if(attacked_goal->GetPlayer() == player)
        {
            // Building already captured? -> Then we might be a far-away-capturer
            // -> Tell the building, that we are here
            RTTR_Assert(dynamic_cast<nobMilitary*>(attacked_goal));
            auto* goal = static_cast<nobMilitary*>(attacked_goal);
            if(goal->IsFarAwayCapturer(*this))
                goal->FarAwayCapturerReachedGoal(*this);
        }
    }
}

/// Versucht, eine aggressiven Verteidiger für uns zu bestellen
void nofAttacker::TryToOrderAggressiveDefender()
{
    RTTR_Assert(state == SoldierState::AttackingWalkingToGoal);
    // Haben wir noch keinen Gegner?
    // Könnte mir noch ein neuer Verteidiger entgegenlaufen?
    if(!mayBeHunted)
        return;

    // 20%ige Chance, dass wirklich jemand angreift
    if(RANDOM_RAND(10) >= 2)
        return;

    OrderAggressiveDefender();
}

void nofAttacker::OrderAggressiveDefender()
{
    // Militärgebäude in der Nähe abgrasen
    sortedMilitaryBlds buildings = world->LookForMilitaryBuildings(pos, 2);
    for(nobBaseMilitary* bld : buildings)
    {
        // darf kein HQ sein, außer, das HQ wird selbst angegriffen,
        if(bld->GetBuildingType() == BuildingType::Headquarters && bld != attacked_goal)
            continue;
        // darf nicht weiter weg als 15 sein
        if(world->CalcDistance(pos, bld->GetPos()) >= 15)
            continue;
        const unsigned bldOwnerId = bld->GetPlayer();
        if(canPlayerSendAggDefender[bldOwnerId] == 0)
            continue;
        // We only send a defender if we are allied with the attacked player and can attack the attacker (no pact etc)
        GamePlayer& bldOwner = world->GetPlayer(bldOwnerId);
        if(bldOwner.IsAlly(attacked_goal->GetPlayer()) && bldOwner.IsAttackable(player))
        {
            // If player did not decide on sending do it now.
            // Doing this as late as here reduces chance, that the player changed the setting when the defender is asked
            // for
            if(canPlayerSendAggDefender[bldOwnerId] == 2)
            {
                bool sendDefender = bldOwner.ShouldSendDefender();
                if(sendDefender)
                    canPlayerSendAggDefender[bldOwnerId] = 1;
                else
                {
                    canPlayerSendAggDefender[bldOwnerId] = 0;
                    continue;
                }
            }
            // ggf. Verteidiger rufen
            huntingDefender = bld->SendAggressiveDefender(*this);
            if(huntingDefender)
            {
                // nun brauchen wir keinen Verteidiger mehr
                mayBeHunted = false;
                break;
            }
        }
    }
}

void nofAttacker::AttackedGoalDestroyed()
{
    attacked_goal = nullptr;

    bool was_waiting_for_defender = (state == SoldierState::AttackingWaitingForDefender);

    // Wenn man gerade rumsteht, muss man sich bewegen
    if(state == SoldierState::AttackingWaitingForDefender || state == SoldierState::AttackingWaitingAroundBuilding
       || state == SoldierState::WaitingForFight)
        ReturnHomeMissionAttacking();
    else if(state == SoldierState::SeaattackingWaitInHarbor)
    {
        // We don't need to wait anymore, target was destroyed
        auto* harbor = world->GetSpecObj<nobHarborBuilding>(harborPos);
        RTTR_Assert(harbor);
        // go home
        goal_ = building;
        state = SoldierState::FigureWork;
        fs = FigureState::GotToGoal;
        harbor->CancelSeaAttacker(this);
        return;
    }

    if(was_waiting_for_defender)
    {
        // Block-Event ggf abmelden
        GetEvMgr().RemoveEvent(blocking_event);
        world->RoadNodeAvailable(pos);
    }
}

bool nofAttacker::AttackFlag(nofDefender* /*defender*/)
{
    // Zur Flagge laufen, findet er einen Weg?
    const auto dir = world->FindHumanPath(pos, attacked_goal->GetFlagPos(), 3, true);

    if(dir)
    {
        // Hat er drumrum gewartet?
        bool waiting_around_building = (state == SoldierState::AttackingWaitingAroundBuilding);

        // Ja er hat einen Weg gefunden, also hinlaufen

        // Wenn er steht, muss er loslaufen
        if(waiting_around_building)
            StartWalking(*dir);

        state = SoldierState::AttackingAttackingFlag;

        // Hatte er ums Gebäude gewartet?
        if(waiting_around_building)
        {
            // evtl. Nachrücker senden
            attacked_goal->SendSuccessor(pos, radius);
        }
        return true;
    }
    return false;
}

void nofAttacker::AttackFlag()
{
    // "Normal" zur Flagge laufen
    state = SoldierState::AttackingWalkingToGoal;
    MissAttackingWalk();
}

void nofAttacker::CaptureBuilding()
{
    // mit ins Militärgebäude gehen
    state = SoldierState::AttackingCapturingNext;
    // und hinlaufen
    CapturingWalking();
}

void nofAttacker::CapturingWalking()
{
    // Ist evtl. das Zielgebäude zerstört?
    if(!attacked_goal)
    {
        // Nach Hause gehen
        ReturnHomeMissionAttacking();
        return;
    }
    RTTR_Assert(dynamic_cast<nobMilitary*>(attacked_goal));

    MapPoint attFlagPos = attacked_goal->GetFlagPos();

    // Sind wir schon im Gebäude?
    if(pos == attacked_goal->GetPos())
    {
        // Meinem alten Heimatgebäude Bescheid sagen (falls es noch existiert)
        if(building)
            building->SoldierLost(this);
        CancelAtHuntingDefender();
        if(ship_obj_id)
            CancelAtShip();
        // Das ist nun mein neues zu Hause
        building = attacked_goal;
        // und zum Gebäude hinzufügen
        attacked_goal->AddActiveSoldier(world->RemoveFigure(pos, *this));

        // Ein erobernder Soldat weniger
        if(BuildingProperties::IsMilitary(attacked_goal->GetBuildingType()))
        {
            RTTR_Assert(dynamic_cast<nobMilitary*>(attacked_goal));
            auto* goal = static_cast<nobMilitary*>(attacked_goal);
            // If we are still a far-away-capturer at this point, then the building belongs to us and capturing was
            // already finished
            if(!goal->IsFarAwayCapturer(*this))
            {
                RemoveFromAttackedGoal();
                goal->CapturingSoldierArrived();
            } else
            {
                RemoveFromAttackedGoal();
                RTTR_Assert(goal->GetPlayer() == player);
            }
        } else
            RemoveFromAttackedGoal();
    }
    // oder zumindest schonmal an der Flagge?
    else if(pos == attFlagPos)
    {
        // ins Gebäude laufen
        StartWalking(Direction::NorthWest);
        // nächsten Angreifer ggf. rufen, der auch reingehen soll
        RTTR_Assert(attacked_goal->GetPlayer() == player); // Assumed by the call below
        static_cast<nobMilitary*>(attacked_goal)->NeedOccupyingTroops();
    } else
    {
        // Ist evtl. unser Heimatgebäude zerstört?
        if(!building)
        {
            // Wenn noch das Ziel existiert (könnte ja zeitgleich abgebrannt worden sein)
            if(attacked_goal)
            {
                auto* attackedBld = static_cast<nobMilitary*>(attacked_goal);
                RemoveFromAttackedGoal();
                // Evtl. neue Besatzer rufen
                RTTR_Assert(attackedBld->GetPlayer() == player);
                attackedBld->NeedOccupyingTroops();
            }

            // Ggf. Schiff Bescheid sagen (Schiffs-Angreifer)
            if(ship_obj_id)
                CancelAtShip();

            // Rumirren
            state = SoldierState::FigureWork;
            StartWandering();
            Wander();

            return;
        }

        // weiter zur Flagge laufen
        const auto dir = world->FindHumanPath(pos, attFlagPos, 10, true);
        if(dir)
            StartWalking(*dir);
        else
        {
            // auweia, es wurde kein Weg mehr gefunden

            // Evtl. neue Besatzer rufen
            RTTR_Assert(attacked_goal->GetPlayer() == player); // Assumed by the call below
            static_cast<nobMilitary*>(attacked_goal)->NeedOccupyingTroops();
            // Nach Hause gehen
            ReturnHomeMissionAttacking();
        }
    }
}

void nofAttacker::CapturedBuildingFull()
{
    // No need to notify the goal
    attacked_goal = nullptr;

    switch(state)
    {
        default: break;

        case SoldierState::AttackingWaitingAroundBuilding:
        {
            // nach Hause gehen
            ReturnHomeMissionAttacking();
        }
        break;
        case SoldierState::AttackingWalkingToGoal:
        case SoldierState::AttackingWaitingForDefender:
        case SoldierState::AttackingAttackingFlag:
        case SoldierState::WaitingForFight:
        case SoldierState::MeetEnemy:
        case SoldierState::Fighting:
        case SoldierState::SeaattackingGoToHarbor:   // geht von seinem Heimatmilitärgebäude zum Starthafen
        case SoldierState::SeaattackingWaitInHarbor: // wartet im Hafen auf das ankommende Schiff
        case SoldierState::SeaattackingOnShip:       // befindet sich auf dem Schiff auf dem Weg zum Zielpunkt
        {
            // Bei allem anderen läuft man oder kämpft --> auf 0 setzen und wenn man fertig
            // mit der jetzigen Aktion ist, entsprechend handeln (nicht die Einnehmer darüber benachrichten, sonst
            // gehen die nicht rein)
            attacked_goal = nullptr;
        }
        break;
    }
}

void nofAttacker::StartSucceeding(const MapPoint /*pt*/, unsigned short /*new_radius*/)
{
    state = SoldierState::AttackingWalkingToGoal;

    const MapPoint oldPos = pos;
    const unsigned short oldRadius = radius;

    MissAttackingWalk();

    if(IsMoving())
        attacked_goal->SendSuccessor(oldPos, oldRadius);
}

void nofAttacker::LetsFight(nofAggressiveDefender* other)
{
    RTTR_Assert(!huntingDefender);
    // wir werden jetzt "gejagt"
    mayBeHunted = false;
    huntingDefender = other;
}

void nofAttacker::AggressiveDefenderLost()
{
    RTTR_Assert(huntingDefender);
    huntingDefender = nullptr;
}

void nofAttacker::SwitchStateAttackingWaitingForDefender()
{
    state = SoldierState::AttackingWaitingForDefender;
    // Blockevent anmelden
    blocking_event = GetEvMgr().AddEvent(this, BLOCK_OFFSET, 5);
}

void nofAttacker::HandleDerivedEvent(const unsigned /*id*/)
{
    // abfragen, nich dass er evtl schon losgelaufen ist wieder, weil das Gebäude abgebrannt wurde etc.
    if(state == SoldierState::AttackingWaitingForDefender)
    {
        // Figuren stoppen
        world->StopOnRoads(pos);
        blocking_event = nullptr;
    }
}

bool nofAttacker::IsBlockingRoads() const
{
    if(state != SoldierState::AttackingWaitingForDefender)
        return false;

    // Wenn Block-Event schon abgelaufen ist --> blocking_event = 0, da dürfen sich nicht mehr durch
    // wenn es das noch gibt, ist es noch nicht abgelaufen und die Leute können noch durchgehen
    return blocking_event == nullptr;
}

/// Sagt den verschiedenen Zielen Bescheid, dass wir doch nicht mehr kommen können
void nofAttacker::InformTargetsAboutCancelling()
{
    nofActiveSoldier::InformTargetsAboutCancelling();
    CancelAtHuntingDefender();

    // Ziel Bescheid sagen, falls es das noch gibt
    if(attacked_goal)
        RemoveFromAttackedGoal();
    RTTR_Assert(attacked_goal == nullptr);
}

void nofAttacker::RemoveFromAttackedGoal()
{
    // If state == AttackingFightingvsdefender then we probably just lost the fight against the defender,
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

/// Startet den Angriff am Landungspunkt vom Schiff
void nofAttacker::StartAttackOnOtherIsland(const MapPoint shipPos, const unsigned ship_id)
{
    pos = this->shipPos = shipPos;
    this->ship_obj_id = ship_id;

    state = SoldierState::AttackingWalkingToGoal;
    on_ship = false;
    // Normal weiterlaufen
    MissAttackingWalk();
}

/// Sea attacker enters harbor and finds no shipping route or no longer has a valid target: set
/// state,target,goal,building to 0 to avoid future problems (and add to harbor inventory)
void nofAttacker::SeaAttackFailedBeforeLaunch()
{
    InformTargetsAboutCancelling();
    RTTR_Assert(!huntingDefender);
    AbrogateWorkplace();
    goal_ = nullptr;
    state = SoldierState::FigureWork;
}

/// Sagt Schiffsangreifern, dass sie mit dem Schiff zurück fahren
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

/// notify sea attackers that they wont return home
void nofAttacker::HomeHarborLost()
{
    goal_ =
      nullptr; // this in combination with telling the home building that the soldier is lost should work just fine
}

/// Für Schiffsangreifer: Sagt dem Schiff Bescheid, dass wir nicht mehr kommen
void nofAttacker::CancelAtShip()
{
    // Alle Figuren durchgehen
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

/// Behandelt das Laufen zurück zum Schiff
void nofAttacker::HandleState_SeaAttack_ReturnToShip()
{
    // Ist evtl. unser Heimatgebäude zerstört?
    if(!building)
    {
        // Rumirren
        state = SoldierState::FigureWork;
        StartWandering();
        Wander();

        // Schiff Bescheid sagen
        CancelAtShip();
        return;
    }

    // Sind wir schon im Schiff?
    if(pos == shipPos)
    {
        // Alle Figuren durchgehen
        for(noBase& figure : world->GetFigures(pos))
        {
            if(figure.GetObjId() == ship_obj_id)
            {
                StartReturnViaShip(static_cast<noShip&>(figure));
                return;
            }
        }

        RTTR_Assert(false);
        ship_obj_id = 0;
        // Kein Schiff gefunden? Das kann eigentlich nich sein!
        // Dann rumirren
        StartWandering();
        state = SoldierState::FigureWork;
        Wander();
        return;
    }
    const auto dir = world->FindHumanPath(pos, shipPos, MAX_ATTACKING_RUN_DISTANCE);
    if(dir)
        StartWalking(*dir);
    else
    {
        // Kein Weg gefunden --> Rumirren
        StartWandering();
        state = SoldierState::FigureWork;
        Wander();

        // Dem Heimatgebäude Bescheid sagen
        building->SoldierLost(this);
        // Und dem Schiff
        CancelAtShip();
    }
}

/// Bricht einen Seeangriff ab
void nofAttacker::CancelSeaAttack()
{
    InformTargetsAboutCancelling();
    RTTR_Assert(!huntingDefender);
    Abrogate();
}

/// The derived classes regain control after a fight of nofActiveSoldier
void nofAttacker::FreeFightEnded()
{
    nofActiveSoldier::FreeFightEnded();
    // Continue with normal walking towards our goal
    state = SoldierState::AttackingWalkingToGoal;
}

/// Try to start capturing although he is still far away from the destination
/// Returns true if successful
bool nofAttacker::TryToStartFarAwayCapturing(nobMilitary* dest)
{
    // Are we already walking to the destination?
    if(state == SoldierState::AttackingWalkingToGoal || state == SoldierState::MeetEnemy
       || state == SoldierState::WaitingForFight || state == SoldierState::Fighting)
    {
        // Not too far away?
        if(world->CalcDistance(pos, dest->GetPos()) < MAX_FAR_AWAY_CAPTURING_DISTANCE)
            return true;
    }

    return false;
}
