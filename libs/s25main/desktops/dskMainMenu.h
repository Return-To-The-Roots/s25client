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

    void Msg_ButtonClick(unsigned ctrl_id) override;
    void Msg_Timer(unsigned ctrl_id) override;
    void Msg_MsgBoxResult(unsigned msgbox_id, MsgboxResult mbr) override;
    bool Msg_LeftUp(const MouseCoords& mc) override;
};
