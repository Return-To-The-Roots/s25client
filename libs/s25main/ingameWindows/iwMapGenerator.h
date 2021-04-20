// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"
#include "mapGenerator/MapSettings.h"

/**
 * The iwMapGenerator is an ingame window for the user to configure parameters used
 * for random map generation (when pressing the "Random" button on map selection).
 */
class iwMapGenerator : public IngameWindow
{
public:
    enum Controls
    {
        CTRL_BTN_BACK,
        CTRL_BTN_APPLY,
        CTRL_TXT_LANDSCAPE,
        CTRL_TXT_GOAL,
        CTRL_TXT_IRON,
        CTRL_TXT_COAL,
        CTRL_TXT_GRANITE,
        CTRL_TXT_RIVERS,
        CTRL_TXT_MOUNTAIN_DIST,
        CTRL_TXT_TREES,
        CTRL_TXT_STONE_PILES,
        CTRL_TXT_ISLANDS,
        CTRL_PLAYER_NUMBER,
        CTRL_MAP_STYLE,
        CTRL_MAP_SIZE,
        CTRL_MAP_TYPE,
        CTRL_RATIO_GOLD,
        CTRL_RATIO_IRON,
        CTRL_RATIO_COAL,
        CTRL_RATIO_GRANITE,
        CTRL_RIVERS,
        CTRL_MOUNTAIN_DIST,
        CTRL_TREES,
        CTRL_STONE_PILES,
        CTRL_ISLANDS
    };

    /**
     * Creates a new ingame window to configure the random map generator.
     * @param settings reference to the settings to be manipulated
     */
    iwMapGenerator(rttr::mapGenerator::MapSettings& settings);

    ~iwMapGenerator() override;

    void Msg_ButtonClick(unsigned ctrl_id) override;

private:
    /**
     * Actual settings used for map generation. After pressing the "apply" button in the
     * UI mapSettings are updated with the values configured in the UI.
     */
    rttr::mapGenerator::MapSettings& mapSettings;

    /**
     * Resets the map generation settings to the original value.
     * Also updates the UI accordingly.
     */
    void Reset();

    /**
     * Updates the mapSettings with the values currently configured in the UI.
     */
    void Apply();
};
