// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"
#include "gameTypes/StatisticTypes.h"

class ctrlText;
class GamePlayer;

/// Fenster mit den Warenstatistiken.
class iwMerchandiseStatistics : public IngameWindow
{
public:
    iwMerchandiseStatistics(const GamePlayer& player);
    ~iwMerchandiseStatistics() override;

private:
    /// Malt die bunten Kästchen über den Buttons
    void drawRectangles();
    /// Zeichnet das Achsensystem
    void drawAxis();
    /// Zeichnet die Statistikdaten (TODO)
    void drawStatistic();

    // Die Farben für die einzelnen Warenlinien
    static const std::array<unsigned, 14> BarColors;
    const GamePlayer& player;

    // Aktueller Zeitbereich
    StatisticTime currentTime;

    // Textelemente für die verschiedenen Zeitbereiche
    std::vector<ctrlText*> timeAnnotations;

    // Maximalwert der y-Achse
    ctrlText* maxValue;

    void drawContent() override;
    // Durchgereichte Methoden vom Window
    void msgOptionGroupChange(unsigned ctrl_id, unsigned selection) override;
    void msgButtonClick(unsigned ctrl_id) override;
};
