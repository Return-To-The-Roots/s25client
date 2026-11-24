// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofFlagWorker.h"
#include "FindWhConditions.h"
#include "GamePlayer.h"
#include "SerializedGameData.h"
#include "buildings/nobBaseWarehouse.h"
#include "world/GameWorld.h"
#include "nodeObjs/noFlag.h"

nofFlagWorker::nofFlagWorker(const Job job, const MapPoint pos, const unsigned char player, noRoadNode* goal)
    : noFigure(job, pos, player, goal), flag(nullptr), state(State::FigureWork)
{
    // Flagge als Ziel, dann arbeiten wir auch, ansonsten kanns aber auch nur ein Lagerhaus oder Null sein, wenn ein
    // Lagerhaus abgerissen wurde oder ausgelagert wurde etc., dann auch den nicht als Flag-Worker registrieren
    if(goal)
    {
        if(goal->getGOT() == GO_Type::Flag)
        {
            this->flag = static_cast<noFlag*>(goal);
            world->getPlayer(player).registerFlagWorker(this);
        } else
            this->flag = nullptr;
    } else
        this->flag = nullptr;
}

nofFlagWorker::nofFlagWorker(SerializedGameData& sgd, const unsigned obj_id)
    : noFigure(sgd, obj_id), flag(sgd.popObject<noFlag>(GO_Type::Flag)), state(sgd.pop<State>())
{}

void nofFlagWorker::serialize(SerializedGameData& sgd) const
{
    noFigure::serialize(sgd);

    sgd.pushObject(flag, true);
    sgd.pushEnum<uint8_t>(state);
}

void nofFlagWorker::destroy()
{
    RTTR_Assert(!flag);
    RTTR_Assert(!world->getPlayer(player).isFlagWorker(this));
    noFigure::destroy();
}

void nofFlagWorker::abrogateWorkplace()
{
    if(flag)
    {
        /// uns entfernen, da wir wieder umdrehen müssen
        world->getPlayer(player).removeFlagWorker(this);
        flag = nullptr;
    }
    // If flag is already null we might still be registered while GamePlayer::flagDestroyed iterates its list.
    // That callback removes us after lostWork() returns, so there is nothing to do here.
}

void nofFlagWorker::abrogate()
{
    state = State::FigureWork;
    noFigure::abrogate();
    RTTR_Assert(!flag);
}

/// Geht wieder zurück zur Flagge und dann nach Hause
void nofFlagWorker::goToFlag()
{
    // Zur Flagge zurücklaufen

    // Bin ich an der Fahne?
    if(pos == flag->getPos())
    {
        noFlag* const currentFlag = flag;
        // Detach once before goHome()/startWandering(): both may run abrogate() which must not remove us again.
        world->getPlayer(player).removeFlagWorker(this);
        state = State::FigureWork;
        flag = nullptr;

        // nach Hause gehen
        nobBaseWarehouse* wh =
          world->getPlayer(player).findWarehouse(*currentFlag, FW::AcceptsFigure(job_), true, false);
        if(wh)
        {
            goHome(wh);
            // Vorgaukeln, dass wir ein Stück Straße bereits geschafft haben
            // damit wir mit walkToGoal weiter bis zum Ziel laufen können
            cur_rs = &emulated_wanderroad;
            rs_pos = 0;
            walkToGoal();
        } else
        {
            // Weg führt nicht mehr zum Lagerhaus, dann rumirren
            startWandering();
        }
    } else
    {
        // Weg suchen
        const auto dir = world->findHumanPath(pos, flag->getPos(), 40);

        // Wenns keinen gibt, rumirren, ansonsten hinlaufen
        if(dir)
            startWalking(*dir);
        else
            startWandering();
    }
}
