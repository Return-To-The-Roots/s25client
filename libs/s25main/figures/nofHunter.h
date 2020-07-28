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

#ifndef NOF_HUNTER_H_
#define NOF_HUNTER_H_

#include "nofBuildingWorker.h"
#include "gameTypes/Direction.h"

class noAnimal;
class SerializedGameData;
class nobUsual;

/// Klasse für den Jäger, der Tiere jagt und Nahrung produziert
class nofHunter : public nofBuildingWorker
{
private:
    /// Tier, das gejagt wird
    noAnimal* animal;
    /// Punkt, von dem aus geschossen wird
    MapPoint shootingPos;
    /// Richtung, in die geschossen wird
    Direction shooting_dir;

private:
    /// Funktionen, die nur von der Basisklasse (noFigure) aufgerufen werden, wenn man gelaufen ist
    void WalkedDerived() override;
    /// Malt den Arbeiter beim Arbeiten
    void DrawWorking(DrawPoint drawPt) override;
    /// Id in jobs.bob or carrier.bob when carrying a ware
    unsigned short GetCarryID() const override { return 89; }

    /// Trifft Vorbereitungen fürs nach Hause - Laufen
    void StartWalkingHome();
    /// Läuft wieder zu seiner Hütte zurück
    void HandleStateWalkingHome();

    /// Prüft, ob der Schießpunkt geeignet ist
    bool IsShootingPointGood(MapPoint pt);

    /// Wenn jeweils gelaufen wurde oder ein Event abgelaufen ist, je nach aktuellem Status folgende Funktionen ausführen
    void HandleStateChasing();
    void HandleStateFindingShootingPoint();
    void HandleStateWaitingForAnimalReady();
    void HandleStateShooting();
    void HandleStateWalkingToCadaver();
    void HandleStateEviscerating();

public:
    nofHunter(MapPoint pos, unsigned char player, nobUsual* workplace);
    nofHunter(SerializedGameData& sgd, unsigned obj_id);

    void Destroy() override
    {
        RTTR_Assert(!animal);
        nofBuildingWorker::Destroy();
    }

    /// Serialisierungsfunktionen
protected:
    void Serialize_nofHunter(SerializedGameData& sgd) const;

public:
    void Serialize(SerializedGameData& sgd) const override { Serialize_nofHunter(sgd); }

    GO_Type GetGOT() const override { return GOT_NOF_HUNTER; }

    void HandleDerivedEvent(unsigned id) override;

    void TryStartHunting();

    /// das Tier ist nicht mehr verfügbar (von selbst gestorben o.Ä.)
    void AnimalLost();
    /// wird aufgerufen, wenn die Arbeit abgebrochen wird (von nofBuildingWorker aufgerufen)
    void WorkAborted() override;
};

#endif
