// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "desktops/dskMenuBase.h"

///  Klasse des Intro Desktops.
class dskTest : public dskMenuBase
{
public:
    dskTest();

    void Msg_EditChange(unsigned ctrl_id) override;
    void Msg_ComboSelectItem(unsigned ctrl_id, unsigned selection) override;
    void Msg_ButtonClick(unsigned ctrl_id) override;
    bool Msg_RightUp(const MouseCoords& mc) override;

    void ToggleCtrlVisibility();

    bool Msg_KeyDown(const KeyEvent& ke) override;

private:
    unsigned curBGIdx;
};
