// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"

class GameWorldView;
class noBaseBuilding;

/// Fenster, welches eine Sicherheitsabfrage vor dem Abreißen eines Gebäudes durchführt
class iwDemolishBuilding : public IngameWindow
{
    GameWorldView& gwv;
    const noBaseBuilding* building;
    const bool flag;

public:
    iwDemolishBuilding(GameWorldView& gwv, const noBaseBuilding* building, bool flag = false);

private:
    void Msg_ButtonClick(unsigned ctrl_id) override;
};
