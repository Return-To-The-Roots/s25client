// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nofBuildingWorker.h"
class GlobalGameSettings;
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
    void walkedDerived() override;

    /// Stop work as workplace becomes unavailable
    void workAborted() override;

    /// Start walking to work point
    void walkToWorkpoint();
    /// Get ready to work back to workplace
    void startWalkingHome();
    /// Do next step for walking home
    void walkHome();

    /// Inform derived class about the start of the whole working process
    /// (at the beginning when walking out of the house)
    virtual void walkingStarted();
    /// Inform derived class when starting work at the work point
    virtual void workStarted() = 0;
    /// Inform derived class when work at work point is done
    virtual void workFinished() = 0;

    /// Draw in other work-related states
    void drawOtherStates(DrawPoint drawPt) override;

public:
    nofFarmhand(Job job, MapPoint pos, unsigned char player, nobUsual* workplace);
    nofFarmhand(SerializedGameData& sgd, unsigned obj_id);

    static unsigned getWorkRadius(const GlobalGameSettings& ggs, Job job);
    unsigned getWorkRadius(Job job);

    void Serialize(SerializedGameData& sgd) const override;

    void handleDerivedEvent(unsigned id) override;
    /// Return the quality of this working point (including if the worker can work here at all)
    /// isBeforeWork determines whether the evaluation happens before leaving for work or afterwards
    virtual PointQuality getPointQuality(MapPoint pt, bool isBeforeWork) const = 0;
    PointQuality getPointQuality(MapPoint pt) const { return getPointQuality(pt, false); }
};
