// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "desktops/dskMenuBase.h"

///  Klasse des Intro Desktops.
class dskTest : public dskMenuBase
{
public:
    dskTest();

    void msgEditChange(unsigned ctrl_id) override;
    void msgComboSelectItem(unsigned ctrl_id, unsigned selection) override;
    void msgButtonClick(unsigned ctrl_id) override;
    bool msgRightUp(const MouseCoords& mc) override;

    void ToggleCtrlVisibility();

    bool msgKeyDown(const KeyEvent& ke) override;

private:
    unsigned curBGIdx;
};
