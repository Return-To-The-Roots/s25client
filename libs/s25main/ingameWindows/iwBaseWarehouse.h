// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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

    void onChange(unsigned changeId) override;

protected:
    /// Update displayed overlay (e.g. stop symbol) for the item at the current page
    void updateOverlay(unsigned i);
    /// Update displayed overlay (e.g. stop symbol) for the item of the given type
    void updateOverlay(unsigned i, bool isWare);
    void updateOverlays();

    void msgGroupButtonClick(unsigned group_id, unsigned ctrl_id) override;
    void msgButtonClick(unsigned ctrl_id) override;

    void setPage(unsigned page) override;
};
