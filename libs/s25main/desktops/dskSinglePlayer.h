// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "desktops/dskMenuBase.h"

/// Klasse des Einzelspieler Desktops.
class dskSinglePlayer : public dskMenuBase
{
public:
    dskSinglePlayer();

private:
    void Msg_ButtonClick(unsigned ctrl_id) override;
    void PrepareSinglePlayerServer();
    void PrepareLoadGame();
};
