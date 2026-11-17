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

void nofBuildingWorker::serialize(SerializedGameData& sgd) const
{
    noFigure::serialize(sgd);

    sgd.pushEnum<uint8_t>(state);

    if(fs != FigureState::GoHome && fs != FigureState::Wander)
    {
        sgd.pushObject(workplace);
        sgd.pushOptionalEnum<uint8_t>(ware);
        sgd.PushBool(was_sounding);
    }
}

nofBuildingWorker::nofBuildingWorker(SerializedGameData& sgd, const unsigned obj_id)
    : noFigure(sgd, obj_id), state(sgd.pop<State>())
{
    if(fs != FigureState::GoHome && fs != FigureState::Wander)
    {
        workplace = sgd.popObject<nobUsual>();
        if(sgd.getGameDataVersion() < 5)
        {
            const auto iWare = sgd.PopUnsignedChar();
            // GoodType::Nothing is moved because of adding new wares due addons
            // The old GoodType::Nothing is now GoodType::Grapes
            if(iWare == rttr::enumCast(GoodType::Grapes))
                ware = std::nullopt;
            else
                ware = GoodType(iWare);
        } else
            ware = sgd.popOptionalEnum<GoodType>();
        was_sounding = sgd.PopBool();
    } else
    {
        workplace = nullptr;
        ware = std::nullopt;
        was_sounding = false;
    }
}

void nofBuildingWorker::abrogateWorkplace()
{
    if(workplace)
    {
        workplace->workerLost();
        workplace = nullptr;
    }
}

void nofBuildingWorker::draw(DrawPoint drawPt)
{
    switch(state)
    {
        case State::FigureWork:
        case State::HunterChasing:
        case State::HunterWalkingToCadaver:
        case State::HunterFindingShootingpoint:
        case State::SkinnerWalkingToCarcass: drawWalking(drawPt); break;
        case State::Work:
        case State::HunterShooting:
        case State::HunterEviscerating:
        case State::HunterWaitingForAnimalReady:
        case State::SkinnerSkinningCarcass:
        case State::CatapultTargetBuilding:
        case State::CatapultBackoff: drawWorking(drawPt); break;
        case State::CarryoutWare: drawWalkingWithWare(drawPt); break;
        case State::WalkingHome:
        case State::EnterBuilding:
            if(ware)
                drawWalkingWithWare(drawPt);
            else
                drawWalking(drawPt);
            break;
        default: drawOtherStates(drawPt); break;
    }
}

void nofBuildingWorker::walked()
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
                if(workplace->getFlag()->hasSpaceForWare())
                    freePlaceAtFlag();
                // Ab jetzt warten, d.h. nicht mehr arbeiten --> schlecht für die Produktivität
                workplace->startNotWorking();
            } else
            {
                // Anfangen zu Arbeiten
                tryToWork();
            }
        }
        break;
        case State::CarryoutWare:
        {
            // Alles weitere übernimmt nofBuildingWorker
            workingReady();
        }
        break;
        default: walkedDerived();
    }
}

void nofBuildingWorker::workingReady()
{
    // wir arbeiten nicht mehr
    workplace->is_working = false;

    // Trage ich eine Ware?
    if(ware)
    {
        noFlag* flag = workplace->getFlag();
        // Ist noch Platz an der Fahne?
        if(flag->hasSpaceForWare())
        {
            // Ware erzeugen
            auto real_ware = std::make_unique<Ware>(*ware, nullptr, flag);
            real_ware->waitAtFlag(*flag);
            // Inventur entsprechend erhöhen, dabei Schilder unterscheiden!
            GoodType ware_type = ConvertShields(real_ware->type);
            world->getPlayer(player).increaseInventoryWare(ware_type, 1);
            // Abnehmer für Ware finden
            real_ware->setGoal(world->getPlayer(player).findClientForWare(*real_ware));
            // Ware soll ihren weiteren Weg berechnen
            real_ware->recalcRoute();
            // Ware ablegen
            flag->addWare(std::move(real_ware));
            // Warenstatistik erhöhen
            world->getPlayer(this->player).increaseMerchandiseStatistic(ware_type);
            // Tragen nun keine Ware mehr
            ware = std::nullopt;
        }
    }

    // Wieder reingehen
    startWalking(Direction::NorthWest);
    state = State::EnterBuilding;
}

void nofBuildingWorker::tryToWork()
{
    if(!workplace->isProductionDisabled() && areWaresAvailable())
    {
        state = State::Waiting1;
        current_ev = getEvMgr().addEvent(
          this, (getGOT() == GO_Type::NofCatapultman) ? CATAPULT_WAIT1_LENGTH : JOB_CONSTS[job_].wait1_length, 1);
        workplace->stopNotWorking();
    } else
    {
        state = State::WaitingForWaresOrProductionStopped;
        // Nun arbeite ich nich mehr
        workplace->startNotWorking();
    }
}

bool nofBuildingWorker::areWaresAvailable() const
{
    return workplace->waresAvailable();
}

void nofBuildingWorker::gotWareOrProductionAllowed()
{
    // Falls man auf Waren wartet, kann man dann anfangen zu arbeiten
    if(state == State::WaitingForWaresOrProductionStopped)
    {
        // anfangen zu arbeiten
        tryToWork();
    }
}

void nofBuildingWorker::goalReached()
{
    // Tür zumachen (ist immer bis zu Erstbesetzung offen)
    workplace->closeDoor();
    // Gebäude Bescheid sagen, dass ich da bin
    workplace->workerArrived();

    workplaceReached();

    // ggf. anfangen zu arbeiten
    tryToWork();
}

bool nofBuildingWorker::freePlaceAtFlag()
{
    // Hinaus gehen, um Ware abzulegen, falls wir auf einen freien Platz warten
    if(state == State::WaitForWareSpace)
    {
        startWalking(Direction::SouthEast);
        state = State::CarryoutWare;
        return true;
    } else
        return false;
}
void nofBuildingWorker::lostWork()
{
    switch(state)
    {
        default: break;
        case State::FigureWork:
        {
            // Auf Wegen nach Hause gehen
            goHome();
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
            getEvMgr().removeEvent(current_ev);

            // Bescheid sagen, dass Arbeit abgebrochen wurde
            workAborted();

            // Rumirren
            startWandering();
            wander();

            // Evtl. Sounds löschen
            world->getSoundMgr().stopSounds(*this);

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
            workAborted();

            // Rumirren
            // Bei diesen States läuft man schon, darf also nicht noch zusätzlich Wander aufrufen, da man dann ja im
            // Laufen nochmal losläuft!
            startWandering();

            // Evtl. Sounds löschen
            world->getSoundMgr().stopSounds(*this);

            state = State::FigureWork;
        }
        break;
    }

    workplace = nullptr;
}

void nofBuildingWorker::productionStopped()
{
    // Wenn ich gerade warte und schon ein Arbeitsevent angemeldet habe, muss das wieder abgemeldet werden
    if(state == State::Waiting1)
    {
        getEvMgr().removeEvent(current_ev);
        current_ev = nullptr;
        state = State::WaitingForWaresOrProductionStopped;
        workplace->startNotWorking();
    }
}

void nofBuildingWorker::workAborted() {}

void nofBuildingWorker::workplaceReached() {}

/// Zeichnen der Figur in sonstigen Arbeitslagen
void nofBuildingWorker::drawOtherStates(DrawPoint) {}

/// Zeichnet Figur beim Hereinlaufen/nach Hause laufen mit evtl. getragenen Waren
void nofBuildingWorker::drawWalkingWithWare(DrawPoint drawPt)
{
    unsigned short id = getCarryID();
    // >=CARRY_ID_CARRIER_OFFSET -> carrier.bob else jobs.bob!
    if(id >= CARRY_ID_CARRIER_OFFSET)
    {
        id -= CARRY_ID_CARRIER_OFFSET;
        RTTR_Assert(id <= helpers::MaxEnumValue_v<GoodType>);
        drawWalkingCarrier(drawPt, GoodType(id), JOB_SPRITE_CONSTS[job_].isFat());
    } else
        drawWalking(drawPt, LOADER.getBob("jobs"), id, JOB_SPRITE_CONSTS[job_].isFat());
}
