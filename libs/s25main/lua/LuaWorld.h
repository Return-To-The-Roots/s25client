// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "SafeEnum.h"
#include "gameTypes/AnimalTypes.h"

namespace kaguya {
class State;
}

class GameWorld;

class LuaWorld
{
    GameWorld& gw;

public:
    LuaWorld(GameWorld& gw) : gw(gw) {}
    static void Register(kaguya::State& state);
    bool AddEnvObject(int x, int y, unsigned id, unsigned file = 0xFFFF);
    bool AddStaticObject(int x, int y, unsigned id, unsigned file = 0xFFFF, unsigned size = 1);
    void AddAnimal(int x, int y, lua::SafeEnum<Species> species);
    void SetComputerBarrier(unsigned radius, unsigned short x, unsigned short y);
};
