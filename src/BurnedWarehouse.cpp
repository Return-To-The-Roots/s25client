// $Id: BurnedWarehouse.cpp 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "main.h"
#include "BurnedWarehouse.h"

#include "SerializedGameData.h"
#include "EventManager.h"
#include "GameWorld.h"
#include "Random.h"
#include "noFigure.h"
#include "glArchivItem_Map.h"
#include "GameClient.h"
#include "GameClientPlayer.h"
#include "nofPassiveWorker.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/// Anzahl der Rausgeh-Etappen
const unsigned GO_OUT_PHASES = 10;
/// Länge zwischen zwei solchen Phasen
const unsigned PHASE_LENGTH = 2;

BurnedWarehouse::BurnedWarehouse(const unsigned short x, const unsigned short y, const unsigned char player, const unsigned* people)
    : noCoordBase(NOP_BURNEDWAREHOUSE, x, y), player(player), go_out_phase(0)
{
    memcpy(this->people, people, 30 * sizeof(unsigned));
    // Erstes Event anmelden
    em->AddEvent(this, PHASE_LENGTH, 0);
}

BurnedWarehouse::BurnedWarehouse(SerializedGameData* sgd, const unsigned obj_id) : noCoordBase(sgd, obj_id),
    player(sgd->PopUnsignedChar()),
    go_out_phase(sgd->PopUnsignedInt())
{
    for(unsigned i = 0; i < 31; ++i)
        people[i] = sgd->PopUnsignedInt();
}


BurnedWarehouse::~BurnedWarehouse()
{
}


void BurnedWarehouse::Destroy(void)
{
}


void BurnedWarehouse::Serialize_BurnedWarehouse(SerializedGameData* sgd) const
{
    Serialize_noCoordBase(sgd);

    sgd->PushUnsignedChar(player);
    sgd->PushUnsignedInt(go_out_phase);

    for(unsigned i = 0; i < 31; ++i)
        sgd->PushUnsignedInt(people[i]);
}


void BurnedWarehouse::HandleEvent(const unsigned int id)
{
    for(unsigned i = 0; i < 30; ++i)
    {
        // Anzahl ausrechnen, die in dieser Runde rausgeht
        unsigned count = people[i] / (GO_OUT_PHASES - go_out_phase);

        // Letzte Runde? Dann den Rest auch noch mit nehmen
        if(go_out_phase == GO_OUT_PHASES)
            count += people[i];

        // Von der vorhandenen Abzahl abziehen
        people[i] -= count;

        // In Alle Richtungen verteilen
        // Startrichtung zufällig bestimmen
        unsigned char start_dir = Random::inst().Rand(__FILE__, __LINE__, obj_id, 6);

        bool possible[6];
        unsigned possible_count = 0;

        // Mögliche Richtungen zählen und speichern
        for(unsigned char d = 0; d < 6; ++d)
        {
            if(gwg->IsNodeForFigures(gwg->GetXA(x, y, d),
                                     gwg->GetYA(x, y, d)))
            {
                possible[d] = true;
                ++possible_count;
            }
            else
                possible[d] = false;
        }

        // GAR KEINE Richtungen?
        if(possible_count == 0)
        {
            // Das ist traurig, dann muss die Titanic mit allen restlichen an Board leider untergehen
            em->AddToKillList(this);
            // restliche Leute von der Inventur abziehen
            for(unsigned int i = 0; i < 30; ++i)
                GAMECLIENT.GetPlayer(player)->DecreaseInventoryJob(Job(i), people[i]);

            return;
        }

        // Letzte mögliche Richtung bestimmen
        unsigned char last_dir = 0xFF;

        for(unsigned char d = 0; d < 6; ++d)
        {
            unsigned char dir = (start_dir + d) % 6;
            if(possible[dir])
                last_dir = dir;
        }

        assert(last_dir < 6);

        for(unsigned char d = 0; d < 6; ++d)
        {
            // Aktuelle Richtung, die jetzt dran ist bestimmen
            unsigned dir = (start_dir + d) % 6;

            // Wenn Richtung nicht möglich ist --> weglassen
            if(!possible[dir])
                continue;

            // Anzahl jetzt für diese Richtung ausrechnen
            unsigned dir_count = count / possible_count;
            // Bei letzter Richtung noch den übriggebliebenen Rest dazuaddieren
            if(dir == last_dir)
                dir_count += count % possible_count;

            // Die Figuren schließlich rausschicken
            for(unsigned z = 0; z < dir_count; ++z)
            {
                // Job erzeugen
                nofPassiveWorker* figure = new nofPassiveWorker(Job(i), x, y, player, NULL);
                // Auf die Map setzen
                gwg->AddFigure(figure, x, y);
                // Losrumirren in die jeweilige Richtung
                figure->StartWandering(obj_id);
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
        em->AddToKillList(this);
#ifndef NDEBUG
        // Prüfen, ob alle evakuiert wurden und keiner mehr an Board ist
        for(unsigned i = 0; i < 30; ++i)
            assert(people[i] == 0);
#endif
    }
    else
    {
        // Nächstes Event anmelden
        em->AddEvent(this, PHASE_LENGTH, 0);
    }

}

