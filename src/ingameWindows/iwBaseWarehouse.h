// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef iwBASEWAREHOUSE_H_INCLUDED
#define iwBASEWAREHOUSE_H_INCLUDED

#pragma once

#include "iwWares.h"
#include "IDataChangedListener.h"

class nobBaseWarehouse;
class GameWorldView;

/// Basisklasse für die HQ- und Lagerhäuserfenster
class iwBaseWarehouse : public iwWares, public IDataChangedListener
{
        GameWorldView& gwv;		

    protected:
        nobBaseWarehouse* wh; /// Pointer zum entsprechenden Lagerhaus

    public:
        iwBaseWarehouse(GameWorldView& gwv, const std::string& title, unsigned char page_count, nobBaseWarehouse* wh);
        ~iwBaseWarehouse() override;

        void OnChange(unsigned changeId) override;

    protected:

        /// Update displayed overlay (e.g. stop symbol) for the item at the current page
        void UpdateOverlay(unsigned i);
        /// Update displayed overlay (e.g. stop symbol) for the item of the given type
        void UpdateOverlay(unsigned i, bool isWare);
        void UpdateOverlays();

        void Msg_Group_ButtonClick(const unsigned int group_id, const unsigned int ctrl_id) override;
        void Msg_ButtonClick(const unsigned int ctrl_id) override;
};

#endif // !iwHQ_H_INCLUDED
