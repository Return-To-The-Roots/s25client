// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "EconomyModeHandler.h"
#include "IngameWindow.h"

class ctrlText;
class GameWorldViewer;

/// Window for displaying the economic mode progress
class iwEconomicProgress : public IngameWindow
{
public:
    iwEconomicProgress(const GameWorldViewer& gwv);
    ~iwEconomicProgress() override;

private:
    const GameWorldViewer& gwv;
    ctrlText* txtRemainingTime;

    /// Order in which the teams are displayed
    std::vector<const EconomyModeHandler::EconTeam*> teamOrder;

    void Draw_() override;

    void Msg_ButtonClick(unsigned ctrl_id) override;
    void Msg_PaintBefore() override;
};
