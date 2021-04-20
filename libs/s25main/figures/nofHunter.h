// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

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

    /// Wenn jeweils gelaufen wurde oder ein Event abgelaufen ist, je nach aktuellem Status folgende Funktionen
    /// ausführen
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

    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const final { return GO_Type::NofHunter; }

    void HandleDerivedEvent(unsigned id) override;

    void TryStartHunting();

    /// das Tier ist nicht mehr verfügbar (von selbst gestorben o.Ä.)
    void AnimalLost();
    /// wird aufgerufen, wenn die Arbeit abgebrochen wird (von nofBuildingWorker aufgerufen)
    void WorkAborted() override;
};
