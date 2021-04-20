// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"

class GameCommandFactory;
class GameWorldView;

class iwMainMenu : public IngameWindow
{
public:
    iwMainMenu(GameWorldView& gwv, GameCommandFactory& gcFactory);

private:
    GameWorldView& gwv;
    GameCommandFactory& gcFactory;

    void Msg_ButtonClick(unsigned ctrl_id) override;
};
