// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nofWorkman.h"

class SerializedGameData;
class nobUsual;

/// Class for the carpenter
class nofBaker : public nofWorkman
{
    /// Draws him while working
    void DrawWorking(DrawPoint drawPt) override;
    /// Id in jobs.bob or carrier.bob when carrying a ware
    unsigned short GetCarryID() const override { return 76; }
    /// The worker produces a ware
    helpers::OptionalEnum<GoodType> ProduceWare() override;

public:
    nofBaker(MapPoint pos, unsigned char player, nobUsual* workplace);
    nofBaker(SerializedGameData& sgd, unsigned obj_id);

    GO_Type GetGOT() const final { return GO_Type::NofBaker; }
};
