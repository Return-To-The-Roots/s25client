// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "desktops/dskMenuBase.h"

/// Klasse des Hauptmenü Desktops.
class dskMainMenu : public dskMenuBase
{
public:
    dskMainMenu();

    void msgButtonClick(unsigned ctrl_id) override;
    void msgTimer(unsigned ctrl_id) override;
    void msgMsgBoxResult(unsigned msgbox_id, MsgboxResult mbr) override;
    bool msgLeftUp(const MouseCoords& mc) override;
};
