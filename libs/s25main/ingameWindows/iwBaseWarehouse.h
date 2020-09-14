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

#include "IDataChangedListener.h"
#include "iwWares.h"

class nobBaseWarehouse;
class GameWorldView;
class GameCommandFactory;

/// Basisklasse für die HQ- und Lagerhäuserfenster
class iwBaseWarehouse : public iwWares, public IDataChangedListener
{
    GameWorldView& gwv;
    GameCommandFactory& gcFactory;

protected:
    nobBaseWarehouse* wh; /// Pointer zum entsprechenden Lagerhaus

public:
    iwBaseWarehouse(GameWorldView& gwv, GameCommandFactory& gcFactory, nobBaseWarehouse* wh);
    ~iwBaseWarehouse() override;

    void OnChange(unsigned changeId) override;

protected:
    /// Update displayed overlay (e.g. stop symbol) for the item at the current page
    void UpdateOverlay(unsigned i);
    /// Update displayed overlay (e.g. stop symbol) for the item of the given type
    void UpdateOverlay(unsigned i, bool isWare);
    void UpdateOverlays();

    void Msg_Group_ButtonClick(unsigned group_id, unsigned ctrl_id) override;
    void Msg_ButtonClick(unsigned ctrl_id) override;

    void SetPage(unsigned page) override;
};
