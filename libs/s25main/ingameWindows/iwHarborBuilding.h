// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "iwHQ.h"

class nobHarborBuilding;
class GameWorldView;

class iwHarborBuilding : public iwHQ
{
public:
    iwHarborBuilding(GameWorldView& gwv, GameCommandFactory& gcFactory, nobHarborBuilding* hb);

protected:
    void msgGroupButtonClick(unsigned group_id, unsigned ctrl_id) override;

private:
    void adjustExpeditionButton(bool flip);
    void adjustExplorationExpeditionButton(bool flip);

    unsigned grpIdExpedition;
};
