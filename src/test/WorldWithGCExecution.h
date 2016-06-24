// Copyright (c) 2016 Settlers Freaks (sf-team at siedler25.org)
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

#include "CreateEmptyWorld.h"
#include "WorldFixture.h"
#include "GamePlayer.h"
#include "GCExecutor.h"

#ifndef WorldWithGCExecution_h__
#define WorldWithGCExecution_h__

template<unsigned T_numPlayers, unsigned T_width = 64, unsigned T_height = 64>
class WorldWithGCExecution: public WorldFixture<CreateEmptyWorld, T_numPlayers, T_width, T_height>, public GCExecutor
{
public:
    typedef WorldFixture<CreateEmptyWorld, T_numPlayers, T_width, T_height> Parent;
    using Parent::world;

    MapPoint hqPos;
    WorldWithGCExecution(): hqPos(world.GetPlayer(curPlayer).GetHQPos()){}

protected:
    virtual GameWorldGame& GetWorld() override { return world; }
};

// Avoid having to use "this->" to access those
class WorldWithGCExecution2P: public WorldWithGCExecution<2>
{
public:
    using WorldWithGCExecution<2>::world;
    using WorldWithGCExecution<2>::curPlayer;
    using WorldWithGCExecution<2>::hqPos;
};

class WorldWithGCExecution3P: public WorldWithGCExecution<3>
{
public:
    using WorldWithGCExecution<3>::world;
    using WorldWithGCExecution<3>::curPlayer;
    using WorldWithGCExecution<3>::hqPos;
};

#endif // WorldWithGCExecution_h__
