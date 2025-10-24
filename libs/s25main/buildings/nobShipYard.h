// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nobUsual.h"
#include <cstdint>
class SerializedGameData;

/// Specialized building class for shipyards, exposing additional configuration
class nobShipYard : public nobUsual
{
public:
    /// Available production modes
    enum class Mode : uint8_t
    {
        Boats, // produces small boats
        Ships  // produces large ships
    };
    friend constexpr auto maxEnumValue(Mode) { return Mode::Ships; }

private:
    /// Currently selected shipyard mode
    Mode mode;

    friend class SerializedGameData;
    friend class BuildingFactory;
    nobShipYard(MapPoint pos, unsigned char player, Nation nation);
    nobShipYard(SerializedGameData& sgd, unsigned obj_id);

public:
    /// Serialization hook
    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const final { return GO_Type::NobShipyard; }

    /// Retrieve the active mode
    Mode GetMode() const { return mode; }
    /// Change the current mode
    void SetMode(Mode newMode);
};
