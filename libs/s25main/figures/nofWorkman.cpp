// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "rttrDefines.h" // IWYU pragma: keep
#include "nofWorkman.h"
#include "EventManager.h"
#include "SoundManager.h"
#include "buildings/nobUsual.h"
#include "world/GameWorldGame.h"
#include "gameData/GameConsts.h"
#include "gameData/JobConsts.h"

nofWorkman::nofWorkman(const Job job, const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofBuildingWorker(job, pos, player, workplace)
{}

nofWorkman::nofWorkman(const Job job, const MapPoint pos, const unsigned char player, nobBaseWarehouse* goalWh)
    : nofBuildingWorker(job, pos, player, goalWh)
{}

void nofWorkman::Serialize_nofWorkman(SerializedGameData& sgd) const
{
    Serialize_nofBuildingWorker(sgd);
}

nofWorkman::nofWorkman(SerializedGameData& sgd, const unsigned obj_id) : nofBuildingWorker(sgd, obj_id) {}

void nofWorkman::HandleDerivedEvent(const unsigned /*id*/)
{
    switch(state)
    {
        case STATE_WAITING1: HandleStateWaiting1(); break;
        case STATE_WORK: HandleStateWork(); break;
        case STATE_WAITING2: HandleStateWaiting2(); break;
        default: break;
    }
}

bool nofWorkman::StartWorking()
{
    current_ev = GetEvMgr().AddEvent(this, JOB_CONSTS[job_].work_length, 1);
    state = STATE_WORK;
    workplace->is_working = true;
    // Waren verbrauchen
    workplace->ConsumeWares();
    return true;
}

void nofWorkman::HandleStateWaiting1()
{
    current_ev = nullptr;
    if(!StartWorking())
    {
        state = STATE_WAITINGFORWARES_OR_PRODUCTIONSTOPPED;
        workplace->StartNotWorking();
    }
}

void nofWorkman::HandleStateWaiting2()
{
    current_ev = nullptr;
    // Ware erzeugen... (noch nicht "richtig"!, sondern nur viruell erstmal)
    if((ware = ProduceWare()) == GD_NOTHING)
    {
        // Soll keine erzeugt werden --> wieder anfangen zu arbeiten
        TryToWork();
    } else
    {
        // und diese raustragen
        StartWalking(Direction::SOUTHEAST);
        state = STATE_CARRYOUTWARE;
    }

    // abgeleiteten Klassen Bescheid sagen
    WorkFinished();
}

void nofWorkman::HandleStateWork()
{
    // Nach Arbeiten wird noch ein bisschen gewartet, bevor das Produkt herausgetragen wird
    // Bei 0 mind. 1 GF
    current_ev = GetEvMgr().AddEvent(this, JOB_CONSTS[job_].wait2_length ? JOB_CONSTS[job_].wait2_length : 1, 1);
    state = STATE_WAITING2;
    // wir arbeiten nicht mehr
    workplace->is_working = false;

    // Evtl. Sounds löschen
    if(was_sounding)
    {
        SOUNDMANAGER.WorkingFinished(this);
        was_sounding = false;
    }
}

namespace {
struct NodeHasResource
{
    const GameWorldGame& gwg;
    const Resource::Type res;
    NodeHasResource(const GameWorldGame& gwg, const Resource::Type res) : gwg(gwg), res(res) {}

    bool operator()(const MapPoint pt) { return gwg.GetNode(pt).resources.has(res); }
};
} // namespace

MapPoint nofWorkman::FindPointWithResource(Resource::Type type) const
{
    // Alle Punkte durchgehen, bis man einen findet, wo man graben kann
    std::vector<MapPoint> pts = gwg->GetPointsInRadius<1>(pos, MINER_RADIUS, Identity<MapPoint>(), NodeHasResource(*gwg, type), true);
    if(!pts.empty())
        return pts.front();

    workplace->OnOutOfResources();

    return MapPoint::Invalid();
}
