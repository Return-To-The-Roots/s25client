// Copyright (C) 2016 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "GCExecutor.h"
#include "GamePlayer.h"
#include "worldFixtures/CreateEmptyWorld.h"
#include "worldFixtures/WorldFixture.h"

template<unsigned T_numPlayers, unsigned T_width = WorldDefault<T_numPlayers>::width,
         unsigned T_height = WorldDefault<T_numPlayers>::height>
class WorldWithGCExecution : public WorldFixture<CreateEmptyWorld, T_numPlayers, T_width, T_height>, public GCExecutor
{
public:
    using Parent = WorldFixture<CreateEmptyWorld, T_numPlayers, T_width, T_height>;
    using Parent::world;

    MapPoint hqPos;
    WorldWithGCExecution() : hqPos(world.GetPlayer(curPlayer).GetHQPos()) {}

protected:
    GameWorld& GetWorld() override { return world; }
};

using WorldWithGCExecution1P = WorldWithGCExecution<1>;
using WorldWithGCExecution2P = WorldWithGCExecution<2>;
using WorldWithGCExecution3P = WorldWithGCExecution<3>;
