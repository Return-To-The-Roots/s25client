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

#ifndef WorldFixture_h__
#define WorldFixture_h__

#include "Game.h"
#include "GlobalGameSettings.h"
#include "PlayerInfo.h"
#include "TestEventManager.h"
#include "addons/const_addons.h"
#include "world/GameWorld.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/Nation.h"
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <vector>

//////////////////////////////////////////////////////////////////////////
// Macros for executing GFs in tests effectively by skipping GFs without any events

/// Execute up to maxGFs gameframes or till a condition is met. Asserts the condition is true afterwards
/// Return the number of GFs executed in gfReturnVar
#define RTTR_EXEC_TILL_CT_GF(maxGFs, cond, gfReturnVar)                                          \
    gfReturnVar = 0;                                                                             \
    for(unsigned endGf = this->em.GetCurrentGF() + (maxGFs); !(cond) && gfReturnVar < (maxGFs);) \
    {                                                                                            \
        unsigned numGF = this->em.ExecuteNextEvent(endGf);                                       \
        if(numGF == 0)                                                                           \
            break;                                                                               \
        gfReturnVar += numGF;                                                                    \
    }                                                                                            \
    BOOST_REQUIRE((cond))

/// Execute up to maxGFs gameframes or till a condition is met. Asserts the condition is true afterwards
#define RTTR_EXEC_TILL(maxGFs, cond)                       \
    {                                                      \
        unsigned dummyReturnGF;                            \
        RTTR_EXEC_TILL_CT_GF(maxGFs, cond, dummyReturnGF); \
        (void)dummyReturnGF;                               \
    }

/// Skip up to numGFs GFs or until no event left
#define RTTR_SKIP_GFS(numGFs)                                                      \
    for(unsigned gf = 0, endGf = this->em.GetCurrentGF() + (numGFs); gf < numGFs;) \
    {                                                                              \
        unsigned numGFsExecuted = this->em.ExecuteNextEvent(endGf);                \
        if(numGFsExecuted == 0)                                                    \
            break;                                                                 \
        gf += numGFsExecuted;                                                      \
    }
//////////////////////////////////////////////////////////////////////////

template<unsigned T_numPlayers>
struct WorldDefault
{
    BOOST_STATIC_CONSTEXPR unsigned width = 40;
    BOOST_STATIC_CONSTEXPR unsigned height = 32;
};

template<>
struct WorldDefault<0>
{
    BOOST_STATIC_CONSTEXPR unsigned width = 10;
    BOOST_STATIC_CONSTEXPR unsigned height = 8;
};

template<>
struct WorldDefault<1>
{
    // Note: Less than HQ radius but enough for most tests
    BOOST_STATIC_CONSTEXPR unsigned width = 12;
    BOOST_STATIC_CONSTEXPR unsigned height = 10;
};

template<>
struct WorldDefault<2>
{
    // Based on HQ radius of 9 -> min size 20 per player
    BOOST_STATIC_CONSTEXPR unsigned width = 40;
    BOOST_STATIC_CONSTEXPR unsigned height = 20;
};

template<class T_WorldCreator, unsigned T_numPlayers = 0, unsigned T_width = WorldDefault<T_numPlayers>::width,
         unsigned T_height = WorldDefault<T_numPlayers>::height>
struct WorldFixture
{
    boost::shared_ptr<Game> game;
    TestEventManager& em;
    GlobalGameSettings& ggs;
    GameWorld& world;
    T_WorldCreator worldCreator;
    WorldFixture()
        : game(new Game(GlobalGameSettings(), new TestEventManager, std::vector<PlayerInfo>(T_numPlayers, GetPlayer()))),
          em(static_cast<TestEventManager&>(*game->em)), ggs(const_cast<GlobalGameSettings&>(game->ggs)), world(game->world),
          worldCreator(MapExtent(T_width, T_height))
    {
        // Fast moving ships
        ggs.setSelection(AddonId::SHIP_SPEED, 4);
        // Explored area stays explored. Avoids fow creation
        ggs.exploration = EXP_CLASSIC;
        BOOST_REQUIRE(worldCreator(world));
        BOOST_REQUIRE_EQUAL(world.GetNumPlayers(), T_numPlayers);
    }
    static PlayerInfo GetPlayer()
    {
        PlayerInfo result;
        result.ps = PS_OCCUPIED;
        return result;
    }
};

#endif // WorldFixture_h__
