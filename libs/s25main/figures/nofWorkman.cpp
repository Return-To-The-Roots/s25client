// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofWorkman.h"
#include "EventManager.h"
#include "SoundManager.h"
#include "buildings/nobUsual.h"
#include "world/GameWorld.h"
#include "gameData/GameConsts.h"
#include "gameData/JobConsts.h"

nofWorkman::nofWorkman(const Job job, const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofBuildingWorker(job, pos, player, workplace)
{}

nofWorkman::nofWorkman(SerializedGameData& sgd, const unsigned obj_id) : nofBuildingWorker(sgd, obj_id) {}

void nofWorkman::HandleDerivedEvent(const unsigned /*id*/)
{
    switch(state)
    {
        case State::Waiting1: HandleStateWaiting1(); break;
        case State::Work: HandleStateWork(); break;
        case State::Waiting2: HandleStateWaiting2(); break;
        default: break;
    }
}

bool nofWorkman::StartWorking()
{
    current_ev = GetEvMgr().AddEvent(this, JOB_CONSTS[job_].work_length, 1);
    state = State::Work;
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
        state = State::WaitingForWaresOrProductionStopped;
        workplace->StartNotWorking();
    }
}

void nofWorkman::HandleStateWaiting2()
{
    current_ev = nullptr;
    // Ware erzeugen... (noch nicht "richtig"!, sondern nur viruell erstmal)
    if(!(ware = ProduceWare()).has_value())
    {
        // Soll keine erzeugt werden --> wieder anfangen zu arbeiten
        TryToWork();
    } else
    {
        // und diese raustragen
        StartWalking(Direction::SouthEast);
        state = State::CarryoutWare;
    }

    // abgeleiteten Klassen Bescheid sagen
    WorkFinished();
}

void nofWorkman::HandleStateWork()
{
    // Nach Arbeiten wird noch ein bisschen gewartet, bevor das Produkt herausgetragen wird
    // Bei 0 mind. 1 GF
    current_ev = GetEvMgr().AddEvent(this, JOB_CONSTS[job_].wait2_length ? JOB_CONSTS[job_].wait2_length : 1, 1);
    state = State::Waiting2;
    // wir arbeiten nicht mehr
    workplace->is_working = false;

    // Evtl. Sounds löschen
    if(was_sounding)
    {
        world->GetSoundMgr().stopSounds(*this);
        was_sounding = false;
    }
}

namespace {
struct NodeHasResource
{
    const GameWorld& world;
    const ResourceType res;
    NodeHasResource(const GameWorld& world, const ResourceType res) : world(world), res(res) {}

    bool operator()(const MapPoint pt) { return world.GetNode(pt).resources.has(res); }
};
} // namespace

MapPoint nofWorkman::FindPointWithResource(ResourceType type) const
{
    // Alle Punkte durchgehen, bis man einen findet, wo man graben kann
    const std::vector<MapPoint> pts =
      world->GetMatchingPointsInRadius<1>(pos, MINER_RADIUS, NodeHasResource(*world, type), true);
    if(!pts.empty())
        return pts.front();

    workplace->OnOutOfResources();

    return MapPoint::Invalid();
}
