// Copyright (C) 2016 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "CreateSeaWorld.h"
#include "GCExecutor.h"
#include "worldFixtures/WorldFixture.h"

template<unsigned T_numPlayers = 3, unsigned T_width = SeaWorldDefault::width,
         unsigned T_height = SeaWorldDefault::height>
class SeaWorldWithGCExecution : public WorldFixture<CreateSeaWorld, T_numPlayers, T_width, T_height>, public GCExecutor
{
public:
    using WorldFixture<CreateSeaWorld, T_numPlayers, T_width, T_height>::world;

protected:
    GameWorld& GetWorld() override { return world; }
};
