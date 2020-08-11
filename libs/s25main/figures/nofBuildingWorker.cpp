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
#include "world/GameWorldGame.h"
#include "nodeObjs/noFlag.h"
#include "gameData/JobConsts.h"
#include "gameData/ShieldConsts.h"

nofBuildingWorker::nofBuildingWorker(const Job job, const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : noFigure(job, pos, player, workplace), state(STATE_FIGUREWORK), workplace(workplace), was_sounding(false)
{
    RTTR_Assert(dynamic_cast<nobUsual*>(
      static_cast<GameObject*>(workplace))); // Assume we have at least a GameObject and check if it is a valid workplace
}

nofBuildingWorker::nofBuildingWorker(const Job job, const MapPoint pos, const unsigned char player, nobBaseWarehouse* goalWh)
    : noFigure(job, pos, player, goalWh), state(STATE_FIGUREWORK), workplace(nullptr), was_sounding(false)
{}

void nofBuildingWorker::Serialize_nofBuildingWorker(SerializedGameData& sgd) const
{
    Serialize_noFigure(sgd);

    sgd.PushUnsignedChar(static_cast<unsigned char>(state));

    if(fs != FS_GOHOME && fs != FS_WANDER)
    {
        sgd.PushObject(workplace, false);
        sgd.PushOptionalEnum<uint8_t>(ware);
        sgd.PushBool(was_sounding);
    }
}

nofBuildingWorker::nofBuildingWorker(SerializedGameData& sgd, const unsigned obj_id)
    : noFigure(sgd, obj_id), state(State(sgd.PopUnsignedChar()))
{
    if(fs != FS_GOHOME && fs != FS_WANDER)
    {
        workplace = sgd.PopObject<nobUsual>(GOT_UNKNOWN);
        if(sgd.GetGameDataVersion() < 5)
        {
            const auto iWare = sgd.PopUnsignedChar();
            if(iWare == GD_NOTHING)
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
        case STATE_FIGUREWORK:
        case STATE_HUNTER_CHASING:
        case STATE_HUNTER_WALKINGTOCADAVER:
        case STATE_HUNTER_FINDINGSHOOTINGPOINT: DrawWalking(drawPt); break;
        case STATE_WORK:
        case STATE_HUNTER_SHOOTING:
        case STATE_HUNTER_EVISCERATING:
        case STATE_HUNTER_WAITING_FOR_ANIMAL_READY:
        case STATE_CATAPULT_TARGETBUILDING:
        case STATE_CATAPULT_BACKOFF: DrawWorking(drawPt); break;
        case STATE_CARRYOUTWARE: DrawWalkingWithWare(drawPt); break;
        case STATE_WALKINGHOME:
        case STATE_ENTERBUILDING:
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
        case STATE_ENTERBUILDING:
        {
            // Hab ich noch ne Ware in der Hand?

            if(ware)
            {
                // dann war draußen kein Platz --> ist jetzt evtl Platz?
                state = STATE_WAITFORWARESPACE;
                if(workplace->GetFlag()->GetNumWares() < 8)
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
        case STATE_CARRYOUTWARE:
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
        if(flag->GetNumWares() < 8)
        {
            // Ware erzeugen
            auto* real_ware = new Ware(*ware, nullptr, flag);
            real_ware->WaitAtFlag(flag);
            // Inventur entsprechend erhöhen, dabei Schilder unterscheiden!
            GoodType ware_type = ConvertShields(real_ware->type);
            gwg->GetPlayer(player).IncreaseInventoryWare(ware_type, 1);
            // Abnehmer für Ware finden
            real_ware->SetGoal(gwg->GetPlayer(player).FindClientForWare(real_ware));
            // Ware soll ihren weiteren Weg berechnen
            real_ware->RecalcRoute();
            // Ware ablegen
            flag->AddWare(real_ware);
            // Warenstatistik erhöhen
            gwg->GetPlayer(this->player).IncreaseMerchandiseStatistic(ware_type);
            // Tragen nun keine Ware mehr
            ware = boost::none;
        }
    }

    // Wieder reingehen
    StartWalking(Direction::NORTHWEST);
    state = STATE_ENTERBUILDING;
}

void nofBuildingWorker::TryToWork()
{
    if(!workplace->IsProductionDisabled() && AreWaresAvailable())
    {
        state = STATE_WAITING1;
        current_ev =
          GetEvMgr().AddEvent(this, (GetGOT() == GOT_NOF_CATAPULTMAN) ? CATAPULT_WAIT1_LENGTH : JOB_CONSTS[job_].wait1_length, 1);
        workplace->StopNotWorking();
    } else
    {
        state = STATE_WAITINGFORWARES_OR_PRODUCTIONSTOPPED;
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
    if(state == STATE_WAITINGFORWARES_OR_PRODUCTIONSTOPPED)
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
    if(state == STATE_WAITFORWARESPACE)
    {
        StartWalking(Direction::SOUTHEAST);
        state = STATE_CARRYOUTWARE;
        return true;
    } else
        return false;
}
void nofBuildingWorker::LostWork()
{
    switch(state)
    {
        default: break;
        case STATE_FIGUREWORK:
        {
            // Auf Wegen nach Hause gehen
            GoHome();
        }
        break;
        case STATE_WAITING1:
        case STATE_WAITING2:
        case STATE_WORK:
        case STATE_WAITINGFORWARES_OR_PRODUCTIONSTOPPED:
        case STATE_WAITFORWARESPACE:
        case STATE_HUNTER_SHOOTING:
        case STATE_HUNTER_EVISCERATING:
        case STATE_HUNTER_WAITING_FOR_ANIMAL_READY:
        case STATE_CATAPULT_TARGETBUILDING:
        case STATE_CATAPULT_BACKOFF:
        {
            // Bisheriges Event abmelden, da die Arbeit unterbrochen wird
            GetEvMgr().RemoveEvent(current_ev);

            // Bescheid sagen, dass Arbeit abgebrochen wurde
            WorkAborted();

            // Rumirren
            StartWandering();
            Wander();

            // Evtl. Sounds löschen
            SOUNDMANAGER.WorkingFinished(this);

            state = STATE_FIGUREWORK;
        }
        break;
        case STATE_ENTERBUILDING:
        case STATE_CARRYOUTWARE:
        case STATE_WALKTOWORKPOINT:
        case STATE_WALKINGHOME:
        case STATE_HUNTER_CHASING:
        case STATE_HUNTER_FINDINGSHOOTINGPOINT:
        case STATE_HUNTER_WALKINGTOCADAVER:
        {
            // Bescheid sagen, dass Arbeit abgebrochen wurde
            WorkAborted();

            // Rumirren
            // Bei diesen States läuft man schon, darf also nicht noch zusätzlich Wander aufrufen, da man dann ja im Laufen nochmal
            // losläuft!
            StartWandering();

            // Evtl. Sounds löschen
            SOUNDMANAGER.WorkingFinished(this);

            state = STATE_FIGUREWORK;
        }
        break;
    }

    workplace = nullptr;
}

void nofBuildingWorker::ProductionStopped()
{
    // Wenn ich gerade warte und schon ein Arbeitsevent angemeldet habe, muss das wieder abgemeldet werden
    if(state == STATE_WAITING1)
    {
        GetEvMgr().RemoveEvent(current_ev);
        current_ev = nullptr;
        state = STATE_WAITINGFORWARES_OR_PRODUCTIONSTOPPED;
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
