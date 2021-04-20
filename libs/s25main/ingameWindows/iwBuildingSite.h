// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"

class noBuildingSite;
class GameWorldView;

class iwBuildingSite : public IngameWindow
{
public:
    iwBuildingSite(GameWorldView& gwv, const noBuildingSite* buildingsite);

protected:
    void Msg_ButtonClick(unsigned ctrl_id) override;
    void Msg_PaintAfter() override;

private:
    GameWorldView& gwv;
    const noBuildingSite* buildingsite;
};
