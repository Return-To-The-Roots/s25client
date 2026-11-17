// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nofBuildingWorker.h"
#include "gameTypes/Direction.h"
#include <vector>

class GameWorldBase;
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
    void walkedDerived() override;
    /// Malt den Arbeiter beim Arbeiten
    void drawWorking(DrawPoint drawPt) override;
    /// Id in jobs.bob or carrier.bob when carrying a ware
    unsigned short getCarryID() const override { return 89; }

    /// Trifft Vorbereitungen fürs nach Hause - Laufen
    void startWalkingHome();
    /// Läuft wieder zu seiner Hütte zurück
    void handleStateWalkingHome();

    /// Prüft, ob der Schießpunkt geeignet ist
    bool isShootingPointGood(MapPoint pt);

    /// Wenn jeweils gelaufen wurde oder ein Event abgelaufen ist, je nach aktuellem Status folgende Funktionen
    /// ausführen
    void handleStateChasing();
    void handleStateFindingShootingPoint();
    void handleStateWaitingForAnimalReady();
    void handleStateShooting();
    void handleStateWalkingToCadaver();
    void handleStateEviscerating();

public:
    nofHunter(MapPoint pos, unsigned char player, nobUsual* workplace);
    nofHunter(SerializedGameData& sgd, unsigned obj_id);

    void destroy() override
    {
        RTTR_Assert(!animal);
        nofBuildingWorker::destroy();
    }

    void serialize(SerializedGameData& sgd) const override;

    GO_Type getGOT() const final { return GO_Type::NofHunter; }

    void handleDerivedEvent(unsigned id) override;

    void tryStartHunting();

    /// Find animals in a radius around a position that satisfy a predicate and are reachable
    static std::vector<noAnimal*> getAnimalsInRange(const GameWorldBase& world, MapPoint pos, unsigned radius,
                                                    unsigned maxDistance, bool (*isValidAnimal)(const noAnimal*));
    static std::vector<noAnimal*> getHuntableAnimalsInRange(const GameWorldBase& world, MapPoint pos);

    /// das Tier ist nicht mehr verfügbar (von selbst gestorben o.Ä.)
    void animalLost();
    /// wird aufgerufen, wenn die Arbeit abgebrochen wird (von nofBuildingWorker aufgerufen)
    void workAborted() override;
};
