// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nobShipYard.h"
#include "SerializedGameData.h"

nobShipYard::nobShipYard(const MapPoint pos, const unsigned char player, const Nation nation)
    : nobUsual(BuildingType::Shipyard, pos, player, nation), mode(nobShipYard::Mode::Boats)
{}

nobShipYard::nobShipYard(SerializedGameData& sgd, const unsigned obj_id)
    : nobUsual(sgd, obj_id), mode(sgd.Pop<nobShipYard::Mode>())
{}

/// Serialisierungsfunktionen
void nobShipYard::Serialize(SerializedGameData& sgd) const
{
    nobUsual::Serialize(sgd);

    sgd.PushEnum<uint8_t>(mode);
}

/// Schaltet Modus entsprechend um
void nobShipYard::SetMode(Mode newMode)
{
    mode = newMode;
}
