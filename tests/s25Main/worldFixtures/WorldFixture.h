// Copyright (c) 2016 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "Game.h"
#include "GlobalGameSettings.h"
#include "PlayerInfo.h"
#include "TestEventManager.h"
#include "addons/const_addons.h"
#include "world/GameWorldGame.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/Nation.h"
#include <boost/test/unit_test.hpp>
#include <memory>
#include <vector>

//////////////////////////////////////////////////////////////////////////
// Macros for executing GFs in tests effectively by skipping GFs without any events

/// Execute up to maxGFs gameframes or till a condition is met. Asserts the condition is true afterwards
/// Return the number of GFs executed
template<class T>
unsigned rttr_exec_till_ct_gf(TestEventManager& em, unsigned maxGFs, T&& cond)
{
    const unsigned endGf = em.GetCurrentGF() + maxGFs;
    unsigned gfsExecuted = 0;
    while(!cond())
    {
        const unsigned numGF = em.ExecuteNextEvent(endGf);
        if(numGF == 0)
            break;
        gfsExecuted += numGF;
    }
    return gfsExecuted;
}

#define RTTR_EXEC_TILL_CT_GF(maxGFs, cond, gfReturnVar)                           \
    gfReturnVar = rttr_exec_till_ct_gf(this->em, maxGFs, [&] { return (cond); }); \
    BOOST_TEST_REQUIRE((cond))

/// Execute up to maxGFs gameframes or till a condition is met. Asserts the condition is true afterwards
#define RTTR_EXEC_TILL(maxGFs, cond)                       \
    {                                                      \
        unsigned dummyReturnGF;                            \
        RTTR_EXEC_TILL_CT_GF(maxGFs, cond, dummyReturnGF); \
        (void)dummyReturnGF;                               \
    }

inline void rttr_skip_gfs(TestEventManager& em, unsigned numGFs)
{
    rttr_exec_till_ct_gf(em, numGFs, [] { return false; });
}
/// Skip up to numGFs GFs or until no event left
#define RTTR_SKIP_GFS(numGFs) rttr_skip_gfs(this->em, numGFs)
//////////////////////////////////////////////////////////////////////////

template<unsigned T_numPlayers>
struct WorldDefault
{
    static constexpr unsigned width = 40;
    static constexpr unsigned height = 32;
};

template<>
struct WorldDefault<0>
{
    static constexpr unsigned width = 10;
    static constexpr unsigned height = 8;
};

template<>
struct WorldDefault<1>
{
    // Note: Less than HQ radius but enough for most tests
    static constexpr unsigned width = 12;
    static constexpr unsigned height = 10;
};

template<>
struct WorldDefault<2>
{
    // Based on HQ radius of 9 -> min size 20 per player
    static constexpr unsigned width = 40;
    static constexpr unsigned height = 20;
};

template<class T_WorldCreator, unsigned T_numPlayers = 0, unsigned T_width = WorldDefault<T_numPlayers>::width,
         unsigned T_height = WorldDefault<T_numPlayers>::height>
struct WorldFixture
{
    std::shared_ptr<Game> game;
    TestEventManager& em;
    GlobalGameSettings& ggs;
    GameWorldGame& world;
    T_WorldCreator worldCreator;
    WorldFixture()
        : game(std::make_shared<Game>(GlobalGameSettings(), std::make_unique<TestEventManager>(),
                                      std::vector<PlayerInfo>(T_numPlayers, GetPlayer()))),
          em(static_cast<TestEventManager&>(*game->em_)), ggs(const_cast<GlobalGameSettings&>(game->ggs_)),
          world(game->world_), worldCreator(MapExtent(T_width, T_height))
    {
        // Fast moving ships
        ggs.setSelection(AddonId::SHIP_SPEED, 4);
        // Explored area stays explored. Avoids fow creation
        ggs.exploration = Exploration::Classic;
        BOOST_TEST_REQUIRE(worldCreator(world));
        BOOST_TEST_REQUIRE(world.GetNumPlayers() == T_numPlayers);
    }
    static PlayerInfo GetPlayer()
    {
        PlayerInfo result;
        result.ps = PlayerState::Occupied;
        return result;
    }
};

class TestWorld : public World
{
public:
    TestWorld() = default;
    TestWorld(const MapExtent size, DescIdx<LandscapeDesc> lt = DescIdx<LandscapeDesc>{1}) { Init(size, lt); }
    using World::GetNodeInt;

protected:
    void AltitudeChanged(MapPoint) override {}
    void VisibilityChanged(MapPoint, unsigned, Visibility, Visibility) override {}
};