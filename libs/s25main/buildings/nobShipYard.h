// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nobUsual.h"
#include <cstdint>
class SerializedGameData;

/// Extraklasse für ein Schiffsbauer-Gebäude, da hier extra Optionen eingestellt werden müssen
class nobShipYard : public nobUsual
{
public:
    /// Modi für den Schiffsbauer
    enum class Mode : uint8_t
    {
        Boats, // baut kleine Boote
        Ships  // baut große Schiffe
    };
    friend constexpr auto maxEnumValue(Mode) { return Mode::Ships; }

private:
    /// Aktueller Modus vom Schiffsbauer
    Mode mode;

    friend class SerializedGameData;
    friend class BuildingFactory;
    nobShipYard(MapPoint pos, unsigned char player, Nation nation);
    nobShipYard(SerializedGameData& sgd, unsigned obj_id);

public:
    /// Serialisierungsfunktionen
    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const final { return GO_Type::NobShipyard; }

    /// Gibt aktuellen Modus zurück
    Mode GetMode() const { return mode; }
    /// Schaltet Modus entsprechend um
    void SetMode(Mode newMode);
};
