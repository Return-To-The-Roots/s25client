// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"

class GameWorldView;

class iwSkipGFs : public IngameWindow
{
public:
    iwSkipGFs(GameWorldView& gwv);

private:
    GameWorldView& gwv;

    void SkipGFs(unsigned edtCtrlId);

    void Msg_ButtonClick(unsigned ctrlId) override;
    void Msg_EditEnter(unsigned ctrlId) override;
};
