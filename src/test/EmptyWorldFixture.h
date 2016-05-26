// Copyright (c) 2005 - 2016 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef EmptyWorldFixture_h__
#define EmptyWorldFixture_h__

#include "CreateEmptyWorld.h"
#include "world/GameWorldGame.h"
#include "EventManager.h"
#include "GlobalGameSettings.h"
#include "PlayerInfo.h"
#include "gameTypes/Nation.h"
#include <boost/test/unit_test.hpp>
#include <vector>

template<unsigned T_numPlayers = 0, unsigned T_width = 128, unsigned T_height = 128>
struct EmptyWorldFixture
{
    EventManager em;
    GlobalGameSettings ggs;
    GameWorldGame world;
    EmptyWorldFixture(): em(0), world(std::vector<PlayerInfo>(T_numPlayers, GetPlayer()), ggs, em)
    {
        GameObject::SetPointers(&world);
        BOOST_REQUIRE(CreateEmptyWorld(T_width, T_height, T_numPlayers)(world));
        BOOST_REQUIRE_EQUAL(world.GetPlayerCount(), T_numPlayers);
    }
    ~EmptyWorldFixture()
    {
        // Reset to allow assertions on GameObject destruction to pass
        GameObject::SetPointers(NULL);
    }
    static PlayerInfo GetPlayer()
    {
        PlayerInfo result;
        result.ps = PS_OCCUPIED;
        return result;
    }
};

#endif // EmptyWorldFixture_h__
