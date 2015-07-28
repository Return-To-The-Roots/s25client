﻿
#include "defines.h"
#include "nobShipYard.h"
#include "SerializedGameData.h"

nobShipYard::nobShipYard(const MapPoint pos, const unsigned char player, const Nation nation)
    : nobUsual(BLD_SHIPYARD, pos, player, nation), mode(nobShipYard::BOATS)
{
}

nobShipYard::nobShipYard(SerializedGameData* sgd, const unsigned obj_id)
    : nobUsual(sgd, obj_id), mode(nobShipYard::Mode(sgd->PopUnsignedChar()))
{
}

/// Serialisierungsfunktionen
void nobShipYard::Serialize(SerializedGameData* sgd) const
{
    Serialize_nobUsual(sgd);

    sgd->PushUnsignedChar(static_cast<unsigned char>(mode));
}

/// Schaltet Modus entsprechend um
void nobShipYard::ToggleMode()
{
    if(mode == nobShipYard::BOATS)
        mode = nobShipYard::SHIPS;
    else
        mode = nobShipYard::BOATS;
}
