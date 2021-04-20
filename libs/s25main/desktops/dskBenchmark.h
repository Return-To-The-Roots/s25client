// Copyright (c) 2005 - 2018 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "FrameCounter.h"
#include "desktops/dskMenuBase.h"
#include "helpers/EnumArray.h"
#include <chrono>
#include <memory>
#include <vector>

class Game;

enum class Benchmark
{
    None,
    Text,
    Primitives,
    EmptyGame,
    BasicGame,
    FullGame,
};
constexpr auto maxEnumValue(Benchmark)
{
    return Benchmark::FullGame;
}

class dskBenchmark : public dskMenuBase
{
    using clock = std::chrono::steady_clock;

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
    Benchmark curTest_;
    bool runAll_;
    int numInstances_;
    FrameCounter frameCtr_;
    std::vector<ColoredRect> rects_;
    std::vector<ColoredLine> lines_;
    std::shared_ptr<Game> game_;
    std::unique_ptr<GameView> gameView_;
    helpers::EnumArray<std::chrono::milliseconds, Benchmark> testDurations_;

    void startTest(Benchmark test);
    void finishTest();
    void createGame();
    void printTimes() const;
};
