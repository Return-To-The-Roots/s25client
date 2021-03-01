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

    GO_Type GetGOT() const override { return GO_Type::NofCatapultman; }

    void HandleDerivedEvent(unsigned id) override;

    /// wird aufgerufen, wenn die Arbeit abgebrochen wird (von nofBuildingWorker aufgerufen)
    void WorkAborted() override;
};
