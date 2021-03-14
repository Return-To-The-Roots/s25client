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
    world->RemoveFigure(pos, this);
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
        GetEvMgr().AddToKillList(this);
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
                auto* figure = new nofPassiveWorker(job, pos, player, nullptr);
                // Auf die Map setzen
                world->AddFigure(pos, figure);
                // Losrumirren in die jeweilige Richtung
                figure->StartWandering(GetObjId());
                figure->StartWalking(dir);
            }
        }
    }

    // Nächste Runde
    ++go_out_phase;

    // Nächste Runde anmelden bzw. sich selbst killen, wenn alle Runden erledigt sind
    if(go_out_phase == GO_OUT_PHASES)
    {
        // fertig, sich selbst töten
        GetEvMgr().AddToKillList(this);
        // Prüfen, ob alle evakuiert wurden und keiner mehr an Board ist
        for(unsigned int it : people)
            RTTR_Assert(it == 0);
    } else
    {
        // Nächstes Event anmelden
        GetEvMgr().AddEvent(this, PHASE_LENGTH, 0);
    }
}
