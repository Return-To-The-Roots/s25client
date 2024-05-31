// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nofWorkman.h"

class SerializedGameData;
class nobUsual;

class nofVintner : public nofWorkman
{
    /// Zeichnet ihn beim Arbeiten
    void DrawWorking(DrawPoint drawPt) override;
    /// Id in jobs.bob or carrier.bob when carrying a ware
    unsigned short GetCarryID() const override;
    /// Der Arbeiter erzeugt eine Ware
    helpers::OptionalEnum<GoodType> ProduceWare() override;

    /// Draws the figure while returning home / entering the building (often carrying wares)
    void DrawWalkingWithWare(DrawPoint drawPt) override;

public:
    nofVintner(MapPoint pos, unsigned char player, nobUsual* workplace);
    nofVintner(SerializedGameData& sgd, unsigned obj_id);

    GO_Type GetGOT() const final { return GO_Type::NofVintner; }
};
