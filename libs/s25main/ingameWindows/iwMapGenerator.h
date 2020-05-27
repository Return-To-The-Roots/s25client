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

#include "IngameWindow.h"
#include "randomMaps/MapSettings.h"
#include "randomMaps/MapStyle.h"

/**
 * The iwMapGenerator is an ingame window for the user to configure parameters used
 * for random map generation (when pressing the "Random" button on map selection).
 */
class iwMapGenerator : public IngameWindow
{
public:
    /**
     * Creates a new ingame window to configure the random map generator.
     * @param settings reference to the settings to be manipulated
     */
    iwMapGenerator(MapSettings& settings);

    ~iwMapGenerator() override;

protected:
    void Msg_ButtonClick(unsigned ctrl_id) override;

private:
    /**
     * Actual settings used for map generation. After pressing the "apply" button in the
     * UI mapSettings are updated with the values configured in the UI.
     */
    MapSettings& mapSettings;

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
