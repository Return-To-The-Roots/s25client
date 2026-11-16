// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nobBaseWarehouse.h"
class SerializedGameData;

class nobStorehouse : public nobBaseWarehouse
{
    friend class SerializedGameData;
    friend class BuildingFactory;
    nobStorehouse(MapPoint pos, unsigned char player, Nation nation);
    nobStorehouse(SerializedGameData& sgd, unsigned obj_id);

public:
    GO_Type getGOT() const final { return GO_Type::NobStorehouse; }
    unsigned getMilitaryRadius() const override { return 0; }
    bool isAttackable(unsigned /*playerIdx*/) const override { return false; }

    void draw(DrawPoint drawPt) override;

    void handleEvent(unsigned id) override;
};
