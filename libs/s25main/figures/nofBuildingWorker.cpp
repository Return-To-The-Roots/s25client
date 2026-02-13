// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofBuildingWorker.h"
#include "EventManager.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "RTTR_Assert.h"
#include "SerializedGameData.h"
#include "SoundManager.h"
#include "Ware.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobUsual.h"
#include "helpers/MaxEnumValue.h"
#include "world/GameWorld.h"
#include "nodeObjs/noFlag.h"
#include "gameData/JobConsts.h"
#include "gameData/ShieldConsts.h"

nofBuildingWorker::nofBuildingWorker(const Job job, const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : noFigure(job, pos, player, workplace), state(State::FigureWork), workplace(workplace), was_sounding(false)
{
    RTTR_Assert(dynamic_cast<nobUsual*>(static_cast<GameObject*>(
      workplace))); // Assume we have at least a GameObject and check if it is a valid workplace
}

nofBuildingWorker::nofBuildingWorker(const Job job, const MapPoint pos, const unsigned char player,
                                     nobBaseWarehouse* goalWh)
    : noFigure(job, pos, player, goalWh), state(State::FigureWork), workplace(nullptr), was_sounding(false)
{}

void nofBuildingWorker::Serialize(SerializedGameData& sgd) const
{
    noFigure::Serialize(sgd);

    sgd.PushEnum<uint8_t>(state);

    if(fs != FigureState::GoHome && fs != FigureState::Wander)
    {
        sgd.PushObject(workplace);
        sgd.PushOptionalEnum<uint8_t>(ware);
        sgd.PushBool(was_sounding);
    }
}

nofBuildingWorker::nofBuildingWorker(SerializedGameData& sgd, const unsigned obj_id)
    : noFigure(sgd, obj_id), state(sgd.Pop<State>())
{
    if(fs != FigureState::GoHome && fs != FigureState::Wander)
    {
        workplace = sgd.PopObject<nobUsual>();
        if(sgd.GetGameDataVersion() < 5)
        {
            const auto iWare = sgd.PopUnsignedChar();
            if(iWare == rttr::enum_cast(GoodType::Nothing))
                ware = boost::none;
            else
                ware = GoodType(iWare);
        } else
            ware = sgd.PopOptionalEnum<GoodType>();
        was_sounding = sgd.PopBool();
    } else
    {
        workplace = nullptr;
        ware = boost::none;
        was_sounding = false;
    }
}

void nofBuildingWorker::AbrogateWorkplace()
{
    if(workplace)
    {
        workplace->WorkerLost();
        workplace = nullptr;
    }
}

void nofBuildingWorker::Draw(DrawPoint drawPt)
{
    switch(state)
    {
        case State::FigureWork:
        case State::HunterChasing:
        case State::HunterWalkingToCadaver:
        case State::HunterFindingShootingpoint: DrawWalking(drawPt); break;
        case State::Work:
        case State::HunterShooting:
        case State::HunterEviscerating:
        case State::HunterWaitingForAnimalReady:
        case State::CatapultTargetBuilding:
        case State::CatapultBackoff: DrawWorking(drawPt); break;
        case State::CarryoutWare: DrawWalkingWithWare(drawPt); break;
        case State::WalkingHome:
        case State::EnterBuilding:
            if(ware)
                DrawWalkingWithWare(drawPt);
            else
                DrawWalking(drawPt);
            break;
        default: DrawOtherStates(drawPt); break;
    }
}

void nofBuildingWorker::Walked()
{
    switch(state)
    {
        case State::EnterBuilding:
        {
            // Do I still have a ware in hand?

            if(ware)
            {
                // Then there was no space outside -> maybe there is space now?
                state = State::WaitForWareSpace;
                if(workplace->GetFlag()->HasSpaceForWare())
                    FreePlaceAtFlag();
                // Wait now, i.e. no longer working -> bad for productivity
                workplace->StartNotWorking();
            } else
            {
                // Start working
                TryToWork();
            }
        }
        break;
        case State::CarryoutWare:
        {
            // nofBuildingWorker takes over from here
            WorkingReady();
        }
        break;
        default: WalkedDerived();
    }
}

void nofBuildingWorker::WorkingReady()
{
    // We are no longer working
    workplace->is_working = false;

    // Am I carrying a ware?
    if(ware)
    {
        noFlag* flag = workplace->GetFlag();
        // Is there still space at the flag?
        if(flag->HasSpaceForWare())
        {
            // Create ware
            auto real_ware = std::make_unique<Ware>(*ware, nullptr, flag);
            real_ware->WaitAtFlag(flag);
            // Increase inventory accordingly, distinguishing shields!
            GoodType ware_type = ConvertShields(real_ware->type);
            world->GetPlayer(player).IncreaseInventoryWare(ware_type, 1);
            // Find a recipient for the ware
            real_ware->SetGoal(world->GetPlayer(player).FindClientForWare(*real_ware));
            // Ware should calculate its further route
            real_ware->RecalcRoute();
            // Drop the ware
            flag->AddWare(std::move(real_ware));
            // Increase ware statistics
            world->GetPlayer(this->player).IncreaseMerchandiseStatistic(ware_type);
            workplace->RegisterProducedGood(ware_type);
            // No longer carrying a ware
            ware = boost::none;
        }
    }

    // Go back inside
    StartWalking(Direction::NorthWest);
    state = State::EnterBuilding;
}

void nofBuildingWorker::TryToWork()
{
    if(!workplace->IsProductionDisabled() && AreWaresAvailable())
    {
        state = State::Waiting1;
        current_ev = GetEvMgr().AddEvent(
          this, (GetGOT() == GO_Type::NofCatapultman) ? CATAPULT_WAIT1_LENGTH : JOB_CONSTS[job_].wait1_length, 1);
        workplace->StopNotWorking();
    } else
    {
        state = State::WaitingForWaresOrProductionStopped;
        // Not working anymore
        workplace->StartNotWorking();
    }
}

bool nofBuildingWorker::AreWaresAvailable() const
{
    return workplace->WaresAvailable();
}

void nofBuildingWorker::GotWareOrProductionAllowed()
{
    // If waiting for wares, we can start working then
    if(state == State::WaitingForWaresOrProductionStopped)
    {
        // start working
        TryToWork();
    }
}

void nofBuildingWorker::GoalReached()
{
    // Close the door (always open until the first assignment)
    workplace->CloseDoor();
    // Notify the building that I am here
    workplace->WorkerArrived();

    WorkplaceReached();

    // Start working if applicable
    TryToWork();
}

bool nofBuildingWorker::FreePlaceAtFlag()
{
    // Go out to drop the ware if we are waiting for a free slot
    if(state == State::WaitForWareSpace)
    {
        StartWalking(Direction::SouthEast);
        state = State::CarryoutWare;
        return true;
    } else
        return false;
}
void nofBuildingWorker::LostWork()
{
    switch(state)
    {
        default: break;
        case State::FigureWork:
        {
            // Go home via roads
            GoHome();
        }
        break;
        case State::Waiting1:
        case State::Waiting2:
        case State::Work:
        case State::WaitingForWaresOrProductionStopped:
        case State::WaitForWareSpace:
        case State::HunterShooting:
        case State::HunterEviscerating:
        case State::HunterWaitingForAnimalReady:
        case State::CatapultTargetBuilding:
        case State::CatapultBackoff:
        {
            // Remove the current event since the work is interrupted
            GetEvMgr().RemoveEvent(current_ev);

            // Notify that work was aborted
            WorkAborted();

            // Wander
            StartWandering();
            Wander();

            // Possibly stop sounds
            world->GetSoundMgr().stopSounds(*this);

            state = State::FigureWork;
        }
        break;
        case State::EnterBuilding:
        case State::CarryoutWare:
        case State::WalkToWorkpoint:
        case State::WalkingHome:
        case State::HunterChasing:
        case State::HunterFindingShootingpoint:
        case State::HunterWalkingToCadaver:
        {
            // Notify that work was aborted
            WorkAborted();

            // Wander
            // In these states we are already walking, so don't call Wander again or we'd start walking twice!
            StartWandering();

            // Possibly stop sounds
            world->GetSoundMgr().stopSounds(*this);

            state = State::FigureWork;
        }
        break;
    }

    workplace = nullptr;
}

void nofBuildingWorker::ProductionStopped()
{
    // If I'm waiting and already registered a work event, it has to be removed again
    if(state == State::Waiting1)
    {
        GetEvMgr().RemoveEvent(current_ev);
        current_ev = nullptr;
        state = State::WaitingForWaresOrProductionStopped;
        workplace->StartNotWorking();
    }
}

void nofBuildingWorker::WorkAborted() {}

void nofBuildingWorker::WorkplaceReached() {}

/// Draw the figure in other work states
void nofBuildingWorker::DrawOtherStates(DrawPoint) {}

/// Draw the figure while entering/returning home, possibly carrying wares
void nofBuildingWorker::DrawWalkingWithWare(DrawPoint drawPt)
{
    unsigned short id = GetCarryID();
    // >=CARRY_ID_CARRIER_OFFSET -> carrier.bob else jobs.bob!
    if(id >= CARRY_ID_CARRIER_OFFSET)
    {
        id -= CARRY_ID_CARRIER_OFFSET;
        RTTR_Assert(id <= helpers::MaxEnumValue_v<GoodType>);
        DrawWalkingCarrier(drawPt, GoodType(id), JOB_SPRITE_CONSTS[job_].isFat());
    } else
        DrawWalking(drawPt, LOADER.GetBob("jobs"), id, JOB_SPRITE_CONSTS[job_].isFat());
}
