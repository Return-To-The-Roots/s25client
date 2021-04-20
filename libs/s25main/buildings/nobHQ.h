// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nobBaseWarehouse.h"
#include "gameData/MilitaryConsts.h"
class SerializedGameData;

class nobHQ : public nobBaseWarehouse
{
    /// True if tent graphic should be used
    bool isTent_;

public:
    nobHQ(MapPoint pos, unsigned char player, Nation nation, bool isTent = false);
    nobHQ(SerializedGameData& sgd, unsigned obj_id);

protected:
    void DestroyBuilding() override;

public:
    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const final { return GO_Type::NobHq; }

    void Draw(DrawPoint drawPt) override;

    unsigned GetMilitaryRadius() const override { return HQ_RADIUS; }

    void HandleEvent(unsigned id) override;
    bool IsTent() const { return isTent_; }
    void SetIsTent(const bool isTent) { isTent_ = isTent; }
};
