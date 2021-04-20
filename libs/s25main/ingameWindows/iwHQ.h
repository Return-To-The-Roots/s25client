// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "iwBaseWarehouse.h"

class GameWorldView;
class nobBaseWarehouse;

class iwHQ : public iwBaseWarehouse
{
public:
    iwHQ(GameWorldView& gwv, GameCommandFactory& gcFactory, nobBaseWarehouse* wh);

protected:
    void Msg_Group_ButtonClick(unsigned group_id, unsigned ctrl_id) override;
    unsigned grpIdReserve;
};
