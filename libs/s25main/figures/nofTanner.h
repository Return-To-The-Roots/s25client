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

class nofTanner : public nofWorkman
{
    /// Draw worker at work
    void DrawWorking(DrawPoint drawPt) override;
    /// Id in jobs.bob or carrier.bob when carrying a ware
    [[noreturn]] unsigned short GetCarryID() const override;
    /// Der Arbeiter erzeugt eine Ware
    helpers::OptionalEnum<GoodType> ProduceWare() override;

    /// Draws the figure while returning home / entering the building (often carrying wares)
    void DrawWalkingWithWare(DrawPoint drawPt) override;

public:
    nofTanner(MapPoint pos, unsigned char player, nobUsual* workplace);
    nofTanner(SerializedGameData& sgd, unsigned obj_id);

    GO_Type GetGOT() const final { return GO_Type::NofTanner; }
};
