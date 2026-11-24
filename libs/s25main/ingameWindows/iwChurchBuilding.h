// Copyright (C) 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ingameWindows/iwBuilding.h"

class GameCommandFactory;
class GameWorldView;
class nobUsual;

class iwChurchBuilding : public iwBuilding
{
public:
    iwChurchBuilding(GameWorldView& gwv, GameCommandFactory& gcFactory, nobUsual* building);

private:
    void msgPaintBefore() override;
};
