// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include "nobUsual.h"
#include <cstdint>
class SerializedGameData;

/// Extraklasse für ein Schiffsbauer-Gebäude, da hier extra Optionen eingestellt werden müssen
class nobShipYard : public nobUsual
{
public:
    /// Modi für den Schiffsbauer
    enum Mode : uint8_t
    {
        BOATS, // baut kleine Boote
        SHIPS  // baut große Schiffe
    };
    friend constexpr auto maxEnumValue(Mode) { return nobShipYard::Mode::SHIPS; }

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

    GO_Type GetGOT() const override { return GOT_NOB_SHIPYARD; }

    /// Gibt aktuellen Modus zurück
    Mode GetMode() const { return mode; }
    /// Schaltet Modus entsprechend um
    void SetMode(Mode newMode);
};
