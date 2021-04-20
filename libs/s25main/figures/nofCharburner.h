// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nofFarmhand.h"
#include <cstdint>

class SerializedGameData;
class nobUsual;

#ifdef _MSC_VER
#    pragma warning(disable : 4646) // function declared with [[noreturn]] has non-void return type
#endif

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
    /// Malt den Arbeiter beim Arbeiten
    void DrawWorking(DrawPoint drawPt) override;
    /// Id in jobs.bob or carrier.bob when carrying a ware
    [[noreturn]] unsigned short GetCarryID() const override;

    /// Abgeleitete Klasse informieren, wenn sie anfängt zu arbeiten (Vorbereitungen)
    void WorkStarted() override;
    /// Abgeleitete Klasse informieren, wenn fertig ist mit Arbeiten
    void WorkFinished() override;

    /// Returns the quality of this working point or determines if the worker can work here at all
    PointQuality GetPointQuality(MapPoint pt) const override;

    /// Inform derived class about the start of the whole working process (at the beginning when walking out of the
    /// house)
    void WalkingStarted() override;

    /// Draws the figure while returning home / entering the building (often carrying wares)
    void DrawWalkingWithWare(DrawPoint drawPt) override;
    /// Draws the charburner while walking
    /// (overriding standard method of nofFarmhand)
    void DrawOtherStates(DrawPoint drawPt) override;

protected:
    bool AreWaresAvailable() const override;

public:
    nofCharburner(MapPoint pos, unsigned char player, nobUsual* workplace);
    nofCharburner(SerializedGameData& sgd, unsigned obj_id);

    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const final { return GO_Type::NofCharburner; }
};
