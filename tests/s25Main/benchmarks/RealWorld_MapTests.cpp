// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Game.h"
#include "PlayerInfo.h"
#include "network/GameClient.h"
#include "ogl/glAllocator.h"
#include "world/MapLoader.h"
#include "libsiedler2/libsiedler2.h"
#include <rttr/test/Fixture.hpp>
#include <benchmark/benchmark.h>
#include <array>
#include <test/testConfig.h>
#include <utility>

constexpr std::array<std::tuple<const char*, MapPoint, MapPoint>, 7> routes = {{{"Simple 1", {85, 147}, {87, 150}},
                                                                                {"Simple 2", {85, 147}, {85, 152}},
                                                                                {"Simple 3", {85, 147}, {79, 149}},
                                                                                {"Medium 1", {85, 147}, {77, 163}},
                                                                                {"Medium 2", {85, 147}, {79, 127}},
                                                                                {"Hard", {21, 200}, {42, 188}},
                                                                                {"Water", {152, 66}, {198, 34}}}};

static void BM_PathFinding(benchmark::State& state)
{
    rttr::test::Fixture f;
    libsiedler2::setAllocator(new GlAllocator);

    std::vector<PlayerInfo> players(2);
    for(auto& player : players)
        player.ps = PlayerState::Occupied;
    auto game = std::make_shared<Game>(GlobalGameSettings(), 0, players);
    GameWorld& world = game->world_;
    MapLoader loader(world);
    if(!loader.Load(rttr::test::rttrBaseDir / "data/RTTR/MAPS/NEW/AM_FANGDERZEIT.SWD"))
        state.SkipWithError("Map failed to load");

    const auto& curValues = routes[static_cast<size_t>(state.range())];
    state.SetLabel(std::get<0>(curValues));
    const MapPoint start = std::get<1>(curValues);
    const MapPoint goal = std::get<2>(curValues);

    for(auto _ : state)
    {
        const bool result = state.range() < 6 ? world.FindHumanPath(start, goal).has_value() :
                                                world.FindShipPath(start, goal, 600, nullptr, nullptr);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_PathFinding)->DenseRange(0, routes.size() - 1);

constexpr std::array<std::tuple<const char*, unsigned>, 3> maps = {
  {{"AM_FANGDERZEIT", 7}, {"TueranTuer", 2}, {"Suedameri", 5}}};
static void BM_BQ_Calculation(benchmark::State& state)
{
    const auto& curValues = maps[static_cast<size_t>(state.range())];

    std::vector<PlayerInfo> players(std::get<1>(curValues));
    for(auto& player : players)
        player.ps = PlayerState::Occupied;
    auto game = std::make_shared<Game>(GlobalGameSettings(), 0, players);
    GameWorld& world = game->world_;
    MapLoader loader(world);

    const std::string curMap = std::get<0>(curValues);
    state.SetLabel(curMap);
    const std::string mapPath = "data/RTTR/MAPS/NEW/" + curMap + ".SWD";
    if(!loader.Load(rttr::test::rttrBaseDir / mapPath))
        state.SkipWithError(("Map " + curMap + " failed to load").c_str());

    for(auto _ : state)
    {
        world.InitAfterLoad();
        benchmark::DoNotOptimize(world);
    }
}
BENCHMARK(BM_BQ_Calculation)->DenseRange(0, maps.size() - 1);