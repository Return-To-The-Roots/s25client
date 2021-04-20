// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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
    void Msg_Group_ButtonClick(unsigned group_id, unsigned ctrl_id) override;

private:
    void AdjustExpeditionButton(bool flip);
    void AdjustExplorationExpeditionButton(bool flip);

    unsigned grpIdExpedition;
};
