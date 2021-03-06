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
#include "pathfinding/OpenListBinaryHeap.h"
#include "rttr/test/random.hpp"
#include "s25util/warningSuppression.h"
#include <benchmark/benchmark.h>
#include <test/testConfig.h>

namespace {
struct DummyNode : BinaryHeapPosMarker
{
    unsigned dummy[8]; // Roughly the size of FreePathNode
    unsigned key;
};
struct NodeGetKey
{
    constexpr auto operator()(const DummyNode& el) const { return el.key; }
};
using OpenList = OpenListBinaryHeap<DummyNode, NodeGetKey>;

auto getRandomNodes(size_t numElements, unsigned maxValue = 512)
{
    std::vector<DummyNode> nodes(numElements);
    for(auto& node : nodes)
        node.key = rttr::test::randomValue(0u, maxValue);
    return nodes;
}
} // namespace

static void BM_PushElements(benchmark::State& state)
{
    const auto numElements = static_cast<size_t>(state.range(0));
    for(auto _ : state)
    {
        state.PauseTiming();
        auto nodes = getRandomNodes(numElements);
        OpenList list;
        state.ResumeTiming();
        for(auto& node : nodes)
            list.push(&node);
        benchmark::DoNotOptimize(list);
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_PushElements)->Arg(3)->Arg(5)->Arg(7)->Arg(10)->Arg(20)->Arg(30)->Arg(40);

static void BM_PopElements(benchmark::State& state)
{
    const auto numElements = static_cast<size_t>(state.range(0));
    for(auto _ : state)
    {
        state.PauseTiming();
        auto nodes = getRandomNodes(numElements);
        OpenList list;
        for(auto& node : nodes)
            list.push(&node);
        benchmark::DoNotOptimize(list);
        state.ResumeTiming();
        for(auto& node : nodes)
        {
            list.pop();
            RTTR_UNUSED(node);
        }
        benchmark::DoNotOptimize(list);
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_PopElements)->Arg(3)->Arg(5)->Arg(7)->Arg(10)->Arg(20)->Arg(30)->Arg(40);

static void BM_PushPopElements(benchmark::State& state)
{
    const auto numElements = static_cast<size_t>(state.range(0));
    const auto numOperations = static_cast<size_t>(state.range(1));
    for(auto _ : state)
    {
        state.PauseTiming();
        auto nodes = getRandomNodes(numElements, numElements / 3u); // Force duplicates
        OpenList list;
        for(auto& node : nodes)
            list.push(&node);
        benchmark::DoNotOptimize(list);
        state.ResumeTiming();
        for(unsigned i = 0; i < numOperations; i++)
        {
            auto* el = list.pop();
            // Modify sligthly and readd
            el->key =
              static_cast<unsigned>(static_cast<int>(el->key) + rttr::test::randomValue(-std::min<int>(2, el->key), 2));
            list.push(el);
        }
        benchmark::DoNotOptimize(list);
    }
    state.SetItemsProcessed(state.iterations() * numOperations * 2);
}
BENCHMARK(BM_PushPopElements)->ArgsProduct({{3, 5, 7, 10, 20, 30, 40, 70}, {5, 7, 15, 20, 50, 200, 600}});
