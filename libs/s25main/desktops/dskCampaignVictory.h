// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Desktop.h"

struct MouseCoords;

class dskCampaignVictory : public Desktop
{
public:
    dskCampaignVictory();

private:
    bool Msg_LeftDown(const MouseCoords&) override;
    bool Msg_KeyDown(const KeyEvent&) override;

    bool ShowMenu();
};
