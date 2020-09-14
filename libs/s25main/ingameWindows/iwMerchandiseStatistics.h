// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

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
    void DrawRectangles();
    /// Zeichnet das Achsensystem
    void DrawAxis();
    /// Zeichnet die Statistikdaten (TODO)
    void DrawStatistic();

    // Die Farben für die einzelnen Warenlinien
    static const std::array<unsigned, 14> BarColors;
    const GamePlayer& player;

    // Aktueller Zeitbereich
    StatisticTime currentTime;

    // Textelemente für die verschiedenen Zeitbereiche
    std::vector<ctrlText*> timeAnnotations;

    // Maximalwert der y-Achse
    ctrlText* maxValue;

    // Durchgereichte Methoden vom Window
    void Draw_() override;
    void Msg_OptionGroupChange(unsigned ctrl_id, unsigned selection) override;
    void Msg_ButtonClick(unsigned ctrl_id) override;
};
