// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"
#include "gameTypes/StatisticTypes.h"

class ctrlText;
class GameWorldViewer;

/// Fenster mit den Statistiken.
class iwStatistics : public IngameWindow
{
public:
    iwStatistics(const GameWorldViewer& gwv);
    ~iwStatistics() override;

private:
    const GameWorldViewer& gwv;
    StatisticType currentView;
    StatisticTime currentTime;
    ctrlText* headline;
    ctrlText* maxValue;
    ctrlText* minValue;
    std::vector<ctrlText*> timeAnnotations;
    std::vector<bool> activePlayers;
    unsigned numPlayingPlayers;

    void Msg_ButtonClick(unsigned ctrl_id) override;
    void Draw_() override;
    void Msg_OptionGroupChange(unsigned ctrl_id, unsigned selection) override;
    void DrawPlayerBoxes();
    void DrawStatistic(StatisticType type);
    void DrawAxis();
};
