// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
