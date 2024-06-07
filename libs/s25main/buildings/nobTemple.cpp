// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nobTemple.h"
#include "SerializedGameData.h"

ProductionMode nobTemple::getNextProductionMode()
{
    return TRANSITIONS[productionMode];
}

nobTemple::nobTemple(const MapPoint pos, const unsigned char player, const Nation nation)
    : nobUsual(BuildingType::Temple, pos, player, nation), productionMode(ProductionMode::Default)
{}

nobTemple::nobTemple(SerializedGameData& sgd, const unsigned obj_id)
    : nobUsual(sgd, obj_id), productionMode(sgd.Pop<ProductionMode>())
{}

void nobTemple::Serialize(SerializedGameData& sgd) const
{
    nobUsual::Serialize(sgd);

    sgd.PushEnum<uint8_t>(productionMode);
}

void nobTemple::SetProductionMode(ProductionMode newProductionMode)
{
    productionMode = newProductionMode;
}
