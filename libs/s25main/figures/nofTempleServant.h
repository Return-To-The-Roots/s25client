// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nofWorkman.h"

class SerializedGameData;
class nobUsual;

#ifdef _MSC_VER
#    pragma warning(disable : 4646) // function declared with [[noreturn]] has non-void return type
#endif

class nofTempleServant : public nofWorkman
{
    /// Draw worker at work
    void DrawWorking(DrawPoint drawPt) override;
    [[noreturn]] unsigned short GetCarryID() const override;
    /// The worker produces a ware
    helpers::OptionalEnum<GoodType> ProduceWare() override;

    /// Draws the figure while returning home / entering the building (often carrying wares)
    void DrawWalkingWithWare(DrawPoint drawPt) override;
    void DrawWalking(DrawPoint drawPt) override;

    GoodType currentProduction;

public:
    nofTempleServant(MapPoint pos, unsigned char player, nobUsual* workplace);
    nofTempleServant(SerializedGameData& sgd, unsigned obj_id);
    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const final { return GO_Type::NofTempleservant; }
};
