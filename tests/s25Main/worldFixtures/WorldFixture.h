// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Game.h"
#include "GlobalGameSettings.h"
#include "PlayerInfo.h"
#include "TestEventManager.h"
#include "addons/const_addons.h"
#include "world/GameWorld.h"
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

struct WorldFixtureBase
{
    std::shared_ptr<Game> game;
    TestEventManager& em;
    GlobalGameSettings& ggs;
    GameWorld& world;
    WorldFixtureBase(unsigned numPlayers)
        : game(std::make_shared<Game>(GlobalGameSettings(), std::make_unique<TestEventManager>(),
                                      std::vector<PlayerInfo>(numPlayers, GetPlayer()))),
          em(static_cast<TestEventManager&>(*game->em_)), ggs(const_cast<GlobalGameSettings&>(game->ggs_)),
          world(game->world_)
    { // Fast moving ships
        ggs.setSelection(AddonId::SHIP_SPEED, 4);
        // Explored area stays explored. Avoids fow creation
        ggs.exploration = Exploration::Classic;
    }
    /// Add start resources to HQs of all players
    void addStartResources();
    /// Add start resources to HQ of the given player
    void addStartResources(unsigned playerIdx);

    static PlayerInfo GetPlayer()
    {
        PlayerInfo result;
        result.ps = PlayerState::Occupied;
        return result;
    }
};

template<class T_WorldCreator, unsigned T_numPlayers = 0, unsigned T_width = WorldDefault<T_numPlayers>::width,
         unsigned T_height = WorldDefault<T_numPlayers>::height>
struct WorldFixture : WorldFixtureBase
{
    T_WorldCreator worldCreator;
    WorldFixture() : WorldFixtureBase(T_numPlayers), worldCreator(MapExtent(T_width, T_height))
    {
        BOOST_TEST_REQUIRE(worldCreator(world));
        BOOST_TEST_REQUIRE(world.GetNumPlayers() == T_numPlayers);
    }
};

class TestWorld : public World
{
public:
    TestWorld() = default;
    TestWorld(const MapExtent size, DescIdx<LandscapeDesc> lt = DescIdx<LandscapeDesc>{1}) { Init(size, lt); }
    using World::GetNodeInt;

protected:
    // LCOV_EXCL_START
    void AltitudeChanged(MapPoint) override {}
    void VisibilityChanged(MapPoint, unsigned, Visibility, Visibility) override {}
    // LCOV_EXCL_STOP
};
