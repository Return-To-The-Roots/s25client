// Copyright (c) 2005-2010 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Siedler II.5 RTTR.
//
// Siedler II.5 RTTR is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Siedler II.5 RTTR is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Siedler II.5 RTTR. If not, see <http://www.gnu.org/licenses/>.
#ifndef iwMAPGENERATOR_H_INCLUDED
#define iwMAPGENERATOR_H_INCLUDED

#pragma once

#include "mapGenerator/MapSettings.h"
#include "mapGenerator/MapStyle.h"
#include "IngameWindow.h"
#include "addons/const_addons.h"
#include <vector>

class iwMapGenerator : public IngameWindow
{
    public:
        iwMapGenerator(MapSettings& settings);
        ~iwMapGenerator() override;

    protected:
        void Msg_ButtonClick(const unsigned int ctrl_id) override;
        void Msg_ComboSelectItem(const unsigned int ctrl_id, const int selection) override;
    
    private:
    
        /**
         * Actual settings used for map generation. After pressing the "apply" button in the
         * UI mapSettings are set to the current tmpSettings.
         */
        MapSettings& mapSettings;
    
        /**
         * Map settings which are directly manipulated by UI.
         */
        MapSettings tmpSettings;
    
        /**
         * Resets the map generation settings to the original value.
         * Also updates the UI accordingly.
         */
        void Reset();
};

#endif // !iwMAPGENERATOR_H_INCLUDED
