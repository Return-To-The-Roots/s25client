// Copyright (c) 2005 - 2018 Settlers Freaks (sf-team at siedler25.org)
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

#include "FrameCounter.h"
#include "desktops/dskMenuBase.h"
#include <chrono>
#include <memory>
#include <vector>

class Game;

class dskBenchmark : public dskMenuBase
{
    using clock = std::chrono::steady_clock;
    enum Test
    {
        TEST_NONE,
        TEST_TEXT,
        TEST_PRIMITIVES,
        TEST_EMPTY_GAME,
        TEST_BASIC_GAME,
        TEST_FULL_GAME,
        TEST_CT
    };
    struct ColoredRect
    {
        Rect rect;
        unsigned clr;
    };
    struct ColoredLine
    {
        Position p1, p2;
        unsigned width, clr;
    };
    struct GameView;

public:
    dskBenchmark();
    ~dskBenchmark();

    bool Msg_KeyDown(const KeyEvent& ke) override;
    void Msg_PaintAfter() override;
    void SetActive(bool activate) override;

private:
    Test curTest_;
    bool runAll_;
    int numInstances_;
    FrameCounter frameCtr_;
    std::vector<ColoredRect> rects_;
    std::vector<ColoredLine> lines_;
    std::shared_ptr<Game> game_;
    std::unique_ptr<GameView> gameView_;
    std::array<std::chrono::milliseconds, TEST_CT> testDurations_;

    void startTest(Test test);
    void finishTest();
    void createGame();
    void printTimes() const;
};
