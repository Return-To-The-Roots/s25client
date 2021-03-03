// Copyright (c) 2021 - 2021 Settlers Freaks (sf-team at siedler25.org)
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

static void BM_World(benchmark::State& state)
{
    rttr::test::Fixture f;
    libsiedler2::setAllocator(new GlAllocator);

    std::vector<PlayerInfo> players(2);
    for(auto& player : players)
        player.ps = PlayerState::Occupied;
    GlobalGameSettings ggs;
    auto game = std::make_shared<Game>(ggs, 0, players);
    GameWorldGame& world = game->world_;
    MapLoader loader(world);
    if(!loader.Load(rttr::test::rttrBaseDir / "data/RTTR/MAPS/NEW/AM_FANGDERZEIT.SWD"))
        state.SkipWithError("Map failed to load");

    const auto& curValues = routes[state.range()];
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
BENCHMARK(BM_World)->DenseRange(0, routes.size() - 1);