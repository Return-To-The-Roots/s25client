// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"

class noShip;
class GameWorldView;
class GameCommandFactory;

class iwShip : public IngameWindow
{
    GameWorldView& gwv;
    GameCommandFactory& gcFactory;
    unsigned char player; /// Besitzer des Schiffes, den wir für die Umwandlung ID->richtiges Schiff brauchen
    unsigned ship_id;     /// ID des Schiffes, welches gerade angezeigt wird

public:
    iwShip(GameWorldView& gwv, GameCommandFactory& gcFactory, const noShip* ship,
           const DrawPoint& pos = IngameWindow::posAtMouse);

private:
    void Draw_() override;
    void Msg_ButtonClick(unsigned ctrl_id) override;

    void DrawCargo();
};
