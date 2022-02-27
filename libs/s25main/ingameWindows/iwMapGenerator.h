// Copyright (C) 2005 - 2022 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"
#include "mapGenerator/MapSettings.h"

/// Window to configure parameters used for random map generation
class iwMapGenerator : public IngameWindow
{
public:
    enum
    {
        ID_btBack,
        ID_btApply,
        ID_txtLandscape,
        ID_txtGold,
        ID_txtIron,
        ID_txtCoal,
        ID_txtGranite,
        ID_txtRivers,
        ID_txtMountainDist,
        ID_txtTrees,
        ID_txtStonePiles,
        ID_txtIslands,
        ID_cbNumPlayers,
        ID_txtMapStyle,
        ID_cbMapStyle,
        ID_txtMapSize,
        ID_txtMapSizeX,
        ID_cbMapSizeX,
        ID_cbMapSizeY,
        ID_cbMapType,
        ID_pgGoldRatio,
        ID_pgIronRatio,
        ID_pgCoalRatio,
        ID_pgGraniteRatio,
        ID_pgRivers,
        ID_cbMountainDist,
        ID_pgTrees,
        ID_pgStonePiles,
        ID_cbIslands
    };

    /**
     * Creates a new ingame window to configure the random map generator.
     * @param settings reference to the settings to be manipulated
     */
    iwMapGenerator(rttr::mapGenerator::MapSettings& settings);

    void Msg_ButtonClick(unsigned ctrl_id) override;

private:
    rttr::mapGenerator::MapSettings& mapSettings;

    /// Reset the UI to the values of @ref mapSettings
    void Reset();
    /// Updates @ref mapSettings with the values currently configured in the UI.
    void Apply();
};
