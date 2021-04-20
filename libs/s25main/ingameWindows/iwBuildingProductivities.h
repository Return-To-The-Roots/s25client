// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"
#include "helpers/EnumArray.h"
#include "gameTypes/BuildingType.h"

class GamePlayer;

/// Fenster, welches die Anzahl aller Gebäude und der Baustellen auflistet
class iwBuildingProductivities : public IngameWindow
{
    const GamePlayer& player;
    /// Prozentzahlen der einzelnen Gebäude
    helpers::EnumArray<uint16_t, BuildingType> percents;

public:
    iwBuildingProductivities(const GamePlayer& player);

private:
    /// Aktualisieren der Prozente
    void UpdatePercents();

    /// Produktivitäts-Progressbars aktualisieren
    void Msg_PaintAfter() override;
};
