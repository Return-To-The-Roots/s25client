// Copyright (C) 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "figures/nofWorkman.h"

class nobUsual;
class SerializedGameData;

/// Worker staffing a church, crafting Blessings stored inside the church
class nofChurchWorker : public nofWorkman
{
private:
    void drawWorking(DrawPoint drawPt) override;
    unsigned short getCarryID() const override;
    helpers::OptionalEnum<GoodType> produceWare() override;
    bool areWaresAvailable() const override;

public:
    nofChurchWorker(MapPoint pos, unsigned char player, nobUsual* workplace);
    nofChurchWorker(SerializedGameData& sgd, unsigned obj_id);

    GO_Type getGOT() const final { return GO_Type::NofChurchworker; }
};
