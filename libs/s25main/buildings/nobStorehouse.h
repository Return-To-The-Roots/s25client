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
    GO_Type GetGOT() const final { return GO_Type::NobStorehouse; }
    unsigned GetMilitaryRadius() const override { return 0; }
    bool CanBeAttackedBy(unsigned /*playerIdx*/) const override { return false; }

    void Draw(DrawPoint drawPt) override;

    void HandleEvent(unsigned id) override;
};
