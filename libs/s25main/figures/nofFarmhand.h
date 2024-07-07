// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nofBuildingWorker.h"
class SerializedGameData;
class nobUsual;

/// Worker that walks out of his building and works at some point
class nofFarmhand : public nofBuildingWorker
{
public:
    enum class PointQuality
    {
        NotPossible, /// Work is not possible at this position
        Class1,      /// Work is possible, points are preferred to other points
        Class2,      /// Work is possible, points are preferred to other points class 2
        Class3       /// Work is possible, points are only chosen if there are no other class 1/2's
    };

protected:
    /// Point the worker is going to work at
    MapPoint dest;

    friend constexpr auto maxEnumValue(PointQuality) { return PointQuality::Class3; }

    /// Called by base class (noFigure) after having walked to new point
    void WalkedDerived() override;

    /// Stop work as workplace becomes unavailable
    void WorkAborted() override;

    /// Start walking to work point
    void WalkToWorkpoint();
    /// Get ready to work back to workplace
    void StartWalkingHome();
    /// Do next step for walking home
    void WalkHome();

    /// Inform derived class about the start of the whole working process
    /// (at the beginning when walking out of the house)
    virtual void WalkingStarted();
    /// Inform derived class when starting work at the work point
    virtual void WorkStarted() = 0;
    /// Inform derived class when work at work point is done
    virtual void WorkFinished() = 0;

    /// Draw in other work-related states
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
