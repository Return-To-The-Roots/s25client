// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nofFarmhand.h"

class SerializedGameData;
class nobUsual;

#ifdef _MSC_VER
#    pragma warning(disable : 4646) // function declared with [[noreturn]] has non-void return type
#endif

class nofWinegrower : public nofFarmhand
{
    /// Is he harvesting grapes (or planting?)
    bool harvest;

private:
    /// Draw worker at work
    void DrawWorking(DrawPoint drawPt) override;
    /// Id in jobs.bob or carrier.bob when carrying a ware
    [[noreturn]] unsigned short GetCarryID() const override;

    /// Inform derived class, when working starts (preparing)
    void WorkStarted() override;
    /// Inform derived class, when work is finished
    void WorkFinished() override;
    /// Inform derived class, when work was aborted
    void WorkAborted() override;

    /// Returns the quality of this working point or determines if the worker can work here at all
    PointQuality GetPointQuality(MapPoint pt) const override;

    /// Inform derived class about the start of the whole working process (at the beginning when walking out of the
    /// house)
    void WalkingStarted() override;

    /// Draws the figure while returning home / entering the building (often carrying wares)
    void DrawWalkingWithWare(DrawPoint drawPt) override;
    /// Draws the vintner while walking
    /// (overriding standard method of nofFarmhand)
    void DrawOtherStates(DrawPoint drawPt) override;

public:
    nofWinegrower(MapPoint pos, unsigned char player, nobUsual* workplace);
    nofWinegrower(SerializedGameData& sgd, unsigned obj_id);

    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const final { return GO_Type::NofWinegrower; }
};
