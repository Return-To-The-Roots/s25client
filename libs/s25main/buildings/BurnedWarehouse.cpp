// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "BurnedWarehouse.h"

#include "EventManager.h"
#include "GamePlayer.h"
#include "SerializedGameData.h"
#include "figures/nofPassiveWorker.h"
#include "pathfinding/PathConditionHuman.h"
#include "random/Random.h"
#include "world/GameWorld.h"

/// Anzahl der Rausgeh-Etappen
const unsigned GO_OUT_PHASES = 10;
/// Länge zwischen zwei solchen Phasen
const unsigned PHASE_LENGTH = 2;

BurnedWarehouse::BurnedWarehouse(const MapPoint pos, const unsigned char player, const PeopleArray& people)
    : noCoordBase(NodalObjectType::BurnedWarehouse, pos), player(player), go_out_phase(0), people(people)
{
    // Erstes Event anmelden
    GetEvMgr().AddEvent(this, PHASE_LENGTH, 0);
}

BurnedWarehouse::BurnedWarehouse(SerializedGameData& sgd, const unsigned obj_id)
    : noCoordBase(sgd, obj_id), player(sgd.PopUnsignedChar()), go_out_phase(sgd.PopUnsignedInt())
{
    helpers::popContainer(sgd, people);
}

BurnedWarehouse::~BurnedWarehouse() = default;

void BurnedWarehouse::Destroy()
{
    noCoordBase::Destroy();
}

void BurnedWarehouse::Serialize(SerializedGameData& sgd) const
{
    noCoordBase::Serialize(sgd);

    sgd.PushUnsignedChar(player);
    sgd.PushUnsignedInt(go_out_phase);
    helpers::pushContainer(sgd, people);
}

void BurnedWarehouse::HandleEvent(const unsigned /*id*/)
{
    RTTR_Assert(go_out_phase != GO_OUT_PHASES);

    std::array<Direction, helpers::NumEnumValues_v<Direction>> possibleDirs;
    unsigned possibleDirCt = 0;

    // Mögliche Richtungen zählen und speichern
    PathConditionHuman pathChecker(*world);
    for(const auto dir : helpers::EnumRange<Direction>{})
    {
        if(pathChecker.IsNodeOk(world->GetNeighbour(pos, dir)))
            possibleDirs[possibleDirCt++] = dir;
    }

    // GAR KEINE Richtungen?
    if(possibleDirCt == 0)
    {
        // Das ist traurig, dann muss die Titanic mit allen restlichen an Board leider untergehen
        GetEvMgr().AddToKillList(world->RemoveFigure(pos, *this));
        // restliche Leute von der Inventur abziehen
        for(const auto i : helpers::enumRange<Job>())
            world->GetPlayer(player).DecreaseInventoryJob(i, people[i]);

        return;
    }

    for(const auto job : helpers::enumRange<Job>())
    {
        // Anzahl ausrechnen, die in dieser Runde rausgeht
        unsigned count;
        if(go_out_phase + 1 >= GO_OUT_PHASES)
            count = people[job]; // Take all on last round
        else
            count = people[job] / (GO_OUT_PHASES - go_out_phase);

        // Von der vorhandenen Abzahl abziehen
        people[job] -= count;

        // In Alle Richtungen verteilen
        // Startrichtung zufällig bestimmen
        unsigned start_dir = RANDOM_RAND(helpers::NumEnumValues_v<Direction>);

        for(unsigned j = 0; j < possibleDirCt; ++j)
        {
            // Aktuelle Richtung, die jetzt dran ist bestimmen
            Direction dir = possibleDirs[j] + start_dir;

            // Anzahl jetzt für diese Richtung ausrechnen
            unsigned numPeopleInDir = count / possibleDirCt;
            // Bei letzter Richtung noch den übriggebliebenen Rest dazuaddieren
            if(j + 1 == possibleDirCt)
                numPeopleInDir += count % possibleDirCt;

            // Die Figuren schließlich rausschicken
            for(unsigned z = 0; z < numPeopleInDir; ++z)
            {
                // Job erzeugen
                auto& figure = world->AddFigure(pos, std::make_unique<nofPassiveWorker>(job, pos, player, nullptr));
                // Losrumirren in die jeweilige Richtung
                figure.StartWandering(GetObjId());
                figure.StartWalking(dir);
            }
        }
    }

    // Nächste Runde
    ++go_out_phase;

    // Nächste Runde anmelden bzw. sich selbst killen, wenn alle Runden erledigt sind
    if(go_out_phase == GO_OUT_PHASES)
    {
        // fertig, sich selbst töten
        GetEvMgr().AddToKillList(world->RemoveFigure(pos, *this));
        // Prüfen, ob alle evakuiert wurden und keiner mehr an Board ist
        for(unsigned int it : people)
            RTTR_Assert(it == 0);
    } else
    {
        // Nächstes Event anmelden
        GetEvMgr().AddEvent(this, PHASE_LENGTH, 0);
    }
}
