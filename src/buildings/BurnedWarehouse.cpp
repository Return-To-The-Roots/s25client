// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#include "defines.h" // IWYU pragma: keep
#include "BurnedWarehouse.h"

#include "SerializedGameData.h"
#include "EventManager.h"
#include "Random.h"
#include "GameClient.h"
#include "GamePlayer.h"
#include "world/GameWorldGame.h"
#include "figures/nofPassiveWorker.h"

/// Anzahl der Rausgeh-Etappen
const unsigned GO_OUT_PHASES = 10;
/// Länge zwischen zwei solchen Phasen
const unsigned PHASE_LENGTH = 2;

BurnedWarehouse::BurnedWarehouse(const MapPoint pos, const unsigned char player, const PeopleArray& people)
    : noCoordBase(NOP_BURNEDWAREHOUSE, pos), player(player), go_out_phase(0), people(people)
{
    // Erstes Event anmelden
    GetEvMgr().AddEvent(this, PHASE_LENGTH, 0);
}

BurnedWarehouse::BurnedWarehouse(SerializedGameData& sgd, const unsigned obj_id) : noCoordBase(sgd, obj_id),
    player(sgd.PopUnsignedChar()),
    go_out_phase(sgd.PopUnsignedInt())
{
    for(PeopleArray::iterator it = people.begin(); it != people.end(); ++it)
        *it = sgd.PopUnsignedInt();
}


BurnedWarehouse::~BurnedWarehouse()
{
}


void BurnedWarehouse::Destroy()
{
}


void BurnedWarehouse::Serialize_BurnedWarehouse(SerializedGameData& sgd) const
{
    Serialize_noCoordBase(sgd);

    sgd.PushUnsignedChar(player);
    sgd.PushUnsignedInt(go_out_phase);

    for(PeopleArray::const_iterator it = people.begin(); it != people.end(); ++it)
        sgd.PushUnsignedInt(*it);
}


void BurnedWarehouse::HandleEvent(const unsigned int  /*id*/)
{
    RTTR_Assert(go_out_phase != GO_OUT_PHASES);

    bool dirIsPossible[6];
    unsigned possibleDirCt = 0;

    // Mögliche Richtungen zählen und speichern
    for(unsigned char d = 0; d < 6; ++d)
    {
        if(gwg->IsNodeForFigures(gwg->GetNeighbour(pos, d)))
        {
            dirIsPossible[d] = true;
            ++possibleDirCt;
        }
        else
            dirIsPossible[d] = false;
    }

    // GAR KEINE Richtungen?
    if(possibleDirCt == 0)
    {
        // Das ist traurig, dann muss die Titanic mit allen restlichen an Board leider untergehen
        GetEvMgr().AddToKillList(this);
        // restliche Leute von der Inventur abziehen
        for(unsigned int i = 0; i < people.size(); ++i)
            gwg->GetPlayer(player).DecreaseInventoryJob(Job(i), people[i]);

        return;
    }

    for(unsigned int i = 0; i < people.size(); ++i)
    {
        // Anzahl ausrechnen, die in dieser Runde rausgeht
        unsigned count;
        if (go_out_phase + 1 >= GO_OUT_PHASES)
            count = people[i]; // Take all on last round
        else
            count = people[i] / (GO_OUT_PHASES - go_out_phase);

        // Von der vorhandenen Abzahl abziehen
        people[i] -= count;

        // In Alle Richtungen verteilen
        // Startrichtung zufällig bestimmen
        unsigned char start_dir = RANDOM.Rand(__FILE__, __LINE__, GetObjId(), 6);

        // Letzte mögliche Richtung bestimmen
        unsigned char last_dir = 0xFF;

        for(unsigned char d = 0; d < 6; ++d)
        {
            unsigned char dir = (start_dir + d) % 6;
            if(dirIsPossible[dir])
                last_dir = dir;
        }

        RTTR_Assert(last_dir < 6);

        for(unsigned char d = 0; d < 6; ++d)
        {
            // Aktuelle Richtung, die jetzt dran ist bestimmen
            unsigned dir = (start_dir + d) % 6;

            // Wenn Richtung nicht möglich ist --> weglassen
            if(!dirIsPossible[dir])
                continue;

            // Anzahl jetzt für diese Richtung ausrechnen
            unsigned numPeopleInDir = count / possibleDirCt;
            // Bei letzter Richtung noch den übriggebliebenen Rest dazuaddieren
            if(dir == last_dir)
                numPeopleInDir += count % possibleDirCt;

            // Die Figuren schließlich rausschicken
            for(unsigned z = 0; z < numPeopleInDir; ++z)
            {
                // Job erzeugen
                nofPassiveWorker* figure = new nofPassiveWorker(Job(i), pos, player, NULL);
                // Auf die Map setzen
                gwg->AddFigure(figure, pos);
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
        for(PeopleArray::const_iterator it = people.begin(); it != people.end(); ++it)
            RTTR_Assert(*it == 0);
    }
    else
    {
        // Nächstes Event anmelden
        GetEvMgr().AddEvent(this, PHASE_LENGTH, 0);
    }

}

