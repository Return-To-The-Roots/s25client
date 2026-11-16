// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nobUsual.h"
#include "gameTypes/TempleProductionMode.h"
class SerializedGameData;

class nobTemple : public nobUsual
{
private:
    ProductionMode productionMode;

    friend class SerializedGameData;
    friend class BuildingFactory;
    nobTemple(MapPoint pos, unsigned char player, Nation nation);
    nobTemple(SerializedGameData& sgd, unsigned obj_id);

public:
    void serialize(SerializedGameData& sgd) const override;

    GO_Type getGOT() const final { return GO_Type::NobTemple; }

    ProductionMode getProductionMode() const { return productionMode; }
    void setProductionMode(ProductionMode newProductionMode);
    ProductionMode getNextProductionMode();
};
