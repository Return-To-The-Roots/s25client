// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

    /// Teilt dem GameClient den Wert mit
    void SkipGFs();

    void Msg_ButtonClick(unsigned ctrl_id) override;
    void Msg_EditEnter(unsigned ctrl_id) override;
};
