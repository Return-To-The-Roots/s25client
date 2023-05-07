// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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
    void PrepareCampaigns();
    void PrepareSinglePlayerServer();
    void PrepareLoadGame();
};
