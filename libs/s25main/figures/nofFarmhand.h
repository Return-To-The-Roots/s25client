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

/// Ein Landarbeiter geht raus aus seiner Hütte und arbeitet in "freier Natur"
class nofFarmhand : public nofBuildingWorker
{
protected:
    /// Arbeitsziel, das der Arbeiter ansteuert
    MapPoint dest;

    enum PointQuality
    {
        PQ_NOTPOSSIBLE, // Work is not possible at this position
        PQ_CLASS1,      /// Work is possible, points are prefered to other points
        PQ_CLASS2,      /// Work is possible, points are prefered to other points class 2
        PQ_CLASS3       /// Work is possible, points are only chosen if there are no other class 1/2's
    };

protected:
    /// Funktionen, die nur von der Basisklasse (noFigure) aufgerufen werden, wenn...
    void WalkedDerived() override;

    /// Arbeit musste wegen Arbeitsplatzverlust abgebrochen werden
    void WorkAborted() override;

    /// Läuft zum Arbeitspunkt
    void WalkToWorkpoint();
    /// Trifft Vorbereitungen fürs nach Hause - Laufen
    void StartWalkingHome();
    /// Läuft wieder zu seiner Hütte zurück
    void WalkHome();

    /// Inform derived class about the start of the whole working process (at the beginning when walking out of the
    /// house)
    virtual void WalkingStarted();
    /// Abgeleitete Klasse informieren, wenn sie anfängt zu arbeiten (Vorbereitungen)
    virtual void WorkStarted() = 0;
    /// Abgeleitete Klasse informieren, wenn fertig ist mit Arbeiten
    virtual void WorkFinished() = 0;

    /// Zeichnen der Figur in sonstigen Arbeitslagen
    void DrawOtherStates(DrawPoint drawPt) override;

public:
    nofFarmhand(Job job, MapPoint pos, unsigned char player, nobUsual* workplace);
    nofFarmhand(SerializedGameData& sgd, unsigned obj_id);

    /// Aufräummethoden
protected:
    void Destroy_nofFarmhand() { Destroy_nofBuildingWorker(); }

public:
    void Destroy() override { Destroy_nofFarmhand(); }

    /// Serialisierungsfunktionen
protected:
    void Serialize_nofFarmhand(SerializedGameData& sgd) const;

public:
    void Serialize(SerializedGameData& sgd) const override { Serialize_nofFarmhand(sgd); }

    void HandleDerivedEvent(unsigned id) override;
    /// Findet heraus, ob der Beruf an diesem Punkt arbeiten kann
    bool IsPointAvailable(MapPoint pt) const;
    /// Returns the quality of this working point or determines if the worker can work here at all
    virtual PointQuality GetPointQuality(MapPoint pt) const = 0;
};
