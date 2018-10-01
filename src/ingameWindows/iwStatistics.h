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
#ifndef iwSTATISTICS_H_INCLUDED
#define iwSTATISTICS_H_INCLUDED

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

    void Msg_ButtonClick(const unsigned ctrl_id) override;
    void Msg_PaintAfter() override;
    void Msg_OptionGroupChange(const unsigned ctrl_id, const int selection) override;
    void DrawStatistic(StatisticType type);
    void DrawAxis();
};

#endif // !iwSTATISTICS_H_INCLUDED
