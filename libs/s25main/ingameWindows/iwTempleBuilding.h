// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ingameWindows/iwBuilding.h"

class nobUsual;
class GameWorldView;
class GameCommandFactory;

class iwTempleBuilding : public iwBuilding
{
public:
    iwTempleBuilding(GameWorldView& gwv, GameCommandFactory& gcFactory, nobUsual* building);

private:
    void Msg_ButtonClick(unsigned ctrl_id) override;
};