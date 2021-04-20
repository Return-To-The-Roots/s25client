// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"

class nobUsual;
class GameWorldView;
class GameCommandFactory;

class iwBuilding : public IngameWindow
{
    GameWorldView& gwv;
    GameCommandFactory& gcFactory;
    nobUsual* const building; /// Das zugehörige Gebäudeobjekt

public:
    iwBuilding(GameWorldView& gwv, GameCommandFactory& gcFactory, nobUsual* building);

private:
    void Msg_PaintBefore() override;
    void Msg_PaintAfter() override;
    void Msg_ButtonClick(unsigned ctrl_id) override;
};
