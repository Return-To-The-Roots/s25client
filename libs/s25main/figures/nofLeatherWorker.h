// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nofWorkman.h"

class SerializedGameData;
class nobUsual;

class nofLeatherWorker : public nofWorkman
{
    /// Draw worker at work
    void drawWorking(DrawPoint drawPt) override;
    /// Id in jobs.bob or carrier.bob when carrying a ware
    [[noreturn]] unsigned short getCarryID() const override;
    helpers::OptionalEnum<GoodType> produceWare() override;

    /// Draws the figure while returning home / entering the building (often carrying wares)
    void drawWalkingWithWare(DrawPoint drawPt) override;

public:
    nofLeatherWorker(MapPoint pos, unsigned char player, nobUsual* workplace);
    nofLeatherWorker(SerializedGameData& sgd, unsigned obj_id);

    GO_Type getGOT() const final { return GO_Type::NofLeatherWorker; }
};
