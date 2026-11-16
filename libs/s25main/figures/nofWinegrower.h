// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nofFarmhand.h"

class SerializedGameData;
class nobUsual;

class nofWinegrower : public nofFarmhand
{
    /// harvesting grapes or planting
    bool harvest;

private:
    /// Draw worker at work
    void drawWorking(DrawPoint drawPt) override;
    /// Id in jobs.bob or carrier.bob when carrying a ware
    [[noreturn]] unsigned short getCarryID() const override;

    /// Inform derived class, when working starts (preparing)
    void workStarted() override;
    /// Inform derived class, when work is finished
    void workFinished() override;
    /// Inform derived class, when work was aborted
    void workAborted() override;

    /// Returns the quality of this working point or determines if the worker can work here at all
    PointQuality getPointQuality(MapPoint pt, bool isBeforeWork) const override;
    using nofFarmhand::getPointQuality;

    /// Inform derived class about the start of the whole working process (at the beginning when walking out of the
    /// house)
    void walkingStarted() override;

    /// Draws the figure while returning home / entering the building (often carrying wares)
    void drawWalkingWithWare(DrawPoint drawPt) override;
    /// Draws the vintner while walking
    void drawOtherStates(DrawPoint drawPt) override;

    bool areWaresAvailable() const override;

public:
    nofWinegrower(MapPoint pos, unsigned char player, nobUsual* workplace);
    nofWinegrower(SerializedGameData& sgd, unsigned obj_id);

    void serialize(SerializedGameData& sgd) const override;

    GO_Type getGOT() const final { return GO_Type::NofWinegrower; }
};
