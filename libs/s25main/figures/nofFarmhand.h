// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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

    enum class PointQuality
    {
        NotPossible, // Work is not possible at this position
        Class1,      /// Work is possible, points are prefered to other points
        Class2,      /// Work is possible, points are prefered to other points class 2
        Class3       /// Work is possible, points are only chosen if there are no other class 1/2's
    };
    friend constexpr auto maxEnumValue(PointQuality) { return PointQuality::Class3; }

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

    void Serialize(SerializedGameData& sgd) const override;

    void HandleDerivedEvent(unsigned id) override;
    /// Findet heraus, ob der Beruf an diesem Punkt arbeiten kann
    bool IsPointAvailable(MapPoint pt) const;
    /// Returns the quality of this working point or determines if the worker can work here at all
    virtual PointQuality GetPointQuality(MapPoint pt) const = 0;
};
