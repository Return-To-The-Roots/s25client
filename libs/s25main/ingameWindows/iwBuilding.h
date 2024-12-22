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
protected:
    GameWorldView& gwv;
    GameCommandFactory& gcFactory;
    nobUsual* const building; /// Das zugehörige Gebäudeobjekt

public:
    iwBuilding(GameWorldView& gwv, GameCommandFactory& gcFactory, nobUsual* building, Extent extent = Extent(226, 194));

private:
    void Msg_PaintBefore() override;
    void Msg_PaintAfter() override;

protected:
    void Msg_ButtonClick(unsigned ctrl_id) override;
};
