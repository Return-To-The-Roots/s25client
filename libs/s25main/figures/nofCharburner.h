// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nofFarmhand.h"
#include <cstdint>

class SerializedGameData;
class nobUsual;

class nofCharburner : public nofFarmhand
{
    /// Is he harvesting a charburner pile (or planting?)
    bool harvest;
    /// If stacking wood pile: Determines which ware he carries (wood or grain?)
    enum class WareType : uint8_t
    {
        Wood,
        Grain
    } wt;
    friend constexpr auto maxEnumValue(WareType) { return WareType::Grain; }

private:
    void drawWorking(DrawPoint drawPt) override;
    /// Has no ID in jobs.bob or carrier.bob
    [[noreturn]] unsigned short getCarryID() const override;

    void workStarted() override;
    void workFinished() override;

    /// Returns the quality of this working point or determines if the worker can work here at all
    PointQuality getPointQuality(MapPoint pt, bool isBeforeWork) const override;
    using nofFarmhand::getPointQuality;

    /// Inform derived class about the start of the whole working process (at the beginning when walking out of the
    /// house)
    void walkingStarted() override;

    /// Draws the figure while returning home / entering the building (often carrying wares)
    void drawWalkingWithWare(DrawPoint drawPt) override;
    /// Draws the charburner while walking
    /// (overriding standard method of nofFarmhand)
    void drawOtherStates(DrawPoint drawPt) override;

protected:
    bool areWaresAvailable() const override;

public:
    nofCharburner(MapPoint pos, unsigned char player, nobUsual* workplace);
    nofCharburner(SerializedGameData& sgd, unsigned obj_id);

    void serialize(SerializedGameData& sgd) const override;

    GO_Type getGOT() const final { return GO_Type::NofCharburner; }
};
