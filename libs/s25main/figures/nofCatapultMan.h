// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nofBuildingWorker.h"
class SerializedGameData;
class nobUsual;

/// Arbeiter im Katapult
class nofCatapultMan : public nofBuildingWorker
{
    /// Drehschritte für den Katapult auf dem Dach, bis er die Angriffsrichtung erreicht hat
    /// negativ - andere Richtung!
    int wheel_steps;

    /// Ein mögliches Ziel für den Katapult
    class PossibleTarget
    {
    public:
        /// Gebäude
        MapPoint pos;
        /// Entfernung
        unsigned distance;

        PossibleTarget() : pos(0, 0), distance(0) {}
        PossibleTarget(const MapPoint pt, unsigned distance) : pos(pt), distance(distance) {}
        PossibleTarget(SerializedGameData& sgd);

        void Serialize(SerializedGameData& sgd) const;

    } target; /// das anvisierte Ziel

private:
    /// Funktionen, die nur von der Basisklasse (noFigure) aufgerufen werden, wenn man gelaufen ist
    void WalkedDerived() override;
    /// Malt den Arbeiter beim Arbeiten
    void DrawWorking(DrawPoint drawPt) override;
    /// Id in jobs.bob or carrier.bob when carrying a ware
    unsigned short GetCarryID() const override { return 0; }

public:
    nofCatapultMan(MapPoint pos, unsigned char player, nobUsual* workplace);
    nofCatapultMan(SerializedGameData& sgd, unsigned obj_id);

    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const final { return GO_Type::NofCatapultman; }

    void HandleDerivedEvent(unsigned id) override;

    /// wird aufgerufen, wenn die Arbeit abgebrochen wird (von nofBuildingWorker aufgerufen)
    void WorkAborted() override;
};
