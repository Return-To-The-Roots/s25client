// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
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
        case State::HunterFindingShootingpoint:
        case State::SkinnerWalkingToCarcass: DrawWalking(drawPt); break;
        case State::Work:
        case State::HunterShooting:
        case State::HunterEviscerating:
        case State::HunterWaitingForAnimalReady:
        case State::SkinnerSkinningCarcass:
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
            // Hab ich noch ne Ware in der Hand?

            if(ware)
            {
                // dann war draußen kein Platz --> ist jetzt evtl Platz?
                state = State::WaitForWareSpace;
                if(workplace->GetFlag()->HasSpaceForWare())
                    FreePlaceAtFlag();
                // Ab jetzt warten, d.h. nicht mehr arbeiten --> schlecht für die Produktivität
                workplace->StartNotWorking();
            } else
            {
                // Anfangen zu Arbeiten
                TryToWork();
            }
        }
        break;
        case State::CarryoutWare:
        {
            // Alles weitere übernimmt nofBuildingWorker
            WorkingReady();
        }
        break;
        default: WalkedDerived();
    }
}

void nofBuildingWorker::WorkingReady()
{
    // wir arbeiten nicht mehr
    workplace->is_working = false;

    // Trage ich eine Ware?
    if(ware)
    {
        noFlag* flag = workplace->GetFlag();
        // Ist noch Platz an der Fahne?
        if(flag->HasSpaceForWare())
        {
            // Ware erzeugen
            auto real_ware = std::make_unique<Ware>(*ware, nullptr, flag);
            real_ware->WaitAtFlag(flag);
            // Inventur entsprechend erhöhen, dabei Schilder unterscheiden!
            GoodType ware_type = ConvertShields(real_ware->type);
            world->GetPlayer(player).IncreaseInventoryWare(ware_type, 1);
            // Abnehmer für Ware finden
            real_ware->SetGoal(world->GetPlayer(player).FindClientForWare(*real_ware));
            // Ware soll ihren weiteren Weg berechnen
            real_ware->RecalcRoute();
            // Ware ablegen
            flag->AddWare(std::move(real_ware));
            // Warenstatistik erhöhen
            world->GetPlayer(this->player).IncreaseMerchandiseStatistic(ware_type);
            // Tragen nun keine Ware mehr
            ware = boost::none;
        }
    }

    // Wieder reingehen
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
        // Nun arbeite ich nich mehr
        workplace->StartNotWorking();
    }
}

bool nofBuildingWorker::AreWaresAvailable() const
{
    return workplace->WaresAvailable();
}

void nofBuildingWorker::GotWareOrProductionAllowed()
{
    // Falls man auf Waren wartet, kann man dann anfangen zu arbeiten
    if(state == State::WaitingForWaresOrProductionStopped)
    {
        // anfangen zu arbeiten
        TryToWork();
    }
}

void nofBuildingWorker::GoalReached()
{
    // Tür zumachen (ist immer bis zu Erstbesetzung offen)
    workplace->CloseDoor();
    // Gebäude Bescheid sagen, dass ich da bin
    workplace->WorkerArrived();

    WorkplaceReached();

    // ggf. anfangen zu arbeiten
    TryToWork();
}

bool nofBuildingWorker::FreePlaceAtFlag()
{
    // Hinaus gehen, um Ware abzulegen, falls wir auf einen freien Platz warten
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
            // Auf Wegen nach Hause gehen
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
        case State::SkinnerSkinningCarcass:
        case State::CatapultTargetBuilding:
        case State::CatapultBackoff:
        {
            // Bisheriges Event abmelden, da die Arbeit unterbrochen wird
            GetEvMgr().RemoveEvent(current_ev);

            // Bescheid sagen, dass Arbeit abgebrochen wurde
            WorkAborted();

            // Rumirren
            StartWandering();
            Wander();

            // Evtl. Sounds löschen
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
        case State::SkinnerWalkingToCarcass:
        {
            // Bescheid sagen, dass Arbeit abgebrochen wurde
            WorkAborted();

            // Rumirren
            // Bei diesen States läuft man schon, darf also nicht noch zusätzlich Wander aufrufen, da man dann ja im
            // Laufen nochmal losläuft!
            StartWandering();

            // Evtl. Sounds löschen
            world->GetSoundMgr().stopSounds(*this);

            state = State::FigureWork;
        }
        break;
    }

    workplace = nullptr;
}

void nofBuildingWorker::ProductionStopped()
{
    // Wenn ich gerade warte und schon ein Arbeitsevent angemeldet habe, muss das wieder abgemeldet werden
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

/// Zeichnen der Figur in sonstigen Arbeitslagen
void nofBuildingWorker::DrawOtherStates(DrawPoint) {}

/// Zeichnet Figur beim Hereinlaufen/nach Hause laufen mit evtl. getragenen Waren
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
