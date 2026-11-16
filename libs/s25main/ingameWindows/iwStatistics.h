// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"
#include "gameTypes/StatisticTypes.h"
#include <array>

class ctrlText;
class GameWorldViewer;
class GamePlayer;

/// Window showing player statistics
class iwStatistics : public IngameWindow
{
public:
    iwStatistics(const GameWorldViewer& gwv);
    ~iwStatistics() override;

    static constexpr auto MAX_TIME_LABELS = 7;

private:
    const GameWorldViewer& gwv;
    StatisticType currentView;
    StatisticTime currentTime;
    ctrlText* headline;
    ctrlText* txtMaxValueY;
    ctrlText* txtMinValueY;
    std::array<ctrlText*, MAX_TIME_LABELS> timeAnnotations;
    std::vector<bool> showStatistic;
    unsigned numPlayingPlayers;

    void msgButtonClick(unsigned ctrl_id) override;
    void drawContent() override;
    void msgOptionGroupChange(unsigned ctrl_id, unsigned selection) override;
    void drawPlayerBox(DrawPoint const& drawPt, const GamePlayer& player);
    void drawPlayerAlliances(DrawPoint const& drawPt, const GamePlayer& player);
    void drawPlayerOverlays();
    void drawStatistic(StatisticType type);
    void drawAxis();

    void updateTimeAxisLabels();
};
